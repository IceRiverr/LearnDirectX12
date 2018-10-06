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
				this->m_pTangentBufferGPU = Graphics::CreateDefaultBuffer(pDevice, cmdList, &pMeshData->Tangents[0], pMeshData->Tangents.size() * sizeof(XMFLOAT4), &this->m_pTangentBufferUpload);
				this->m_TangentBufferView = Graphics::CreateVertexBufferView(this->m_pTangentBufferGPU, (UINT)pMeshData->Tangents.size() * sizeof(XMFLOAT4), sizeof(XMFLOAT4));
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

		std::vector<XMFLOAT3> Tangents; 
		Tangents.resize(mesh.Positions.size());

		std::vector<XMFLOAT3> BiNormals;
		BiNormals.resize(mesh.Positions.size());

		for (int i = 0; i < mesh.Indices.size(); i += 3)
		{
			int i0 = mesh.Indices[i];
			int i1 = mesh.Indices[i + 1];
			int i2 = mesh.Indices[i + 2];

			XMFLOAT3 p0 = mesh.Positions[i0];
			XMFLOAT3 p1 = mesh.Positions[i1];
			XMFLOAT3 p2 = mesh.Positions[i2];

			XMFLOAT2 uv0 = mesh.UVs[i0];
			XMFLOAT2 uv1 = mesh.UVs[i1];
			XMFLOAT2 uv2 = mesh.UVs[i2];

			float x1 = p1.x - p0.x;
			float y1 = p1.y - p0.y;
			float z1 = p1.z - p0.z;
			float x2 = p2.x - p0.x;
			float y2 = p2.y - p0.y;
			float z2 = p2.z - p0.z;

			float s1 = uv1.x - uv0.x;
			float t1 = uv1.y - uv0.y;
			float s2 = uv2.x - uv0.x;
			float t2 = uv2.y - uv0.y;

			float r = 1.0f / (s1 * t2 - s2 * t1);

			XMFLOAT3 T = XMFLOAT3(r * (t2 * x1 - t1 *x2), r * (t2 * y1 - t1 * y2),	r * (t2 * z1 - t1 * z2));
			XMFLOAT3 B = XMFLOAT3(r * (-s2 * x1 + s1 *x2), r * (-s2 * y1 + s1 * y2), r * (-s2 * z1 + s1 * z2));

			Tangents[i0] = MathLib::XMFloat3Add(Tangents[i0], T);
			Tangents[i1] = MathLib::XMFloat3Add(Tangents[i1], T);
			Tangents[i2] = MathLib::XMFloat3Add(Tangents[i2], T);

			BiNormals[i0] = MathLib::XMFloat3Add(BiNormals[i0], B);
			BiNormals[i1] = MathLib::XMFloat3Add(BiNormals[i1], B);
			BiNormals[i2] = MathLib::XMFloat3Add(BiNormals[i2], B);
		}

		for (int i = 0; i < mesh.Tangents.size(); ++i)
		{
			XMVECTOR N = XMLoadFloat3(&mesh.Normals[i]);
			XMVECTOR T = XMLoadFloat3(&Tangents[i]);
			XMVECTOR B = XMLoadFloat3(&BiNormals[i]);
			
			// T = normalize(T - dot(T, N) * N);
			// 举证做一个初等变换，相当于normalize，随意不会对矩阵的乘积产生影响
			T = XMVectorSubtract(T, XMVector3Dot(T, N) * N);
			T = XMVector3Normalize(T);

			XMVECTOR H = XMVector3Dot(XMVector3Cross(N, T), B);
			float h;
			XMStoreFloat(&h, H);
			float handness = h > 0.0f ? 1.0f : -1.0f;

			XMFLOAT4 mT;
			XMStoreFloat4(&mT, T);
			mT.w = handness;

			mesh.Tangents[i] = mT;
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
	pCommandList->SetPipelineState(m_pStaticMesh->m_pMaterial->m_pPSO);
	
	// Bind Mesh
	if (m_pStaticMesh->m_pMaterial->m_InputLayoutType == INPUT_LAYOUT_TYPE::P3N3)
	{
		D3D12_VERTEX_BUFFER_VIEW VertexBufferViews[] =
		{
			m_pStaticMesh->m_PositionBufferView,
			m_pStaticMesh->m_NormalBufferView,
		};

		pCommandList->IASetVertexBuffers(0, 2, &VertexBufferViews[0]);
	}
	else if (m_pStaticMesh->m_pMaterial->m_InputLayoutType == INPUT_LAYOUT_TYPE::P3N3T4UV2)
	{
		D3D12_VERTEX_BUFFER_VIEW VertexBufferViews[] =
		{
			m_pStaticMesh->m_PositionBufferView,
			m_pStaticMesh->m_NormalBufferView,
			m_pStaticMesh->m_TangentBufferView,
			m_pStaticMesh->m_UVBufferView,
		};

		pCommandList->IASetVertexBuffers(0, 4, &VertexBufferViews[0]);
	}

	pCommandList->IASetIndexBuffer(&m_pStaticMesh->m_IndexBufferView);
	pCommandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	
	auto handle1 = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_ObjectAddress.pBufferHeap->GetGPUDescriptorHandleForHeapStart());
	handle1.Offset(m_ObjectAddress.nHeapOffset, m_ObjectAddress.nSRVDescriptorSize);
	pCommandList->SetGraphicsRootDescriptorTable(ROOT_SIGNATURE_INDEX::OBJECT_BUFFER_INDEX, handle1);

	// Bind Material
	auto handle2 = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_pStaticMesh->m_pMaterial->m_MaterialAddress.pBufferHeap->GetGPUDescriptorHandleForHeapStart());
	handle2.Offset(m_pStaticMesh->m_pMaterial->m_MaterialAddress.nHeapOffset, m_pStaticMesh->m_pMaterial->m_MaterialAddress.nSRVDescriptorSize);
	pCommandList->SetGraphicsRootDescriptorTable(ROOT_SIGNATURE_INDEX::MATERIAL_BUFFER_INDEX, handle2);

	if (m_pStaticMesh->m_pMaterial->m_pAldeboMap)
	{
		auto handle = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_pStaticMesh->m_pMaterial->m_pAldeboMap->m_TextureAddress.pBufferHeap->GetGPUDescriptorHandleForHeapStart());
		handle.Offset(m_pStaticMesh->m_pMaterial->m_pAldeboMap->m_TextureAddress.nHeapOffset, m_pStaticMesh->m_pMaterial->m_pAldeboMap->m_TextureAddress.nSRVDescriptorSize);
		pCommandList->SetGraphicsRootDescriptorTable(ROOT_SIGNATURE_INDEX::MATERIAL_TEXTURE_INDEX, handle);
	}

	// Draw
	for (auto it : m_pStaticMesh->m_SubMeshes)
	{
		SubMesh subMesh = it.second;
		pCommandList->DrawIndexedInstanced(subMesh.nIndexCount, 1, subMesh.nStartIndexLocation, subMesh.nBaseVertexLocation, 0);
	}
}

ObjectShaderBlock CRenderObject::CreateShaderBlock() const
{
	ObjectShaderBlock shaderBlock;

	XMMATRIX InvWOrldMat = XMMatrixInverse(&XMMatrixDeterminant(m_mWorldMatrix), m_mWorldMatrix);
	XMStoreFloat4x4(&shaderBlock.mWorldMat, XMMatrixTranspose(m_mWorldMatrix));
	XMStoreFloat4x4(&shaderBlock.mInvWorldMat, XMMatrixTranspose(InvWOrldMat));

	return shaderBlock;
}
