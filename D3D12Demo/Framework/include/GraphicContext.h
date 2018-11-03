

// 资源的创建命令
// 命令行的提交
// 函数的参数命令

#pragma once

#include "stdafx.h"
#include "GPUResource.h"
#include <unordered_map>

class CGraphicContext;

struct UniformBufferLocation
{
	UINT8* pData;
	UINT size;
	ID3D12Resource* pResource;
	ID3D12DescriptorHeap* pBufferHeap;
	CD3DX12_CPU_DESCRIPTOR_HANDLE CPUHandle;
	CD3DX12_GPU_DESCRIPTOR_HANDLE GPUHandle;
};

class CDescriptorAlloctor
{
public:
	CDescriptorAlloctor(UINT nMaxHeapSize);
	~CDescriptorAlloctor();

	DescriptorAddress Allocate(UINT nDescriptorNum, ID3D12Device* pDevice);
	const std::vector<ID3D12DescriptorHeap*>& GetHeaps() const;

private:
	std::vector<ID3D12DescriptorHeap*> m_pSRVHeaps;
	UINT m_nMaxHeapSize;
	UINT m_nCurrentHeapOffset;
	ID3D12DescriptorHeap* m_pCurrentHeap;
	UINT m_nSRVDescriptorSize;
};

class CUniformBufferAllocator
{
public:
	CUniformBufferAllocator();
	~CUniformBufferAllocator();

	UniformBufferLocation* Allocate(UINT bufferSize, CGraphicContext* pContext);
	void Deallocate();

	ID3D12Resource* CreateNewPage(ID3D12Device* pDevice);

private:
	std::vector<ID3D12Resource*> m_ResourcePool;
	UINT m_nPerPageSize;
	UINT m_nCurrentOffset;
	UINT8* m_pCurrentGPUVA;
	ID3D12Resource* m_pCurrentResource;
};

class CGrahpicsRootSignature
{
public:
	std::vector<CD3DX12_ROOT_PARAMETER> m_RootParamaters;
	ID3D12RootSignature* m_pRootSignature;
};

class CGraphicContext
{
public:
	CGraphicContext();
	~CGraphicContext();
	
	void Init();
	void ReleaseUploadBuffers();

	Texture2D* CreateTexture2DResourceFromFile(std::wstring imagePath, bool bGenerateMipmaps = false);
	Texture2D* CreateTexture2D(const std::string& path, bool bCreateSRV = true);
	void CreateTexture2DSRV(ID3D12Resource* pResource, D3D12_CPU_DESCRIPTOR_HANDLE handle);
	ID3D12Resource* CreateDefaultBuffer(const void* initData, UINT64 byteSize);
	D3D12_VERTEX_BUFFER_VIEW CreateVertexBufferView(ID3D12Resource* pVertexBuffer, UINT size, UINT stride);
	D3D12_INDEX_BUFFER_VIEW CreateIndexBufferView(ID3D12Resource* pIndexBuffer, UINT size, DXGI_FORMAT format);
	ID3DBlob* CompileShader(const std::string& sFileName, const std::string& sEntrypoint, const std::string& sTarget, const D3D_SHADER_MACRO* pDefines = nullptr);

public:
	ID3D12Device* m_pDevice;
	ID3D12GraphicsCommandList* m_pCommandList;
	ID3D12RootSignature* m_pRootSignature;

	DXGI_FORMAT	m_BackBufferFromat;
	DXGI_FORMAT m_DSVFormat;

	CDescriptorAlloctor* m_pSRVAllocator;
	CUniformBufferAllocator* m_pUniformBufferAllocator;
	
	std::unordered_map<std::string, Texture2D*> m_Textures;

private:
	std::vector<ID3D12Resource*> m_pUploladBuffers;
};
