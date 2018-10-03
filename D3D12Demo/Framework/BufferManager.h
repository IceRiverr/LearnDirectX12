
#pragma once
#include "stdafx.h"
#include "StaticMesh.h"
#include "Light.h"

class CFrameBuffer
{
public:
	CFrameBuffer();
	~CFrameBuffer();

	void CreateBuffer(ID3D12Device* pDevice);
	void CreateBufferView(ID3D12Device * pDevice, CD3DX12_CPU_DESCRIPTOR_HANDLE handle);

public:
	struct _Buffer
	{
		XMFLOAT4X4 g_mView;
		XMFLOAT4X4 g_mInvView;
		XMFLOAT4X4 g_mProj;
		XMFLOAT4X4 g_mInvProj;
		XMFLOAT4X4 g_mViewProj;
		XMFLOAT4X4 g_mInvViewProj;
		XMFLOAT3 g_vEyePosition;
		float PAD_0;
		XMFLOAT2 g_RenderTargetSize;
		XMFLOAT2 g_InvRenderTargetSize;
		float g_fNearZ;
		float g_fFarZ;
		float g_fTotalTime;
		float g_fDeltaTime;

		int g_LightNumbers[4]; // 0 DirLightNum 1 PointLightNum 2 SpotLightNum
		LightInfoShaderStruct g_Lights[16];
	};

	_Buffer m_FrameData;

	UINT m_nConstantBufferSizeAligned;
	ID3D12Resource* m_pUploadeConstBuffer;
	UINT8* m_pCbvDataBegin;

	UINT m_nDescriptorIndex;
};

// ×¢Òâ4×Ö½Ú¶ÔÆë
template<typename T>
class TBaseConstantBuffer
{
public:
	TBaseConstantBuffer();
	~TBaseConstantBuffer();

	void CreateBuffer(ID3D12Device* pDevice, UINT nObjectCount);
	void CreateBufferView(ID3D12Device * pDevice, CD3DX12_CPU_DESCRIPTOR_HANDLE handle, UINT nConstntIndex);
	void UpdateBuffer(UINT8 * pData, UINT nDataSize, UINT nConstntIndex);

public:
	UINT m_nTotalConstantBuferByteSize;
	UINT m_nConstantBufferSizeAligned;
	ID3D12Resource* m_pUploadeConstBuffer;
	UINT8* m_pCbvDataBegin;
};

template<typename T>
TBaseConstantBuffer<T>::TBaseConstantBuffer()
{
	m_nTotalConstantBuferByteSize = 0;
	m_nConstantBufferSizeAligned = CalcConstBufferBytersAligned(sizeof(T));
	m_pUploadeConstBuffer = 0;
	m_pCbvDataBegin = nullptr;
}

template<typename T>
TBaseConstantBuffer<T>::~TBaseConstantBuffer()
{
	if (m_pUploadeConstBuffer)
	{
		m_pUploadeConstBuffer->Unmap(0, nullptr);
		m_pUploadeConstBuffer->Release();
		m_pUploadeConstBuffer = nullptr;
	}
}

template<typename T>
void TBaseConstantBuffer<T>::CreateBuffer(ID3D12Device * pDevice, UINT nObjectCount)
{
	if (pDevice)
	{
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

template<typename T>
void TBaseConstantBuffer<T>::CreateBufferView(ID3D12Device * pDevice, CD3DX12_CPU_DESCRIPTOR_HANDLE handle, UINT nConstntIndex)
{
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
	cbvDesc.BufferLocation = m_pUploadeConstBuffer->GetGPUVirtualAddress() + nConstntIndex * m_nConstantBufferSizeAligned;
	cbvDesc.SizeInBytes = m_nConstantBufferSizeAligned;
	pDevice->CreateConstantBufferView(&cbvDesc, handle);
}

template<typename T>
void TBaseConstantBuffer<T>::UpdateBuffer(UINT8 * pData, UINT nDataSize, UINT nConstntIndex)
{
	UINT8* pObjConstBegin = m_pCbvDataBegin + m_nConstantBufferSizeAligned * nConstntIndex;
	memcpy(pObjConstBegin, pData, nDataSize);
}

typedef TBaseConstantBuffer<ObjectShaderBlock> CObjectConstantBuffer;
typedef TBaseConstantBuffer<MaterialShaderBlock> CMaterialConstantBuffer;
