#pragma once

#include "stdafx.h"
#include <vector>

struct ConstantBufferAddress
{
	ID3D12Resource* pBuffer;
	UINT nBufferIndex;
	ID3D12DescriptorHeap* pBufferHeap;
	UINT nHeapOffset;
};

struct Texture2DResource
{
	ID3D12Resource* pTexture;
	ID3D12Resource* pUploadBuffer;

	ConstantBufferAddress m_TextureAddress;
};

enum STATIC_SAMPLER_TYPE
{
	Point_Wrap_Sampler,
	Point_Clamp_Sampler,
	Linear_Wrap_Sampler,
	Linear_Clamp_Sampler,
	Anisotropic_Wrap_Sampler,
	Anisotropic_Clamp_Sampler,
	SAMPLER_COUNT
};

struct StaticSamplerStates
{
	std::vector<CD3DX12_STATIC_SAMPLER_DESC> Samplers;
	void CreateStaticSamplers();
};
