#include "GraphicsUtility.h"
#include "Utility.h"
#include "DirectXTex.h"
#include "DirectXTexExr.h"

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

Texture2DResource* Graphics::CreateTexture2DResourceFromFile(
	ID3D12Device * pDevice, 
	ID3D12GraphicsCommandList * cmdList, 
	std::wstring imagePath, 
	bool bGenerateMipmaps)
{
	if (pDevice && cmdList)
	{
		Texture2DResource* pTextureResource = new Texture2DResource();

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
			hr = LoadFromWICFile(imagePath.c_str(), WIC_FLAGS_NONE,nullptr, *baseImage);
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

			hr = CreateTexture(pDevice, mipmapImage->GetMetadata(), &pTextureResource->pTexture);

			std::vector<D3D12_SUBRESOURCE_DATA> subresources;
			hr = PrepareUpload(pDevice, mipmapImage->GetImages(), mipmapImage->GetImageCount(), mipmapImage->GetMetadata(), subresources);

			const UINT64 uploadBufferSize = GetRequiredIntermediateSize(pTextureResource->pTexture, 0, (UINT)subresources.size());

			hr = pDevice->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
				D3D12_HEAP_FLAG_NONE,
				&CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&pTextureResource->pUploadBuffer));

			UpdateSubresources(cmdList, pTextureResource->pTexture, pTextureResource->pUploadBuffer, 0, 0, (UINT)subresources.size(), subresources.data());
			return pTextureResource;
		}
		else
		{
			delete pTextureResource;
		}
	}
	return nullptr;
}

D3D12_VERTEX_BUFFER_VIEW Graphics::CreateVertexBufferView(ID3D12Resource * pVertexBuffer, UINT size, UINT stride)
{
	D3D12_VERTEX_BUFFER_VIEW bufferView;
	bufferView.BufferLocation = pVertexBuffer->GetGPUVirtualAddress();
	bufferView.SizeInBytes = size;
	bufferView.StrideInBytes = stride;
	return bufferView;
}

D3D12_INDEX_BUFFER_VIEW Graphics::CreateIndexBufferView(ID3D12Resource * pIndexBuffer, UINT size, DXGI_FORMAT format)
{
	D3D12_INDEX_BUFFER_VIEW bufferView;
	bufferView.BufferLocation = pIndexBuffer->GetGPUVirtualAddress();
	bufferView.SizeInBytes = size;
	bufferView.Format = format;
	return bufferView;
}

ID3DBlob* Graphics::CompileShader(std::string sFileName, std::string sEntrypoint, std::string sTarget, const D3D_SHADER_MACRO* pDefines)
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
	D3DCompileFromFile(wFilePath.c_str(), pDefines, D3D_COMPILE_STANDARD_FILE_INCLUDE, sEntrypoint.c_str(), sTarget.c_str(), compileFlags, 0, &pShaderCode, &pErrorMsg);
	if (pErrorMsg)
	{
		std::cout << "ShaderCompileError: " << std::string((char*)pErrorMsg->GetBufferPointer()) << std::endl;
		return nullptr;
	}
	return pShaderCode;
}

void Graphics::CreateUVSphereMesh(int segments, int rings, std::vector<XMFLOAT3>& positions, std::vector<UINT16>& indees)
{
	if (segments >= 3 && rings >= 3)
	{
		for (int i = 1; i < rings; ++i)
		{
			float phi = XM_PI * i / rings;
			for (int j = 0; j < segments; ++j)
			{
				float theta = XM_2PI * j / segments;
				XMFLOAT3 p;
				p.x = std::sinf(phi) * std::cosf(theta);
				p.y = std::cosf(phi);
				p.z = std::sinf(phi) * std::sinf(theta);
				positions.push_back(p);
			}
		}
		positions.push_back(XMFLOAT3(0.0f, 1.0f, 0.0f));
		positions.push_back(XMFLOAT3(0.0f, -1.0f, 0.0f));

		for (int i = 1; i < rings - 1; ++i)
		{
			for (int j = 0; j < segments - 1; ++j)
			{
				int p00 = (i - 1) * segments + j;		int p01 = (i - 1) * segments + j + 1;
				int p10 = (i - 1 + 1) * segments + j;	int p11 = (i - 1 + 1) * segments + j + 1;

				indees.push_back(p00);
				indees.push_back(p10);
				indees.push_back(p11);

				indees.push_back(p00);
				indees.push_back(p11);
				indees.push_back(p01);
			}

			int p00 = (i - 1) * segments + segments - 1;		int p01 = (i - 1) * segments;
			int p10 = (i - 1 + 1) * segments + segments - 1;	int p11 = (i - 1 + 1) * segments ;

			indees.push_back(p00);
			indees.push_back(p10);
			indees.push_back(p11);

			indees.push_back(p00);
			indees.push_back(p11);
			indees.push_back(p01);
		}

		int top = (int)positions.size() - 2;
		for (int j = 0; j < segments; ++j)
		{
			indees.push_back(top);
			indees.push_back(j);

			if (j == segments - 1)
			{
				indees.push_back(0);
			}
			else
			{
				indees.push_back(j + 1);
			}
		}
		
		int bottom = (int)positions.size() - 1;
		int bottomUp = segments * (rings - 2);
		for (int j = 0; j < segments; ++j)
		{
			indees.push_back(bottom);
			indees.push_back(bottomUp + j);
			if (j == segments - 1)
			{
				indees.push_back(bottomUp);
			}
			else
			{
				indees.push_back(bottomUp + j + 1);
			}
		}
	}
}

