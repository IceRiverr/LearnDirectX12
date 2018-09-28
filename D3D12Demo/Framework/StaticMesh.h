#pragma once
#include "stdafx.h"
#include "Utility.h"
#include <unordered_map>
#include "GPUResource.h"

using namespace DirectX;

struct ConstantBufferAddress
{
	ID3D12Resource* pBuffer;
	UINT nBufferIndex;
	ID3D12DescriptorHeap* pBufferHeap;
	UINT nHeapOffset;
};

struct MaterialShaderBlock
{
	XMFLOAT4 DiffuseColor;
};

struct ConstantShaderBlock
{
	XMFLOAT4X4 mWorldMat;
};

class CMaterial
{
public:
	std::string m_sName;
	XMFLOAT4 m_cDiffuseColor;

	ConstantBufferAddress m_MaterialAddress;
};

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
	void Render(ID3D12GraphicsCommandList* pCommandList);
	
public:
	CStaticMesh* m_pStaticMesh;
	TransformData m_WorldTransform;
	XMMATRIX m_mWorldMatrix;
	
	ConstantBufferAddress m_ObjectAddress;
};
