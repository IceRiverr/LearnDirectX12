#include "StaticMesh.h"
#include "ImportObj.h"
#include "GraphicsUtility.h"
#include "MathLib.h"


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
			CalcTangents(*pMeshData);

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

			if (pMeshData->Tangents.size() > 0)
			{
				this->m_pTangentBufferGPU = Graphics::CreateDefaultBuffer(pDevice, cmdList, &pMeshData->Tangents[0], pMeshData->Tangents.size() * sizeof(XMFLOAT3), &this->m_pNormalBufferUpload);
				this->m_TangentBufferView = Graphics::CreateVertexBufferView(this->m_pTangentBufferGPU, (UINT)pMeshData->Tangents.size() * sizeof(XMFLOAT3), sizeof(XMFLOAT3));
			}

			if (pMeshData->UVs.size() > 0)
			{
				this->m_pUVBufferGPU = Graphics::CreateDefaultBuffer(pDevice, cmdList, &pMeshData->UVs[0], pMeshData->UVs.size() * sizeof(XMFLOAT2), &this->m_pUVBufferUpload);
				this->m_UVBufferView = Graphics::CreateVertexBufferView(this->m_pUVBufferGPU, (UINT)pMeshData->UVs.size() * sizeof(XMFLOAT2), sizeof(XMFLOAT2));
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

void CStaticMesh::CalcTangents(MeshData& mesh)
{
	if (mesh.Positions.size() == mesh.UVs.size())
	{
		mesh.Tangents.resize(mesh.Positions.size());
		for (int i = 0; i < mesh.Indices.size(); i += 3)
		{
			XMFLOAT3 T;

			int index0 = mesh.Indices[i];
			int index1 = mesh.Indices[i + 1];
			int index2 = mesh.Indices[i + 2];

			XMFLOAT3 p0 = mesh.Positions[index0];
			XMFLOAT3 p1 = mesh.Positions[index1];
			XMFLOAT3 p2 = mesh.Positions[index2];

			XMFLOAT2 uv0 = mesh.UVs[index0];
			XMFLOAT2 uv1 = mesh.UVs[index1];
			XMFLOAT2 uv2 = mesh.UVs[index2];

			XMFLOAT3 edge1 = MathLib::XMFloat3Subtract(p1, p0);
			XMFLOAT3 edge2 = MathLib::XMFloat3Subtract(p2, p0);

			XMFLOAT2 deltaUV1 = MathLib::XMFloat2Subtract(uv1, uv0);
			XMFLOAT2 deltaUV2 = MathLib::XMFloat2Subtract(uv2, uv0);

			float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

			T.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
			T.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
			T.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

			float length = std::sqrtf(T.x * T.x + T.y * T.y + T.z * T.z);
			T.x /= length;
			T.y /= length;
			T.z /= length;

			mesh.Tangents[index0] = MathLib::XMFloat3Add(mesh.Tangents[index0], T);
			mesh.Tangents[index1] = MathLib::XMFloat3Add(mesh.Tangents[index1], T);
			mesh.Tangents[index2] = MathLib::XMFloat3Add(mesh.Tangents[index2], T);
		}

		for (int i = 0; i < mesh.Tangents.size(); ++i)
		{
			XMFLOAT3& T = mesh.Tangents[i];
			float length = std::sqrtf(T.x * T.x + T.y * T.y + T.z * T.z);
			T.x /= length;
			T.y /= length;
			T.z /= length;
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
