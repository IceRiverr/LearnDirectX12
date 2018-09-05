#include "BufferManager.h"

CFrameBuffer::CFrameBuffer()
{
	m_FrameBufferData = {};

	m_nConstantBufferSizeAligned = 0;
	m_pUploadeConstBuffer = nullptr;
	m_pCbvDataBegin = nullptr;
	m_nDescriptorIndex = 0;
}

CFrameBuffer::~CFrameBuffer()
{
	m_pUploadeConstBuffer->Unmap(0, nullptr);
	m_pUploadeConstBuffer->Release();
	m_pUploadeConstBuffer = nullptr;
}

void CFrameBuffer::CreateBuffer(ID3D12Device* pDevice)
{
	if (pDevice)
	{
		m_nConstantBufferSizeAligned = (sizeof(_Buffer) + 255) & (~255);
		pDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(m_nConstantBufferSizeAligned),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_pUploadeConstBuffer));

		m_pUploadeConstBuffer->Map(0, nullptr, (void**)(&m_pCbvDataBegin));
	}
}

void CFrameBuffer::CreateBufferView(ID3D12Device * pDevice, CD3DX12_CPU_DESCRIPTOR_HANDLE handle)
{
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
	cbvDesc.BufferLocation = m_pUploadeConstBuffer->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = m_nConstantBufferSizeAligned;
	pDevice->CreateConstantBufferView(&cbvDesc, handle);
}

CConstantBuffer::CConstantBuffer()
{
	m_nTotalConstantBuferByteSize = 0;
	m_nConstantBufferSizeAligned = 0;
	m_pUploadeConstBuffer = 0;
	m_pCbvDataBegin = nullptr;
}

CConstantBuffer::~CConstantBuffer()
{
	m_pUploadeConstBuffer->Unmap(0, nullptr);
	m_pUploadeConstBuffer->Release();
	m_pUploadeConstBuffer = nullptr;
}

void CConstantBuffer::CreateBuffer(ID3D12Device * pDevice, UINT nObjectCount)
{
	if (pDevice)
	{
		m_nConstantBufferSizeAligned = CRenderObject::GetConstantElementSizeAligned();
		m_nTotalConstantBuferByteSize = m_nConstantBufferSizeAligned * nObjectCount;
		UINT nSRVDescriptorSize = pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		pDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(m_nTotalConstantBuferByteSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_pUploadeConstBuffer));

		m_pUploadeConstBuffer->Map(0, nullptr, (void**)(&m_pCbvDataBegin));
	}
}

void CConstantBuffer::CreateBufferView(ID3D12Device * pDevice, CD3DX12_CPU_DESCRIPTOR_HANDLE handle, UINT nConstntIndex)
{
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
	cbvDesc.BufferLocation = m_pUploadeConstBuffer->GetGPUVirtualAddress() + nConstntIndex * m_nConstantBufferSizeAligned;
	cbvDesc.SizeInBytes = m_nConstantBufferSizeAligned;
	pDevice->CreateConstantBufferView(&cbvDesc, handle);
}

void CConstantBuffer::UpdateBuffer(UINT8 * pData, UINT nDataSize, UINT nConstntIndex)
{
	UINT8* pObjConstBegin = m_pCbvDataBegin + m_nConstantBufferSizeAligned * nConstntIndex;
	memcpy(pObjConstBegin, pData, nDataSize);
}
