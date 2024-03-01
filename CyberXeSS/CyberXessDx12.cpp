#include "pch.h"
#include "Config.h"
#include "CyberXess.h"
#include "Util.h"

#pragma region DLSS Init Calls

NVSDK_NGX_API NVSDK_NGX_Result NVSDK_NGX_D3D12_Init_Ext(unsigned long long InApplicationId, const wchar_t* InApplicationDataPath,
	ID3D12Device* InDevice, const NVSDK_NGX_FeatureCommonInfo* InFeatureInfo, NVSDK_NGX_Version InSDKVersion, unsigned long long unknown0)
{
	spdlog::info("NVSDK_NGX_D3D12_Init_Ext AppId: {0}", InApplicationId);
	spdlog::info("NVSDK_NGX_D3D12_Init_Ext SDK: {0}", (int)InSDKVersion);

	spdlog::info("NVSDK_NGX_D3D12_Init_Ext BuildPipelines: {0}", CyberXessContext::instance()->MyConfig->BuildPipelines.value_or(true));
	spdlog::info("NVSDK_NGX_D3D12_Init_Ext NetworkModel: {0}", CyberXessContext::instance()->MyConfig->NetworkModel.value_or(0));
	spdlog::info("NVSDK_NGX_D3D12_Init_Ext LogLevel: {0}", CyberXessContext::instance()->MyConfig->LogLevel.value_or(2));

	CyberXessContext::instance()->Shutdown(true, true);

	if (InDevice)
		CyberXessContext::instance()->Dx12Device = InDevice;
	else
		spdlog::error("NVSDK_NGX_D3D12_Init_Ext InDevice is null!");

	return NVSDK_NGX_Result_Success;
}

NVSDK_NGX_API NVSDK_NGX_Result NVSDK_NGX_D3D12_Init(unsigned long long InApplicationId, const wchar_t* InApplicationDataPath,
	ID3D12Device* InDevice, const NVSDK_NGX_FeatureCommonInfo* InFeatureInfo, NVSDK_NGX_Version InSDKVersion)
{
	spdlog::debug("NVSDK_NGX_D3D12_Init AppId: {0}", InApplicationId);
	spdlog::debug("NVSDK_NGX_D3D12_Init SDK: {0}", (int)InSDKVersion);

	return NVSDK_NGX_D3D12_Init_Ext(InApplicationId, InApplicationDataPath, InDevice, InFeatureInfo, InSDKVersion, 0);
}

NVSDK_NGX_API NVSDK_NGX_Result NVSDK_NGX_D3D12_Init_ProjectID(const char* InProjectId, NVSDK_NGX_EngineType InEngineType,
	const char* InEngineVersion, const wchar_t* InApplicationDataPath, ID3D12Device* InDevice, const NVSDK_NGX_FeatureCommonInfo* InFeatureInfo, NVSDK_NGX_Version InSDKVersion)
{
	spdlog::debug("NVSDK_NGX_D3D12_Init_ProjectID: {0}", InProjectId);
	spdlog::debug("NVSDK_NGX_D3D12_Init_ProjectID SDK: {0}", (int)InSDKVersion);

	return NVSDK_NGX_D3D12_Init_Ext(0x1337, InApplicationDataPath, InDevice, InFeatureInfo, InSDKVersion, 0);
}

NVSDK_NGX_API NVSDK_NGX_Result NVSDK_NGX_D3D12_Init_with_ProjectID(const char* InProjectId, NVSDK_NGX_EngineType InEngineType, const char* InEngineVersion,
	const wchar_t* InApplicationDataPath, ID3D12Device* InDevice, const NVSDK_NGX_FeatureCommonInfo* InFeatureInfo, NVSDK_NGX_Version InSDKVersion)
{
	spdlog::debug("NVSDK_NGX_D3D12_Init_with_ProjectID: {0}", InProjectId);
	spdlog::debug("NVSDK_NGX_D3D12_Init_with_ProjectID SDK: {0}", (int)InSDKVersion);

	return NVSDK_NGX_D3D12_Init_Ext(0x1337, InApplicationDataPath, InDevice, InFeatureInfo, InSDKVersion, 0);
}

#pragma endregion

#pragma region DLSS Shutdown Calls

NVSDK_NGX_API NVSDK_NGX_Result NVSDK_NGX_D3D12_Shutdown(void)
{
	spdlog::info("NVSDK_NGX_D3D12_Shutdown");

	CyberXessContext::instance()->Shutdown(false, true);

	for (auto const& [key, val] : CyberXessContext::instance()->Contexts)
		NVSDK_NGX_D3D12_ReleaseFeature(&val->Handle);

	CyberXessContext::instance()->Contexts.clear();

	return NVSDK_NGX_Result_Success;
}

