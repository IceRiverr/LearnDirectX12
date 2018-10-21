
#include "GraphicContext.h"
#include "Utility.h"
#include "GraphicsUtility.h"

CDescriptorAlloctor::CDescriptorAlloctor(UINT nMaxHeapSize)
{
	m_nMaxHeapSize = nMaxHeapSize;
	m_nCurrentHeapOffset = 0;
	m_pCurrentHeap = nullptr;
}

DescriptorAddress CDescriptorAlloctor::Allocate(UINT nDescriptorNum, ID3D12Device* pDevice)
{
	if (m_pCurrentHeap == nullptr || m_nCurrentHeapOffset + nDescriptorNum >= m_nMaxHeapSize)
	{
		D3D12_DESCRIPTOR_HEAP_DESC cbHeapDesc = {};
		cbHeapDesc.NumDescriptors = m_nMaxHeapSize;
		cbHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		cbHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		cbHeapDesc.NodeMask = 0;
		pDevice->CreateDescriptorHeap(&cbHeapDesc, IID_PPV_ARGS(&m_pCurrentHeap));

		m_nSRVDescriptorSize = pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		m_pSRVHeaps.push_back(m_pCurrentHeap);
		m_nCurrentHeapOffset = 0;
	}

	auto CPUHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_pCurrentHeap->GetCPUDescriptorHandleForHeapStart());
	auto GPUHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_pCurrentHeap->GetGPUDescriptorHandleForHeapStart());

	DescriptorAddress address;
	address.pHeap = m_pCurrentHeap;
	address.nOffset = m_nCurrentHeapOffset;
	address.CpuHandle = CPUHandle.Offset(m_nCurrentHeapOffset, m_nSRVDescriptorSize);
	address.GpuHandle = GPUHandle.Offset(m_nCurrentHeapOffset, m_nSRVDescriptorSize);

	m_nCurrentHeapOffset += nDescriptorNum;

	return address;
}

const std::vector<ID3D12DescriptorHeap*>& CDescriptorAlloctor::GetHeaps() const
{
	return m_pSRVHeaps;
}

CGraphicContext::CGraphicContext()
{
	m_pDevice = nullptr;
	m_pCommandList = nullptr;
	m_pSRVAllocator = nullptr;
}

void CGraphicContext::Init()
{
	m_pSRVAllocator = new CDescriptorAlloctor(256);
}

Texture2DResource* CGraphicContext::CreateTexture2D(const std::string& path, bool bCreateSRV /*= true*/)
{
	auto it = m_Textures.find(path);
	if (it != m_Textures.end())
	{
		return it->second;
	}

	Texture2DResource* pTexture = Graphics::CreateTexture2DResourceFromFile(m_pDevice, m_pCommandList, StringToWString(path));
	if (bCreateSRV)
	{
		DescriptorAddress address = m_pSRVAllocator->Allocate(1, m_pDevice);
		pTexture->m_TextureAddress.pBufferHeap = address.pHeap;
		pTexture->m_TextureAddress.CPUHandle = address.CpuHandle;
		pTexture->m_TextureAddress.GPUHandle = address.GpuHandle;

		CreateTexture2DSRV(pTexture->pTexture, address.CpuHandle);
	}

	m_Textures.emplace(path, pTexture);
	return pTexture;
}

void CGraphicContext::CreateTexture2DSRV(ID3D12Resource * pResource, D3D12_CPU_DESCRIPTOR_HANDLE handle)
{
	// Create SRV
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = pResource->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = pResource->GetDesc().MipLevels;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

	m_pDevice->CreateShaderResourceView(pResource, &srvDesc, handle);
}
