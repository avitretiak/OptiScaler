#pragma once

#include "../pch.h"

#include <d3d12.h>
#include <d3dcompiler.h>
#include "../d3dx/d3dx12.h"

struct alignas(256) Constants
{
    int32_t srcWidth;
    int32_t srcHeight;
    int32_t destWidth;
    int32_t destHeight;
};

class OS_Dx12
{
private:
    std::string _name = "";
    bool _init = false;
    ID3D12RootSignature* _rootSignature = nullptr;
    ID3D12PipelineState* _pipelineState = nullptr;
    ID3D12DescriptorHeap* _srvHeap[3] = { nullptr, nullptr, nullptr };
    D3D12_CPU_DESCRIPTOR_HANDLE _cpuSrvHandle[2]{ { NULL }, { NULL } };
    D3D12_CPU_DESCRIPTOR_HANDLE _cpuUavHandle[2]{ { NULL }, { NULL } };
    D3D12_CPU_DESCRIPTOR_HANDLE _cpuCbvHandle[2]{ { NULL }, { NULL } };
    D3D12_GPU_DESCRIPTOR_HANDLE _gpuSrvHandle[2]{ { NULL }, { NULL } };
    D3D12_GPU_DESCRIPTOR_HANDLE _gpuUavHandle[2]{ { NULL }, { NULL } };
    D3D12_GPU_DESCRIPTOR_HANDLE _gpuCbvHandle[2]{ { NULL }, { NULL } };
    int _counter = 0;
    bool _upsample = false;

    uint32_t InNumThreadsX = 32;
    uint32_t InNumThreadsY = 32;

    inline static std::string downsampleCode = R"(
cbuffer Params : register(b0)
{
	int _SrcWidth;
	int _SrcHeight;
	int _DstWidth;
	int _DstHeight;
};

Texture2D<float4> InputTexture : register(t0);
RWTexture2D<float4> OutputTexture : register(u0);

float bicubic_weight(float x)
{
	float a = -0.75f;
	float absX = abs(x);

	if (absX <= 1.0f)
		return (a + 2.0f) * absX * absX * absX - (a + 3.0f) * absX * absX + 1.0f;
	else if (absX < 2.0f)
		return a * absX * absX * absX - 5.0f * a * absX * absX + 8.0f * a * absX - 4.0f * a;
	else
		return 0.0f;
}

[numthreads(32, 32, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
	if (DTid.x >= _DstWidth || DTid.y >= _DstHeight)
		return;

	float2 uv = float2(DTid.x / (_DstWidth - 1.0f), DTid.y / (_DstHeight - 1.0f));
	float2 pixel = uv * float2(_SrcWidth, _SrcHeight);
	float2 texel = floor(pixel);
	float2 t = pixel - texel;
	t = t * t * (3.0f - 2.0f * t);

	float4 result = float4(0.0f, 0.0f, 0.0f, 0.0f);
	for (int y = -1; y <= 2; y++)
	{
		for (int x = -1; x <= 2; x++)
		{
			float4 color = InputTexture.Load(int3(texel.x + x, texel.y + y, 0));
			float weight = bicubic_weight(x - t.x) * bicubic_weight(y - t.y);
			result += color * weight;
		}
	}

	OutputTexture[DTid.xy] = result;
}
)";

