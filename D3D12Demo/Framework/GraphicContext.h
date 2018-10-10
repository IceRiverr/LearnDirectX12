

// 资源的创建命令
// 命令行的提交
// 函数的参数命令

#pragma once

#include "stdafx.h"
#include "GPUResource.h"
#include <unordered_map>

class CDescriptorAlloctor
{
public:
	CDescriptorAlloctor(UINT nMaxHeapSize);

	DescriptorAddress Allocate(UINT nDescriptorNum, ID3D12Device* pDevice);
	
	const std::vector<ID3D12DescriptorHeap*>& GetHeaps() const;

private:
	std::vector<ID3D12DescriptorHeap*> m_pSRVHeaps;
	UINT m_nMaxHeapSize;
	UINT m_nCurrentHeapOffset;
	ID3D12DescriptorHeap* m_pCurrentHeap;
	UINT m_nSRVDescriptorSize;
};

class CGraphicContext
{
public:
	CGraphicContext();

	void Init();
	Texture2DResource* CreateTexture(const std::string& path, bool bCreateSRV = true);
	void CreateTextureSRV(ID3D12Resource* pResource, D3D12_CPU_DESCRIPTOR_HANDLE handle);

public:
	ID3D12Device* m_pDevice;
	ID3D12GraphicsCommandList* m_pCommandList;
	ID3D12RootSignature* m_pRootSignature;

	CDescriptorAlloctor* m_pSRVAllocator;
	
	std::unordered_map<std::string, Texture2DResource*> m_Textures;

private:

};
