#pragma once
#include "stdafx.h"

#include "Utility.h"
#include "GraphicsUtility.h"
#include <unordered_map>

using namespace DirectX;
using namespace Graphics;

struct SubMesh
{
	UINT nIndexCount;
	UINT nStartIndexLocation;
	INT nBaseVertexLocation;
};

class CStaticMesh
{
public:
	D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView();
	D3D12_INDEX_BUFFER_VIEW GetIndexBufferView();

	void AddSubMesh(std::string name, UINT nIndexCount, UINT nStartIndexLoc, INT nBaseVertexLoc);

public:
	ID3D12Resource* m_pVertexBufferGPU = nullptr;
	ID3D12Resource* m_pVertexBufferUpload = nullptr;
	ID3D12Resource* m_pIndexBuferGPU = nullptr;
	ID3D12Resource* m_pIndexBufferUpload = nullptr;

	UINT m_nVertexSizeInBytes = 0;
	UINT m_nVertexStrideInBytes = 0;

	UINT m_nIndexSizeInBytes = 0;
	DXGI_FORMAT m_IndexFormat = DXGI_FORMAT_R16_UINT;
	
	std::unordered_map<std::string, SubMesh> m_SubMeshes;
};

struct TransformData
{
	XMFLOAT3 Position;
	XMFLOAT3 Rotation;
	XMFLOAT3 Scale;
};

class CRenderObject
{
public:
	struct ConstantElement
	{
		XMFLOAT4X4 mWorldMat;
	};

	CRenderObject();
	void Render(ID3D12GraphicsCommandList* pCommandList);
	
	static UINT GetConstantElementSizeAligned();
public:
	CStaticMesh* m_pStaticMesh;
	TransformData m_WorldTransform;
	XMMATRIX m_mWorldMatrix;

	UINT m_nConstantBufferIndex;
};
