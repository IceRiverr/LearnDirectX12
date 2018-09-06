#include "GraphicsUtility.h"
#include "Utility.h"

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

void Graphics::CreateUVSphereMesh(int segments, int rings, std::vector<XMFLOAT3>& positions, std::vector<UINT16>& indees)
{
	if (segments >= 3 && rings >= 3)
	{
		for (int i = 1; i < rings; ++i)
		{
			float phi = PI * i / rings;
			for (int j = 0; j < segments; ++j)
			{
				float theta = PI_2 * j / segments;
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
		for (int j = 0; j < segments - 1; ++j)
		{
			indees.push_back(top);
			indees.push_back(j);
			indees.push_back(j + 1);
		}
		indees.push_back(top);
		indees.push_back(segments);
		indees.push_back(0);

		int bottom = (int)positions.size() - 1;
		int bottomUp = segments * (rings - 2);
		for (int j = 0; j < segments - 1; ++j)
		{
			indees.push_back(bottom);
			indees.push_back(bottomUp + j + 1);
			indees.push_back(bottomUp + j);
		}
	}
}

bool Graphics::ImportObjMesh(std::string path, MeshData & meshData)
{
	return true;
}
