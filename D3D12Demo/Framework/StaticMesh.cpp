#include "StaticMesh.h"
#include "ImportObj.h"
#include "GraphicsUtility.h"


void CStaticMesh::AddSubMesh(std::string name, UINT nIndexCount, UINT nStartIndexLoc, INT nBaseVertexLoc)
{
	SubMesh subMesh;
	subMesh.nIndexCount = nIndexCount;
	subMesh.nStartIndexLocation = nStartIndexLoc;
	subMesh.nBaseVertexLocation = nBaseVertexLoc;
	m_SubMeshes.emplace(name, subMesh);
}

void CStaticMesh::ImportFromFile(std::string filePath, ID3D12Device* pDevice, ID3D12GraphicsCommandList* cmdList)
{
	if (pDevice && cmdList)
	{
		CImportor_Obj impoortor;
		impoortor.SetPath(filePath); // smooth_box plane  scene_simple

		if (impoortor.Import())
		{
			MeshData* pMeshData = impoortor.m_MeshObjs[0];

			if (pMeshData->Positions.size() > 0)
			{
				this->m_pPositionBufferGPU = Graphics::CreateDefaultBuffer(pDevice, cmdList, &pMeshData->Positions[0], pMeshData->Positions.size() * sizeof(XMFLOAT3), &this->m_pPositionBufferUpload);
				this->m_PositionBufferView = Graphics::CreateVertexBufferView(this->m_pPositionBufferGPU, (UINT)pMeshData->Positions.size() * sizeof(XMFLOAT3), sizeof(XMFLOAT3));
			}

			if (pMeshData->Normals.size() > 0)
			{
				this->m_pNormalBufferGPU = Graphics::CreateDefaultBuffer(pDevice, cmdList, &pMeshData->Normals[0], pMeshData->Normals.size() * sizeof(XMFLOAT3), &this->m_pNormalBufferUpload);
				this->m_NormalBufferView = Graphics::CreateVertexBufferView(this->m_pNormalBufferGPU, (UINT)pMeshData->Normals.size() * sizeof(XMFLOAT3), sizeof(XMFLOAT3));
			}

			if (pMeshData->UVs.size() > 0)
			{
				this->m_pUVBufferGPU = Graphics::CreateDefaultBuffer(pDevice, cmdList, &pMeshData->Normals[0], pMeshData->Normals.size() * sizeof(XMFLOAT3), &this->m_pUVBufferUpload);
				this->m_UVBufferView = Graphics::CreateVertexBufferView(this->m_pUVBufferGPU, (UINT)pMeshData->Normals.size() * sizeof(XMFLOAT3), sizeof(XMFLOAT3));
			}
			
			if (pMeshData->Indices.size() > 0)
			{
				this->m_pIndexBuferGPU = Graphics::CreateDefaultBuffer(pDevice, cmdList, &pMeshData->Indices[0], (UINT)pMeshData->Indices.size() * sizeof(UINT), &this->m_pIndexBufferUpload);
				this->m_IndexBufferView = Graphics::CreateIndexBufferView(this->m_pIndexBuferGPU, (UINT)pMeshData->Indices.size() * sizeof(UINT), DXGI_FORMAT_R32_UINT);
			}

			this->AddSubMesh("Sub0", (UINT)pMeshData->Indices.size(), 0, 0);
		}
	}
}

CRenderObject::CRenderObject()
{
	m_pStaticMesh = nullptr;
	m_Transform.Position = XMFLOAT3(0.0f, 0.0f, 0.0f);
	m_Transform.Rotation = XMFLOAT3(0.0f, 0.0f, 0.0f);
	m_Transform.Scale = XMFLOAT3(1.0f, 1.0f, 1.0f);

	m_ObjectAddress = {};
	m_bTransformDirty = true;
}

void CRenderObject::Update()
{
	if (m_bTransformDirty)
	{
		XMMATRIX mScaleMat = XMMatrixScaling(m_Transform.Scale.x, m_Transform.Scale.y, m_Transform.Scale.z);
		XMMATRIX mTranslateMat = XMMatrixTranslation(m_Transform.Position.x, m_Transform.Position.y, m_Transform.Position.z);
		m_mWorldMatrix = mScaleMat * mTranslateMat;

		m_bTransformDirty = false;
	}
}

void CRenderObject::Render(ID3D12GraphicsCommandList * pCommandList)
{
	
}

ObjectShaderBlock CRenderObject::CreateShaderBlock() const
{
	ObjectShaderBlock shaderBlock;
	XMStoreFloat4x4(&shaderBlock.mWorldMat, XMMatrixTranspose(m_mWorldMatrix));

	return shaderBlock;
}
