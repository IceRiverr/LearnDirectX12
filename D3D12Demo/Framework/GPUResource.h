#pragma once

#include "stdafx.h"

struct ConstantBufferAddress
{
	ID3D12Resource* pBuffer;
	UINT nBufferIndex;
	ID3D12DescriptorHeap* pBufferHeap;
	UINT nHeapOffset;
};

struct BufferView
{
	ID3D12Resource* pResource;
	UINT64 DataOffset;
	CD3DX12_CPU_DESCRIPTOR_HANDLE CPUHandle;
	CD3DX12_GPU_DESCRIPTOR_HANDLE GPUHandle;
};

class CCommonBuffer
{
public:
	ID3D12Resource* m_pCommonBuffer;
	UINT8* m_pData;

	void Map();
	void UnMap();
};