void Graphics::CreateUVSphereMesh(int segments, int rings, MeshData & mesh)
{
	if (segments >= 3 && rings >= 3)
	{
		for (int i = 1; i < rings; ++i)
		{
			float phi = XM_PI * i / rings;
			for (int j = 0; j < segments; ++j)
			{
				float theta = XM_2PI * j / segments;
				XMFLOAT3 p;
				p.x = std::sinf(phi) * std::cosf(theta);
				p.y = std::cosf(phi);
				p.z = std::sinf(phi) * std::sinf(theta);
				mesh.Positions.push_back(p);
			}
		}
		mesh.Positions.push_back(XMFLOAT3(0.0f, 1.0f, 0.0f));
		mesh.Positions.push_back(XMFLOAT3(0.0f, -1.0f, 0.0f));

		for (int i = 1; i < rings - 1; ++i)
		{
			for (int j = 0; j < segments - 1; ++j)
			{
				int p00 = (i - 1) * segments + j;		int p01 = (i - 1) * segments + j + 1;
				int p10 = (i - 1 + 1) * segments + j;	int p11 = (i - 1 + 1) * segments + j + 1;
				
				mesh.Indices.push_back(p00);
				mesh.Indices.push_back(p10);
				mesh.Indices.push_back(p11);

				mesh.Indices.push_back(p00);
				mesh.Indices.push_back(p11);
				mesh.Indices.push_back(p01);
			}

			int p00 = (i - 1) * segments + segments - 1;		int p01 = (i - 1) * segments;
			int p10 = (i - 1 + 1) * segments + segments - 1;	int p11 = (i - 1 + 1) * segments;

			mesh.Indices.push_back(p00);
			mesh.Indices.push_back(p10);
			mesh.Indices.push_back(p11);

			mesh.Indices.push_back(p00);
			mesh.Indices.push_back(p11);
			mesh.Indices.push_back(p01);
		}

		int top = (int)mesh.Positions.size() - 2;
		for (int j = 0; j < segments - 1; ++j)
		{
			mesh.Indices.push_back(top);
			mesh.Indices.push_back(j);
			mesh.Indices.push_back(j + 1);
		}
		mesh.Indices.push_back(top);
		mesh.Indices.push_back(segments);
		mesh.Indices.push_back(0);

		int bottom = (int)mesh.Positions.size() - 1;
		int bottomUp = segments * (rings - 2);
		for (int j = 0; j < segments - 1; ++j)
		{
			mesh.Indices.push_back(bottom);
			mesh.Indices.push_back(bottomUp + j + 1);
			mesh.Indices.push_back(bottomUp + j);
		}
	}
}

void Graphics::CreateBox(std::vector<XMFLOAT3>& positions, std::vector<UINT16>& indices)
{
	positions =
	{
		XMFLOAT3(-1.0f, -1.0f, -1.0f),
		XMFLOAT3(-1.0f, +1.0f, -1.0f),
		XMFLOAT3(+1.0f, +1.0f, -1.0f),
		XMFLOAT3(+1.0f, -1.0f, -1.0f),
		XMFLOAT3(-1.0f, -1.0f, +1.0f),
		XMFLOAT3(-1.0f, +1.0f, +1.0f),
		XMFLOAT3(+1.0f, +1.0f, +1.0f),
		XMFLOAT3(+1.0f, -1.0f, +1.0f)
	};

	indices =
	{
		// front face
		0, 1, 2,
		0, 2, 3,

		// back face
		4, 6, 5,
		4, 7, 6,

		// left face
		4, 5, 1,
		4, 1, 0,

		// right face
		3, 2, 6,
		3, 6, 7,

		// top face
		1, 5, 6,
		1, 6, 2,

		// bottom face
		4, 0, 3,
		4, 3, 7
	};
}

UINT Graphics::CalcConstBufferBytersAligned(UINT rawBytes)
{
	return ((rawBytes + 255) & (~255));
}
