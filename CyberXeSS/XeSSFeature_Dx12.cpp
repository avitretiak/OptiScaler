#include "XeSSFeature_Dx12.h"

void XeSSFeatureDx12::SetInit(bool value)
{
	XeSSFeature::SetInit(value);
}

bool XeSSFeatureDx12::IsInited()
{
    return XeSSFeature::IsInited();
}

bool XeSSFeatureDx12::Init(ID3D12Device* device, const NVSDK_NGX_Parameter* InParameters) 
{
	if (IsInited())
		return true;

	Device = device;

	return InitXeSS(device, InParameters);
}

bool XeSSFeatureDx12::Evaluate(ID3D12GraphicsCommandList* commandList, const NVSDK_NGX_Parameter* InParameters) 
{
	if (!IsInited() || !_xessContext)
		return false;

	xess_result_t xessResult;
	xess_d3d12_execute_params_t params{};

	InParameters->Get(NVSDK_NGX_Parameter_Jitter_Offset_X, &params.jitterOffsetX);
	InParameters->Get(NVSDK_NGX_Parameter_Jitter_Offset_Y, &params.jitterOffsetY);

	if (InParameters->Get(NVSDK_NGX_Parameter_DLSS_Exposure_Scale, &params.exposureScale) != NVSDK_NGX_Result_Success)
		params.exposureScale = 1.0f;

	InParameters->Get(NVSDK_NGX_Parameter_Reset, &params.resetHistory);

	XeSSFeature::SetRenderResolution(InParameters, &params);

	spdlog::debug("XeSSFeatureDx12::Evaluate Input Resolution: {0}x{1}", params.inputWidth, params.inputHeight);

	if (InParameters->Get(NVSDK_NGX_Parameter_Color, &params.pColorTexture) != NVSDK_NGX_Result_Success)
		InParameters->Get(NVSDK_NGX_Parameter_Color, (void**)&params.pColorTexture);

	if (params.pColorTexture)
	{
		spdlog::debug("XeSSFeatureDx12::Evaluate Color exist..");

		if (GetConfig()->ColorResourceBarrier.has_value())
			ResourceBarrier(commandList, params.pColorTexture,
				(D3D12_RESOURCE_STATES)GetConfig()->ColorResourceBarrier.value(),
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	}
	else
	{
		spdlog::error("XeSSFeatureDx12::Evaluate Color not exist!!");
		return false;
	}

	if (InParameters->Get(NVSDK_NGX_Parameter_MotionVectors, &params.pVelocityTexture) != NVSDK_NGX_Result_Success)
		InParameters->Get(NVSDK_NGX_Parameter_MotionVectors, (void**)&params.pVelocityTexture);

	if (params.pVelocityTexture)
	{
		spdlog::debug("XeSSFeatureDx12::Evaluate MotionVectors exist..");

		if (GetConfig()->MVResourceBarrier.has_value())
			ResourceBarrier(commandList, params.pVelocityTexture,
				(D3D12_RESOURCE_STATES)GetConfig()->MVResourceBarrier.value(),
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	}
	else
	{
		spdlog::error("XeSSFeatureDx12::Evaluate MotionVectors not exist!!");
		return false;
	}

	ID3D12Resource* paramOutput;
	if (InParameters->Get(NVSDK_NGX_Parameter_Output, &paramOutput) != NVSDK_NGX_Result_Success)
		InParameters->Get(NVSDK_NGX_Parameter_Output, (void**)&paramOutput);

	if (paramOutput)
	{
		spdlog::debug("XeSSFeatureDx12::Evaluate Output exist..");

		if (GetConfig()->OutputResourceBarrier.has_value())
			ResourceBarrier(commandList, paramOutput,
				(D3D12_RESOURCE_STATES)GetConfig()->OutputResourceBarrier.value(),
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		params.pOutputTexture = paramOutput;
	}
	else
	{
		spdlog::error("XeSSFeatureDx12::Evaluate Output not exist!!");
		return false;
	}

	if (InParameters->Get(NVSDK_NGX_Parameter_Depth, &params.pDepthTexture) != NVSDK_NGX_Result_Success)
		InParameters->Get(NVSDK_NGX_Parameter_Depth, (void**)&params.pDepthTexture);

	if (params.pDepthTexture)
	{
		spdlog::debug("XeSSFeatureDx12::Evaluate Depth exist..");

		if (GetConfig()->DepthResourceBarrier.has_value())
			ResourceBarrier(commandList, params.pDepthTexture,
				(D3D12_RESOURCE_STATES)GetConfig()->DepthResourceBarrier.value(),
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	}
	else
	{
		if (!GetConfig()->DisplayResolution.value_or(false))
			spdlog::error("XeSSFeatureDx12::Evaluate Depth not exist!!");
		else
			spdlog::info("XeSSFeatureDx12::Evaluate Using high res motion vectors, depth is not needed!!");

		params.pDepthTexture = nullptr;
	}

	if (!GetConfig()->AutoExposure.value_or(false))
	{
		if (InParameters->Get(NVSDK_NGX_Parameter_ExposureTexture, &params.pExposureScaleTexture) != NVSDK_NGX_Result_Success)
			InParameters->Get(NVSDK_NGX_Parameter_ExposureTexture, (void**)&params.pExposureScaleTexture);

		if (params.pExposureScaleTexture)
			spdlog::debug("XeSSFeatureDx12::Evaluate ExposureTexture exist..");
		else
			spdlog::debug("XeSSFeatureDx12::Evaluate AutoExposure disabled but ExposureTexture is not exist, it may cause problems!!");

		if (GetConfig()->ExposureResourceBarrier.has_value())
			ResourceBarrier(commandList, params.pExposureScaleTexture,
				(D3D12_RESOURCE_STATES)GetConfig()->ExposureResourceBarrier.value(),
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	}
	else
		spdlog::debug("XeSSFeatureDx12::Evaluate AutoExposure enabled!");

	if (!GetConfig()->DisableReactiveMask.value_or(true))
	{
		if (InParameters->Get(NVSDK_NGX_Parameter_DLSS_Input_Bias_Current_Color_Mask, &params.pResponsivePixelMaskTexture) != NVSDK_NGX_Result_Success)
			InParameters->Get(NVSDK_NGX_Parameter_DLSS_Input_Bias_Current_Color_Mask, (void**)&params.pResponsivePixelMaskTexture);

		if (params.pResponsivePixelMaskTexture)
			spdlog::debug("XeSSFeatureDx12::Evaluate Bias mask exist..");
		else
			spdlog::debug("XeSSFeatureDx12::Evaluate Bias mask not exist and its enabled in config, it may cause problems!!");

		if (GetConfig()->MaskResourceBarrier.has_value())
			ResourceBarrier(commandList, params.pResponsivePixelMaskTexture,
				(D3D12_RESOURCE_STATES)GetConfig()->MaskResourceBarrier.value(),
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	}

	float MVScaleX;
	float MVScaleY;

	if (InParameters->Get(NVSDK_NGX_Parameter_MV_Scale_X, &MVScaleX) == NVSDK_NGX_Result_Success &&
		InParameters->Get(NVSDK_NGX_Parameter_MV_Scale_Y, &MVScaleY) == NVSDK_NGX_Result_Success)
	{
		xessResult = xessSetVelocityScale(_xessContext, MVScaleX, MVScaleY);

		if (xessResult != XESS_RESULT_SUCCESS)
		{
			spdlog::error("XeSSFeatureDx12::Evaluate xessSetVelocityScale: {0}", ResultToString(xessResult));
			return false;
		}
	}
	else
		spdlog::warn("XeSSFeatureDx12::Evaluate Can't get motion vector scales!");

	spdlog::debug("XeSSFeatureDx12::Evaluate Executing!!");
	xessResult = xessD3D12Execute(_xessContext, commandList, &params);

	if (xessResult != XESS_RESULT_SUCCESS)
	{
		spdlog::error("XeSSFeatureDx12::Evaluate xessD3D12Execute error: {0}", ResultToString(xessResult));
		return false;
	}

	// restore resource states
	if (params.pColorTexture && GetConfig()->ColorResourceBarrier.value_or(false))
		ResourceBarrier(commandList, params.pColorTexture,
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
			(D3D12_RESOURCE_STATES)GetConfig()->ColorResourceBarrier.value());

	if (GetConfig()->MVResourceBarrier.has_value())
		ResourceBarrier(commandList, params.pVelocityTexture,
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
			(D3D12_RESOURCE_STATES)GetConfig()->MVResourceBarrier.value());

	if (GetConfig()->OutputResourceBarrier.has_value())
		ResourceBarrier(commandList, paramOutput,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			(D3D12_RESOURCE_STATES)GetConfig()->MVResourceBarrier.value());

	if (GetConfig()->DepthResourceBarrier.has_value())
		ResourceBarrier(commandList, params.pDepthTexture,
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
			(D3D12_RESOURCE_STATES)GetConfig()->DepthResourceBarrier.value());

	if (GetConfig()->ExposureResourceBarrier.has_value())
		ResourceBarrier(commandList, params.pExposureScaleTexture,
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
			(D3D12_RESOURCE_STATES)GetConfig()->ExposureResourceBarrier.value());

	if (GetConfig()->MaskResourceBarrier.has_value())
		ResourceBarrier(commandList, params.pResponsivePixelMaskTexture,
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
			(D3D12_RESOURCE_STATES)GetConfig()->MaskResourceBarrier.value());

	return true;
}

void XeSSFeatureDx12::ReInit(const NVSDK_NGX_Parameter* InParameters) 
{
	SetInit(false);

	if (_xessContext)
		xessDestroyContext(_xessContext);

	SetInit(Init(Device, InParameters));
}