NVSDK_NGX_API NVSDK_NGX_Result NVSDK_NGX_D3D12_Shutdown1(ID3D12Device* InDevice)
{
	spdlog::info("NVSDK_NGX_D3D12_Shutdown1");

	CyberXessContext::instance()->Shutdown(false, true);

	for (auto const& [key, val] : CyberXessContext::instance()->Contexts)
		NVSDK_NGX_D3D12_ReleaseFeature(&val->Handle);

	CyberXessContext::instance()->Contexts.clear();

	return NVSDK_NGX_Result_Success;
}

#pragma endregion

#pragma region DLSS Parameter Calls

NVSDK_NGX_API NVSDK_NGX_Result NVSDK_NGX_D3D12_GetParameters(NVSDK_NGX_Parameter** OutParameters)
{
	spdlog::debug("NVSDK_NGX_D3D12_GetParameters");

	try
	{
		*OutParameters = GetNGXParameters();
		return NVSDK_NGX_Result_Success;
	}
	catch (const std::exception& ex)
	{
		spdlog::error("NVSDK_NGX_D3D12_GetParameters exception: {0}", ex.what());
		return NVSDK_NGX_Result_Fail;
	}
}

NVSDK_NGX_API NVSDK_NGX_Result NVSDK_NGX_D3D12_GetCapabilityParameters(NVSDK_NGX_Parameter** OutParameters)
{
	spdlog::debug("NVSDK_NGX_D3D12_GetCapabilityParameters");

	try
	{
		*OutParameters = GetNGXParameters();
		return NVSDK_NGX_Result_Success;
	}
	catch (const std::exception& ex)
	{
		spdlog::error("NVSDK_NGX_D3D12_GetCapabilityParameters exception: {0}", ex.what());
		return NVSDK_NGX_Result_Fail;
	}
}

NVSDK_NGX_API NVSDK_NGX_Result NVSDK_NGX_D3D12_AllocateParameters(NVSDK_NGX_Parameter** OutParameters)
{
	spdlog::debug("NVSDK_NGX_D3D12_AllocateParameters");

	try
	{
		*OutParameters = new NGXParameters();
		return NVSDK_NGX_Result_Success;
	}
	catch (const std::exception& ex)
	{
		spdlog::error("NVSDK_NGX_D3D12_AllocateParameters exception: {0}", ex.what());
		return NVSDK_NGX_Result_Fail;
	}
}

NVSDK_NGX_API NVSDK_NGX_Result NVSDK_NGX_D3D12_PopulateParameters_Impl(NVSDK_NGX_Parameter* InParameters)
{
	spdlog::debug("NVSDK_NGX_D3D12_PopulateParameters_Impl");

	if (InParameters == nullptr)
		return NVSDK_NGX_Result_Fail;

	InitNGXParameters(InParameters);

	return NVSDK_NGX_Result_Success;
}

NVSDK_NGX_API NVSDK_NGX_Result NVSDK_NGX_D3D12_DestroyParameters(NVSDK_NGX_Parameter* InParameters)
{

	spdlog::debug("NVSDK_NGX_D3D12_DestroyParameters");

	if (InParameters == nullptr)
		return NVSDK_NGX_Result_Fail;

	delete InParameters;
	return NVSDK_NGX_Result_Success;
}

#pragma endregion

#pragma region DLSS Feature Calls

NVSDK_NGX_API NVSDK_NGX_Result NVSDK_NGX_D3D12_CreateFeature(ID3D12GraphicsCommandList* InCmdList, NVSDK_NGX_Feature InFeatureID, NVSDK_NGX_Parameter* InParameters, NVSDK_NGX_Handle** OutHandle)
{
	spdlog::info("NVSDK_NGX_D3D12_CreateFeature");

	auto deviceContext = CyberXessContext::instance()->CreateContext();
	*OutHandle = &deviceContext->Handle;

#pragma region Check for Dx12Device Device

	if (CyberXessContext::instance()->Dx12Device == nullptr)
	{
		if (InCmdList == nullptr && CyberXessContext::instance()->Dx11Device != nullptr)
		{
			spdlog::error("NVSDK_NGX_D3D12_CreateFeature InCmdList is null!!!");
			auto fl = CyberXessContext::instance()->Dx11Device->GetFeatureLevel();
			CyberXessContext::instance()->CreateDx12Device(fl);
		}
		else
		{
			spdlog::warn("NVSDK_NGX_D3D12_CreateFeature CyberXessContext::instance()->Dx12Device is null trying to get from InCmdList!");
			InCmdList->GetDevice(IID_PPV_ARGS(&CyberXessContext::instance()->Dx12Device));
		}

		if (CyberXessContext::instance()->Dx12Device == nullptr)
		{
			spdlog::error("NVSDK_NGX_D3D12_CreateFeature CyberXessContext::instance()->Dx12Device can't receive from InCmdList!");
			return NVSDK_NGX_Result_Fail;
		}
	}

#pragma endregion

	if (deviceContext->XeSSInit(CyberXessContext::instance()->Dx12Device, InParameters))
		return NVSDK_NGX_Result_Success;

	spdlog::error("NVSDK_NGX_D3D12_CreateFeature: CreateFeature failed");
	return NVSDK_NGX_Result_Fail;
}

