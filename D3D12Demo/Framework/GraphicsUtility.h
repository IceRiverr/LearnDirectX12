#pragma once
#include "stdafx.h"
#include "StaticMesh.h"
#include "GPUResource.h"

namespace Graphics
{
	ID3D12Resource* CreateDefaultBuffer(ID3D12Device* pDevice, ID3D12GraphicsCommandList* cmdList, const void* initData, UINT64 byteSize, ID3D12Resource** ppUploadBuffer);

	Texture2DResource* CreateTexture2DResourceFromFile(ID3D12Device* pDevice, ID3D12GraphicsCommandList* cmdList, std::wstring tgaPath);

	D3D12_VERTEX_BUFFER_VIEW CreateVertexBufferView(ID3D12Resource* pVertexBuffer, UINT size, UINT stride);

	D3D12_INDEX_BUFFER_VIEW CreateIndexBufferView(ID3D12Resource* pIndexBuffer, UINT size, DXGI_FORMAT format);

	ID3DBlob* CompileShader(std::string sFileName, std::string sEntrypoint, std::string sTarget, const D3D_SHADER_MACRO* pDefines = nullptr);

	// 分辨率需要小于 128 x 64
	void CreateUVSphereMesh(int segments, int rings, std::vector<XMFLOAT3>& positions, std::vector<UINT16>& indees);

	void CreateBox(std::vector<XMFLOAT3>& positions, std::vector<UINT16>& indices);

	UINT CalcConstBufferBytersAligned(UINT rawBytes);
}
