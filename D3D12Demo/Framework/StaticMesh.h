#pragma once
#include "stdafx.h"
#include "Utility.h"
#include <unordered_map>

using namespace DirectX;

struct MeshData
{
	std::vector<XMFLOAT3> Positions;
	std::vector<XMFLOAT2> UVs;
	std::vector<XMFLOAT3> Normals;
	std::vector<XMFLOAT4> VtxColors;
	std::vector<UINT> Indices;
};

struct SubMesh
{
	UINT nIndexCount;
	UINT nStartIndexLocation;
	INT nBaseVertexLocation;
};

class CStaticMesh
{
public:
	void AddSubMesh(std::string name, UINT nIndexCount, UINT nStartIndexLoc, INT nBaseVertexLoc);

public:
	ID3D12Resource* m_pPositionBufferGPU = nullptr;
	ID3D12Resource* m_pPositionBufferUpload = nullptr;
	ID3D12Resource* m_pVertexColorBufferGPU = nullptr;
	ID3D12Resource* m_pVertexColorBufferUpload = nullptr;
	D3D12_VERTEX_BUFFER_VIEW m_PositionBufferView;
	D3D12_VERTEX_BUFFER_VIEW m_VertexColorBufferView;

	ID3D12Resource* m_pIndexBuferGPU = nullptr;
	ID3D12Resource* m_pIndexBufferUpload = nullptr;
	D3D12_INDEX_BUFFER_VIEW m_IndexBufferView;

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
