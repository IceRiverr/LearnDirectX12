#pragma once
#include "stdafx.h"
#include "Utility.h"
#include <unordered_map>
#include "GPUResource.h"
#include "Material.h"
#include "GraphicContext.h"

using namespace DirectX;

struct ObjectShaderBlock
{
	XMFLOAT4X4 mWorldMat;
	XMFLOAT4X4 mInvWorldMat;
};

struct MeshData
{
	std::vector<XMFLOAT3> Positions;
	std::vector<XMFLOAT2> UVs;
	std::vector<XMFLOAT3> Normals;
	std::vector<XMFLOAT4> Tangents;
	std::vector<XMFLOAT4> VtxColors;
	std::vector<UINT> Indices;
};

namespace Mesh
{
	void CalcTangents(MeshData& mesh); // TBN 切线空间是 右手坐标系
}

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
	void CreateBuffer(MeshData* pMeshData, CGraphicContext& Context);

public:
	ID3D12Resource* m_pPositionBufferGPU = nullptr;
	D3D12_VERTEX_BUFFER_VIEW m_PositionBufferView;

	ID3D12Resource* m_pNormalBufferGPU = nullptr;
	D3D12_VERTEX_BUFFER_VIEW m_NormalBufferView;

	ID3D12Resource* m_pTangentBufferGPU = nullptr;
	D3D12_VERTEX_BUFFER_VIEW m_TangentBufferView;

	ID3D12Resource* m_pUVBufferGPU = nullptr;
	D3D12_VERTEX_BUFFER_VIEW m_UVBufferView;

	ID3D12Resource* m_pVertexColorBufferGPU = nullptr;
	D3D12_VERTEX_BUFFER_VIEW m_VertexColorBufferView;

	ID3D12Resource* m_pIndexBuferGPU = nullptr;
	D3D12_INDEX_BUFFER_VIEW m_IndexBufferView;

	std::unordered_map<std::string, SubMesh> m_SubMeshes;

	CMaterial* m_pMaterial;
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
	CRenderObject();
	void Update();
	void Draw(ID3D12GraphicsCommandList* pCommandList);

	ObjectShaderBlock CreateShaderBlock() const;
	
public:
	CStaticMesh* m_pStaticMesh;
	TransformData m_Transform;
	XMMATRIX m_mWorldMatrix;
	
	ConstantBufferAddress m_ObjectAddress;
	bool m_bTransformDirty;
};
