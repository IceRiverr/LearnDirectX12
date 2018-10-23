#pragma once

#include "stdafx.h"
#include <vector>
#include <d3dx12.h>

struct ConstantBufferAddress
{
	ID3D12Resource* pBuffer;
	UINT nBufferIndex;
	ID3D12DescriptorHeap* pBufferHeap;
	CD3DX12_CPU_DESCRIPTOR_HANDLE CPUHandle;
	CD3DX12_GPU_DESCRIPTOR_HANDLE GPUHandle;
};

struct DescriptorAddress
{
	ID3D12DescriptorHeap* pHeap;
	UINT nOffset;
	CD3DX12_CPU_DESCRIPTOR_HANDLE CpuHandle;
	CD3DX12_GPU_DESCRIPTOR_HANDLE GpuHandle;
};

struct Texture2D
{
	std::string Name;
	std::string ImagePath;
	ID3D12Resource* pResource;
	CD3DX12_CPU_DESCRIPTOR_HANDLE CPUHandle;
	CD3DX12_GPU_DESCRIPTOR_HANDLE GPUHandle;
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

enum class ROOT_SIGNATURE_INDEX
{
	FRAME_BUFFER_INDEX = 0,
	OBJECT_BUFFER_INDEX = 1,
	MATERIAL_BUFFER_INDEX = 2,
	MATERIAL_TEXTURE_INDEX = 3,

	SKY_SPHERE_BACKGROUND_INDEX = 4,
};
