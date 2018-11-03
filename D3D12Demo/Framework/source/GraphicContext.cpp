
#include "GraphicContext.h"
#include "Utility.h"
#include "DirectXTex.h"
#include "DirectXTexExr.h"
#include <d3dx12.h>

CDescriptorAlloctor::CDescriptorAlloctor(UINT nMaxHeapSize)
{
	m_nMaxHeapSize = nMaxHeapSize;
	m_nCurrentHeapOffset = 0;
	m_pCurrentHeap = nullptr;
}

CDescriptorAlloctor::~CDescriptorAlloctor()
{
	for (int i = 0; i < m_pSRVHeaps.size(); ++i)
	{
		m_pSRVHeaps[i]->Release();
	}
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

CGraphicContext::~CGraphicContext()
{
	delete m_pSRVAllocator;
	delete m_pUniformBufferAllocator;

	for (auto it = m_Textures.begin(); it != m_Textures.end(); ++it)
	{
		it->second->pResource->Release();
	}
}

void CGraphicContext::Init()
{
	m_pSRVAllocator = new CDescriptorAlloctor(256);
	m_pUniformBufferAllocator = new CUniformBufferAllocator();
}

void CGraphicContext::ReleaseUploadBuffers()
{
	for (int i = 0; i < m_pUploladBuffers.size(); ++i)
	{
		m_pUploladBuffers[i]->Release();
		m_pUploladBuffers[i] = nullptr;
	}
	m_pUploladBuffers.clear();
}

// https://github.com/Microsoft/DirectXTex/wiki/CreateTexture
// https://github.com/Microsoft/DirectXTex/wiki/GenerateMipMaps
Texture2D* CGraphicContext::CreateTexture2DResourceFromFile(std::wstring imagePath, bool bGenerateMipmaps)
{
	Texture2D* pTextureResource = new Texture2D();
	ID3D12Resource* pUploadResource = nullptr;
	auto baseImage = std::make_unique<ScratchImage>();

	size_t offset = imagePath.find_last_of(L'.');
	std::wstring imageType = imagePath.substr(offset + 1);

	HRESULT hr;
	if (imageType == L"dds" || imageType == L"DDS")
	{
		hr = LoadFromDDSFile(imagePath.c_str(), 0, nullptr, *baseImage);
	}
	else if (imageType == L"tga" || imageType == L"TGA")
	{
		hr = LoadFromTGAFile(imagePath.c_str(), nullptr, *baseImage);
	}
	else if (imageType == L"exr" || imageType == L"EXR")
	{
		hr = LoadFromEXRFile(imagePath.c_str(), nullptr, *baseImage);
	}
	else if (imageType == L"hdr" || imageType == L"HDR")
	{
		hr = LoadFromHDRFile(imagePath.c_str(), nullptr, *baseImage);
	}
	else
	{
		hr = LoadFromWICFile(imagePath.c_str(), WIC_FLAGS_NONE, nullptr, *baseImage);
	}
	if (hr == S_OK)
	{
		auto mipmapImage = std::make_unique<ScratchImage>();
		if (bGenerateMipmaps)
		{
			hr = GenerateMipMaps(baseImage->GetImages(), baseImage->GetImageCount(), baseImage->GetMetadata(), TEX_FILTER_DEFAULT, 0, *mipmapImage);
		}
		else
		{
			mipmapImage = std::move(baseImage);
		}

		// 此时资源已经 转变为 D3D12_RESOURCE_STATE_COPY_DEST
		hr = CreateTexture(m_pDevice, mipmapImage->GetMetadata(), &pTextureResource->pResource);

		std::vector<D3D12_SUBRESOURCE_DATA> subresources;
		hr = PrepareUpload(m_pDevice, mipmapImage->GetImages(), mipmapImage->GetImageCount(), mipmapImage->GetMetadata(), subresources);

		const UINT64 uploadBufferSize = GetRequiredIntermediateSize(pTextureResource->pResource, 0, (UINT)subresources.size());

		hr = m_pDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&pUploadResource));

		UpdateSubresources(m_pCommandList, pTextureResource->pResource, pUploadResource, 0, 0, (UINT)subresources.size(), subresources.data());

		m_pCommandList->ResourceBarrier(
			1, &CD3DX12_RESOURCE_BARRIER::Transition(
				pTextureResource->pResource,
				D3D12_RESOURCE_STATE_COPY_DEST,
				D3D12_RESOURCE_STATE_GENERIC_READ));

		if (pUploadResource)
		{
			m_pUploladBuffers.push_back(pUploadResource);
		}
		return pTextureResource;
	}
	else
	{
		delete pTextureResource;
	}
	return nullptr;
}