    inline static std::string upsampleCode = R"(
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Developed by Minigraph
//
// Author(s):  James Stanard
//

Texture2D<float3> Source : register(t0);
RWTexture2D<float3> Dest : register(u0);

cbuffer Params : register(b0)
{
	int _SrcWidth;
	int _SrcHeight;
	int _DstWidth;
	int _DstHeight;
};

#define TILE_DIM_X 32
#define TILE_DIM_Y 32

#define GROUP_COUNT (TILE_DIM_X * TILE_DIM_Y)

#define SAMPLES_X (TILE_DIM_X + 3)
#define SAMPLES_Y (TILE_DIM_Y + 3)

#define TOTAL_SAMPLES (SAMPLES_X * SAMPLES_Y)

// De-interleaved to avoid LDS bank conflicts
groupshared float g_R[TOTAL_SAMPLES];
groupshared float g_G[TOTAL_SAMPLES];
groupshared float g_B[TOTAL_SAMPLES];

float W1(float x, float A)
{
	return x * x * ((A + 2) * x - (A + 3)) + 1.0;
}

float W2(float x, float A)
{
	return A * (x * (x * (x - 5) + 8) - 4);
}

float4 ComputeWeights(float d1, float A)
{
	return float4(W2(1.0 + d1, A), W1(d1, A), W1(1.0 - d1, A), W2(2.0 - d1, A));
}

float4 GetBicubicFilterWeights(float offset, float A)
{
	//return ComputeWeights(offset, A);

	// Precompute weights for 16 discrete offsets
	static const float4 FilterWeights[16] =
	{
		ComputeWeights( 0.5 / 16.0, -0.5),
		ComputeWeights( 1.5 / 16.0, -0.5),
		ComputeWeights( 2.5 / 16.0, -0.5),
		ComputeWeights( 3.5 / 16.0, -0.5),
		ComputeWeights( 4.5 / 16.0, -0.5),
		ComputeWeights( 5.5 / 16.0, -0.5),
		ComputeWeights( 6.5 / 16.0, -0.5),
		ComputeWeights( 7.5 / 16.0, -0.5),
		ComputeWeights( 8.5 / 16.0, -0.5),
		ComputeWeights( 9.5 / 16.0, -0.5),
		ComputeWeights(10.5 / 16.0, -0.5),
		ComputeWeights(11.5 / 16.0, -0.5),
		ComputeWeights(12.5 / 16.0, -0.5),
		ComputeWeights(13.5 / 16.0, -0.5),
		ComputeWeights(14.5 / 16.0, -0.5),
		ComputeWeights(15.5 / 16.0, -0.5)
	};

	return FilterWeights[(uint)(offset * 16.0)];
}

// Store pixel to LDS (local data store)
void StoreLDS(uint LdsIdx, float3 rgb)
{
	g_R[LdsIdx] = rgb.r;
	g_G[LdsIdx] = rgb.g;
	g_B[LdsIdx] = rgb.b;
}

// Load four pixel samples from LDS.  Stride determines horizontal or vertical groups.
float3x4 LoadSamples(uint idx, uint Stride)
{
	uint i0 = idx, i1 = idx+Stride, i2 = idx+2*Stride, i3=idx+3*Stride;
	return float3x4(
		g_R[i0], g_R[i1], g_R[i2], g_R[i3],
		g_G[i0], g_G[i1], g_G[i2], g_G[i3],
		g_B[i0], g_B[i1], g_B[i2], g_B[i3]);
}

[numthreads(TILE_DIM_X, TILE_DIM_Y, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID, uint3 Gid : SV_GroupID, uint GI : SV_GroupIndex)
{
	float scaleX = (float)_SrcWidth / (float)_DstWidth;
	float scaleY = (float)_SrcHeight / (float)_DstHeight;
	const float2 kRcpScale = float2(scaleX, scaleY);
	const float kA = 0.3f;
	
	// Number of samples needed from the source buffer to generate the output tile dimensions.
	const uint2 SampleSpace = ceil(float2(TILE_DIM_X, TILE_DIM_Y) * kRcpScale + 3.0);
	
	// Pre-Load source pixels
	int2 UpperLeft = floor((Gid.xy * uint2(TILE_DIM_X, TILE_DIM_Y) + 0.5) * kRcpScale - 1.5);

	for (uint i = GI; i < TOTAL_SAMPLES; i += GROUP_COUNT)
		StoreLDS(i, Source[UpperLeft + int2(i % SAMPLES_X, i / SAMPLES_Y)]);

	GroupMemoryBarrierWithGroupSync();

	// The coordinate of the top-left sample from the 4x4 kernel (offset by -0.5
	// so that whole numbers land on a pixel center.)  This is in source texture space.
	float2 TopLeftSample = (DTid.xy + 0.5) * kRcpScale - 1.5;

	// Position of samples relative to pixels used to evaluate the Sinc function.
	float2 Phase = frac(TopLeftSample);

	// LDS tile coordinate for the top-left sample (for this thread)
	uint2 TileST = int2(floor(TopLeftSample)) - UpperLeft;

	// Convolution weights, one per sample (in each dimension)
	float4 xWeights = GetBicubicFilterWeights(Phase.x, kA);
	float4 yWeights = GetBicubicFilterWeights(Phase.y, kA);

	// Horizontally convolve the first N rows
	uint ReadIdx = TileST.x + GTid.y * SAMPLES_X;

	uint WriteIdx = GTid.x + GTid.y * SAMPLES_X;
	StoreLDS(WriteIdx, mul(LoadSamples(ReadIdx, 1), xWeights));

	// If the source tile plus border is larger than the destination tile, we
	// have to convolve a few more rows.
	if (GI + GROUP_COUNT < SampleSpace.y * TILE_DIM_X)
	{
		ReadIdx += TILE_DIM_Y * SAMPLES_X;
		WriteIdx += TILE_DIM_Y * SAMPLES_X;
		StoreLDS(WriteIdx, mul(LoadSamples(ReadIdx, 1), xWeights));
	}

	GroupMemoryBarrierWithGroupSync();

	// Convolve vertically N columns
	ReadIdx = GTid.x + TileST.y * SAMPLES_X;
	float3 Result = mul(LoadSamples(ReadIdx, SAMPLES_X), yWeights);

	// Transform to display settings
	// Result = RemoveDisplayProfile(Result, LDR_COLOR_FORMAT);
	// Dest[DTid.xy] = ApplyDisplayProfile(Result, DISPLAY_PLANE_FORMAT);
	Dest[DTid.xy] = Result;
}
)";

    ID3D12Device* _device = nullptr;
    ID3D12Resource* _buffer = nullptr;
    ID3D12Resource* _constantBuffer = nullptr;
    D3D12_RESOURCE_STATES _bufferState = D3D12_RESOURCE_STATE_COMMON;

public:
    bool CreateBufferResource(ID3D12Device* InDevice, ID3D12Resource* InSource, uint32_t InWidth, uint32_t InHeight, D3D12_RESOURCE_STATES InState);
    void SetBufferState(ID3D12GraphicsCommandList* InCommandList, D3D12_RESOURCE_STATES InState);
    bool Dispatch(ID3D12Device* InDevice, ID3D12GraphicsCommandList* InCmdList, ID3D12Resource* InResource, ID3D12Resource* OutResource);

    ID3D12Resource* Buffer() { return _buffer; }
    bool IsUpsampling() { return _upsample; }
    bool IsInit() const { return _init; }

    OS_Dx12(std::string InName, ID3D12Device* InDevice, bool InUpsample);

    ~OS_Dx12();
};