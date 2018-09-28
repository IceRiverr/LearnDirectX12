#pragma once
#include "stdafx.h"
#include "Utility.h"
#include <unordered_map>
#include "GPUResource.h"

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
	D3D12_VERTEX_BUFFER_VIEW m_PositionBufferView;

	ID3D12Resource* m_pNormalBufferGPU = nullptr;
	ID3D12Resource* m_pNormalBufferUpload = nullptr;
	D3D12_VERTEX_BUFFER_VIEW m_NormalBufferView;

	ID3D12Resource* m_pUVBufferGPU = nullptr;
	ID3D12Resource* m_pUVBufferUpload = nullptr;
	D3D12_VERTEX_BUFFER_VIEW m_UVBufferView;

	ID3D12Resource* m_pVertexColorBufferGPU = nullptr;
	ID3D12Resource* m_pVertexColorBufferUpload = nullptr;
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

class CMaterial
{
public:
	BufferView m_MaterialBufferView;
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

	// 放在这里可能，不合适，因为一个Obj，可能有多个材质，应该归于不同的StaticMesh，应该进行拆分
	CMaterial* m_pMaterial;
	BufferView m_ObjectBufferView; // 只和当前物体有关的buffer，和材质无关的放出来
};
