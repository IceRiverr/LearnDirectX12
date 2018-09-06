#include "StaticMesh.h"

void CStaticMesh::AddSubMesh(std::string name, UINT nIndexCount, UINT nStartIndexLoc, INT nBaseVertexLoc)
{
	SubMesh subMesh;
	subMesh.nIndexCount = nIndexCount;
	subMesh.nStartIndexLocation = nStartIndexLoc;
	subMesh.nBaseVertexLocation = nBaseVertexLoc;
	m_SubMeshes.emplace(name, subMesh);
}

CRenderObject::CRenderObject()
{
	m_pStaticMesh = nullptr;
	m_WorldTransform.Position = XMFLOAT3(0.0f, 0.0f, 0.0f);
	m_WorldTransform.Rotation = XMFLOAT3(0.0f, 0.0f, 0.0f);
	m_WorldTransform.Scale = XMFLOAT3(1.0f, 1.0f, 1.0f);

	m_nConstantBufferIndex = -1;
}

void CRenderObject::Render(ID3D12GraphicsCommandList * pCommandList)
{
	
}

UINT CRenderObject::GetConstantElementSizeAligned()
{
	// create const buffer ±ØÐë256b¶ÔÆë
	UINT nSizeAligned = (sizeof(ConstantElement) + 255) & (~255);
	return nSizeAligned;
}