NVSDK_NGX_API NVSDK_NGX_Result NVSDK_NGX_D3D12_ReleaseFeature(NVSDK_NGX_Handle* InHandle)
{
	spdlog::info("NVSDK_NGX_D3D12_ReleaseFeature");

	if (InHandle == nullptr || InHandle == NULL)
		return NVSDK_NGX_Result_Success;

	if (auto deviceContext = CyberXessContext::instance()->Contexts[InHandle->Id].get(); deviceContext != nullptr)
		deviceContext->XeSSDestroy();

	CyberXessContext::instance()->DeleteContext(InHandle);

	return NVSDK_NGX_Result_Success;
}

NVSDK_NGX_API NVSDK_NGX_Result NVSDK_NGX_D3D12_GetFeatureRequirements(IDXGIAdapter* Adapter, const NVSDK_NGX_FeatureDiscoveryInfo* FeatureDiscoveryInfo,
	NVSDK_NGX_FeatureRequirement* OutSupported)
{
	spdlog::debug("NVSDK_NGX_D3D12_GetFeatureRequirements!");

	*OutSupported = NVSDK_NGX_FeatureRequirement();
	OutSupported->FeatureSupported = NVSDK_NGX_FeatureSupportResult_Supported;
	OutSupported->MinHWArchitecture = 0;
	//Some windows 10 os version
	strcpy_s(OutSupported->MinOSVersion, "10.0.10240.16384");
	return NVSDK_NGX_Result_Success;
}

NVSDK_NGX_API NVSDK_NGX_Result NVSDK_NGX_D3D12_EvaluateFeature(ID3D12GraphicsCommandList* InCmdList, const NVSDK_NGX_Handle* InFeatureHandle, const NVSDK_NGX_Parameter* InParameters, PFN_NVSDK_NGX_ProgressCallback InCallback)
{
	spdlog::debug("NVSDK_NGX_D3D12_EvaluateFeature init!");

	if (!InCmdList)
	{
		spdlog::error("NVSDK_NGX_D3D12_EvaluateFeature InCmdList is null!!!");
		return NVSDK_NGX_Result_Fail;
	}

	if (InCallback)
		spdlog::warn("NVSDK_NGX_D3D12_EvaluateFeature callback exist");

	const auto deviceContext = CyberXessContext::instance()->Contexts[InFeatureHandle->Id].get();

	unsigned int width, outWidth, height, outHeight;

	if (InParameters->Get(NVSDK_NGX_Parameter_Width, &width) == NVSDK_NGX_Result_Success &&
		InParameters->Get(NVSDK_NGX_Parameter_Height, &height) == NVSDK_NGX_Result_Success &&
		InParameters->Get(NVSDK_NGX_Parameter_OutWidth, &outWidth) == NVSDK_NGX_Result_Success &&
		InParameters->Get(NVSDK_NGX_Parameter_OutHeight, &outHeight) == NVSDK_NGX_Result_Success)
	{
		width = width > outWidth ? width : outWidth;
		height = height > outHeight ? height : outHeight;

		if (deviceContext->XeSSIsInited() && (deviceContext->DisplayHeight != height || deviceContext->DisplayWidth != width))
			deviceContext->XeSSDestroy();
	}

	if (!deviceContext->XeSSIsInited())
	{
		deviceContext->XeSSInit(CyberXessContext::instance()->Dx12Device, InParameters);

		if (!deviceContext->XeSSIsInited())
		{
			spdlog::error("NVSDK_NGX_D3D12_EvaluateFeature deviceContext.XessInit is false, init failed!");
			return NVSDK_NGX_Result_Fail;
		}
	}

	//dumpParams.frame_count = 1;
	//dumpParams.frame_idx = cnt++;
	//dumpParams.path = "D:\\dmp\\";
	//xessStartDump(deviceContext->XessContext, &dumpParams);

	if (deviceContext->XeSSExecuteDx12(InCmdList, InParameters, deviceContext))
		return NVSDK_NGX_Result_Success;
	else
		return NVSDK_NGX_Result_Fail;
}

NVSDK_NGX_API NVSDK_NGX_Result NVSDK_NGX_UpdateFeature(const NVSDK_NGX_Application_Identifier* ApplicationId, const NVSDK_NGX_Feature FeatureID)
{
	spdlog::debug("NVSDK_NGX_UpdateFeature -> {0}", (int)FeatureID);
	return NVSDK_NGX_Result_Success;
}

#pragma endregion

#pragma region DLSS Buffer Size Call

NVSDK_NGX_API NVSDK_NGX_Result NVSDK_NGX_D3D12_GetScratchBufferSize(NVSDK_NGX_Feature InFeatureId, const NVSDK_NGX_Parameter* InParameters, size_t* OutSizeInBytes)
{
	spdlog::warn("NVSDK_NGX_D3D12_GetScratchBufferSize -> 52428800");
	*OutSizeInBytes = 52428800;
	return NVSDK_NGX_Result_Success;
}

#pragma endregion
