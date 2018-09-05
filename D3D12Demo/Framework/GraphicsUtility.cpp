#include "GraphicsUtility.h"

ID3D12Resource* Graphics::CreateDefaultBuffer(ID3D12Device* pDevice, ID3D12GraphicsCommandList* cmdList, const void* initData, UINT64 byteSize, ID3D12Resource** ppUploadBuffer)
{
	ID3D12Resource * pDefaultBuffer = nullptr;
	if (pDevice && cmdList)
	{
		pDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(byteSize),
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			IID_PPV_ARGS(&pDefaultBuffer));

		pDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),// This heap type is best for CPU-write-once, GPU-read-once data
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(byteSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(ppUploadBuffer));

		D3D12_SUBRESOURCE_DATA subResource = {};
		subResource.pData = initData;
		subResource.RowPitch = byteSize;
		subResource.SlicePitch = byteSize;

		cmdList->ResourceBarrier(
			1, &CD3DX12_RESOURCE_BARRIER::Transition(
				pDefaultBuffer,
				D3D12_RESOURCE_STATE_COMMON,
				D3D12_RESOURCE_STATE_COPY_DEST));

		UpdateSubresources<1>(cmdList, pDefaultBuffer, *ppUploadBuffer, 0, 0, 1, &subResource);

		cmdList->ResourceBarrier(
			1, &CD3DX12_RESOURCE_BARRIER::Transition(
				pDefaultBuffer,
				D3D12_RESOURCE_STATE_COPY_DEST,
				D3D12_RESOURCE_STATE_GENERIC_READ));
	}

	return pDefaultBuffer;
}