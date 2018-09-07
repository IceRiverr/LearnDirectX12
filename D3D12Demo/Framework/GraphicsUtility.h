#pragma once
#include "stdafx.h"
#include "StaticMesh.h"

namespace Graphics
{
	ID3D12Resource* CreateDefaultBuffer(ID3D12Device* pDevice, ID3D12GraphicsCommandList* cmdList, const void* initData, UINT64 byteSize, ID3D12Resource** ppUploadBuffer);

	D3D12_VERTEX_BUFFER_VIEW CreateVertexBufferView(ID3D12Resource* pVertexBuffer, UINT size, UINT stride);

	D3D12_INDEX_BUFFER_VIEW CreateIndexBufferView(ID3D12Resource* pIndexBuffer, UINT size, DXGI_FORMAT format);

	// �ֱ�����ҪС�� 128 x 64
	void CreateUVSphereMesh(int segments, int rings, std::vector<XMFLOAT3>& positions, std::vector<UINT16>& indees);
}