Texture2D* CGraphicContext::CreateTexture2D(const std::string& path, bool bCreateSRV /*= true*/)
{
	auto it = m_Textures.find(path);
	if (it != m_Textures.end())
	{
		return it->second;
	}

	Texture2D* pTexture = CreateTexture2DResourceFromFile(StringToWString(path));
	if (pTexture)
	{
		if (bCreateSRV)
		{
			DescriptorAddress address = m_pSRVAllocator->Allocate(1, m_pDevice);
			pTexture->CPUHandle = address.CpuHandle;
			pTexture->GPUHandle = address.GpuHandle;
			CreateTexture2DSRV(pTexture->pResource, address.CpuHandle);
		}
		m_Textures.emplace(path, pTexture);
		return pTexture;
	}
	return nullptr;
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

ID3D12Resource* CGraphicContext::CreateDefaultBuffer(const void* initData, UINT64 byteSize)
{
	ID3D12Resource* pDefaultBuffer = nullptr;
	ID3D12Resource* pUploadBuffer = nullptr;

	m_pDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(byteSize),
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&pDefaultBuffer));

	m_pDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),// This heap type is best for CPU-write-once, GPU-read-once data
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(byteSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&pUploadBuffer));

	D3D12_SUBRESOURCE_DATA subResource = {};
	subResource.pData = initData;
	subResource.RowPitch = byteSize;
	subResource.SlicePitch = byteSize;

	m_pCommandList->ResourceBarrier(
		1, &CD3DX12_RESOURCE_BARRIER::Transition(
			pDefaultBuffer,
			D3D12_RESOURCE_STATE_COMMON,
			D3D12_RESOURCE_STATE_COPY_DEST));

	UpdateSubresources<1>(m_pCommandList, pDefaultBuffer, pUploadBuffer, 0, 0, 1, &subResource);

	m_pCommandList->ResourceBarrier(
		1, &CD3DX12_RESOURCE_BARRIER::Transition(
			pDefaultBuffer,
			D3D12_RESOURCE_STATE_COPY_DEST,
			D3D12_RESOURCE_STATE_GENERIC_READ));

	if (pUploadBuffer)
	{
		m_pUploladBuffers.push_back(pUploadBuffer);
	}

	return pDefaultBuffer;
}

D3D12_VERTEX_BUFFER_VIEW CGraphicContext::CreateVertexBufferView(ID3D12Resource * pVertexBuffer, UINT size, UINT stride)
{
	D3D12_VERTEX_BUFFER_VIEW bufferView;
	bufferView.BufferLocation = pVertexBuffer->GetGPUVirtualAddress();
	bufferView.SizeInBytes = size;
	bufferView.StrideInBytes = stride;
	return bufferView;
}

D3D12_INDEX_BUFFER_VIEW CGraphicContext::CreateIndexBufferView(ID3D12Resource * pIndexBuffer, UINT size, DXGI_FORMAT format)
{
	D3D12_INDEX_BUFFER_VIEW bufferView;
	bufferView.BufferLocation = pIndexBuffer->GetGPUVirtualAddress();
	bufferView.SizeInBytes = size;
	bufferView.Format = format;
	return bufferView;
}

ID3DBlob * CGraphicContext::CompileShader(const std::string& sFileName, const std::string& sEntrypoint, const std::string& sTarget, const D3D_SHADER_MACRO * pDefines)
{
	ID3DBlob* pShaderCode = nullptr;
	// Compile Shader
	UINT compileFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)
	compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	std::wstring wFilePath;
	StringToWString(sFileName, wFilePath);

	ID3DBlob* pErrorMsg = nullptr;
	HRESULT hr = D3DCompileFromFile(wFilePath.c_str(), pDefines, D3D_COMPILE_STANDARD_FILE_INCLUDE, sEntrypoint.c_str(), sTarget.c_str(), compileFlags, 0, &pShaderCode, &pErrorMsg);
	if (pErrorMsg)
	{
		std::cout << "ShaderCompileError: " << std::string((char*)pErrorMsg->GetBufferPointer()) << std::endl;
		return nullptr;
	}
	return pShaderCode;
}

CUniformBufferAllocator::CUniformBufferAllocator()
{
	m_nPerPageSize = 1024 * 1024; // 1M
	m_nCurrentOffset = m_nPerPageSize;
	m_pCurrentResource = nullptr;
}

CUniformBufferAllocator::~CUniformBufferAllocator()
{
	for (int i = 0; i < m_ResourcePool.size(); ++i)
	{
		m_ResourcePool[i]->Release();
	}
}

UniformBufferLocation* CUniformBufferAllocator::Allocate(UINT bufferSize, CGraphicContext* pContext)
{
	UINT nAlignedBufferSize = (bufferSize + 255) & (~255);

	if (nAlignedBufferSize > m_nPerPageSize)
	{
		return nullptr;
	}

	if (m_pCurrentResource == nullptr || m_nCurrentOffset + nAlignedBufferSize > m_nPerPageSize)
	{
		m_pCurrentResource = CreateNewPage(pContext->m_pDevice);
		if (m_pCurrentResource == nullptr)
		{
			std::cout << "Create Uniform Buffer Error.\n";
			return nullptr;
		}

		m_ResourcePool.push_back(m_pCurrentResource);
		m_pCurrentResource->Map(0, nullptr, (void**)(&m_pCurrentGPUVA));
		m_nCurrentOffset = 0;
	}

	// Descriptor
	UniformBufferLocation* loc = new UniformBufferLocation();
	DescriptorAddress address = pContext->m_pSRVAllocator->Allocate(1, pContext->m_pDevice);
	loc->size = nAlignedBufferSize;
	loc->pResource = m_pCurrentResource;
	loc->pData = m_pCurrentGPUVA + m_nCurrentOffset;
	loc->pBufferHeap = address.pHeap;
	loc->CPUHandle = address.CpuHandle;
	loc->GPUHandle = address.GpuHandle;

	// Create Const Buffer View
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
	cbvDesc.BufferLocation = m_pCurrentResource->GetGPUVirtualAddress() + m_nCurrentOffset;
	cbvDesc.SizeInBytes = nAlignedBufferSize;
	pContext->m_pDevice->CreateConstantBufferView(&cbvDesc, loc->CPUHandle);

	m_nCurrentOffset += nAlignedBufferSize;
	return loc;
}

void CUniformBufferAllocator::Deallocate()
{

}

ID3D12Resource* CUniformBufferAllocator::CreateNewPage(ID3D12Device* pDevice)
{
	if (pDevice)
	{
		ID3D12Resource* pPageResource = nullptr;
		pDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(m_nPerPageSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&pPageResource));

		return pPageResource;
	}
	return nullptr;
}
