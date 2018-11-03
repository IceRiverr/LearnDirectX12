
#include "PBRMaterial.h"
#include <d3d12shader.h>
#include <fstream>
#include <sstream>

PBRShaderDataBlock::PBRShaderDataBlock()
{
	UniformBufferLoc = nullptr;
	Data = nullptr;
	Size = 0;
}

void PBRShaderDataBlock::InitData(UINT size)
{
	Size = size;
	Data = new UINT8[size];
	memset(Data, 0, size);
}

void PBRShaderDataBlock::ReleaseData()
{
	if (Data)
	{
		delete[] Data;
	}
}

void PBRShaderDataBlock::CopyToCPU(UINT offset, UINT size, void * v)
{
	memcpy(Data + offset, v, size);
}

void PBRShaderDataBlock::CopyToGPU()
{
	if (UniformBufferLoc && Data && Size)
	{
		memcpy(UniformBufferLoc->pData, Data, Size);
	}
}


CShaderPipeline::CShaderPipeline()
{
	m_pVSShader = nullptr;
	m_pPSShader = nullptr;
	m_pPSO = nullptr;
}

void CShaderPipeline::InitParamMap()
{
	ShaderParamaterMap VSParamMap;
	ReflectShader(m_pVSShader, VSParamMap);

	ShaderParamaterMap PSParamMap;
	ReflectShader(m_pPSShader, PSParamMap);

	// Merge 有时候Name会出现不一致的情况，但是Size还是一致的，需要注意 bug，可能是复制的时候引起的
	m_ParamaterMap = VSParamMap;
	{
		for (auto it = PSParamMap.BoundResourceMap.begin(); it != PSParamMap.BoundResourceMap.end(); ++it)
		{
			if (m_ParamaterMap.BoundResourceMap.find(it->first) == m_ParamaterMap.BoundResourceMap.end())
			{
				m_ParamaterMap.BoundResourceMap.emplace(it->first, it->second);
			}
		}
	}
	{
		for (auto it = PSParamMap.ConstantBufferMap.begin(); it != PSParamMap.ConstantBufferMap.end(); ++it)
		{
			if (m_ParamaterMap.ConstantBufferMap.find(it->first) == m_ParamaterMap.ConstantBufferMap.end())
			{
				m_ParamaterMap.ConstantBufferMap.emplace(it->first, it->second);
			}
		}
	}
	{
		for (auto it = PSParamMap.ShaderValiableMap.begin(); it != PSParamMap.ShaderValiableMap.end(); ++it)
		{
			if (m_ParamaterMap.ShaderValiableMap.find(it->first) == m_ParamaterMap.ShaderValiableMap.end())
			{
				m_ParamaterMap.ShaderValiableMap.emplace(it->first, it->second);
			}
		}
	}
}

void CShaderPipeline::CreatePSO(ID3D12RootSignature* pRootSignature, CGraphicContext* pContext)
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC OpaquePSODesc = {};

	auto InputLayout = GetInputLayout(m_InputLayout);
	OpaquePSODesc.InputLayout = { InputLayout.data(), (UINT)InputLayout.size() };
	OpaquePSODesc.pRootSignature = pRootSignature;
	OpaquePSODesc.VS = { m_pVSShader->GetBufferPointer(),m_pVSShader->GetBufferSize() };
	OpaquePSODesc.PS = { m_pPSShader->GetBufferPointer(),m_pPSShader->GetBufferSize() };
	OpaquePSODesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	OpaquePSODesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	OpaquePSODesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	OpaquePSODesc.SampleMask = UINT_MAX;
	OpaquePSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	OpaquePSODesc.NumRenderTargets = 1;
	OpaquePSODesc.RTVFormats[0] = pContext->m_BackBufferFromat;
	OpaquePSODesc.SampleDesc.Count = 1;
	OpaquePSODesc.SampleDesc.Quality = 0;
	OpaquePSODesc.DSVFormat = pContext->m_DSVFormat;
	pContext->m_pDevice->CreateGraphicsPipelineState(&OpaquePSODesc, IID_PPV_ARGS(&m_pPSO));
}

void CShaderPipeline::ReflectShader(ID3DBlob* pShader, ShaderParamaterMap& ParamMap)
{
	if (pShader)
	{
		ID3D12ShaderReflection* pReflector = nullptr;
		D3DReflect(pShader->GetBufferPointer(), pShader->GetBufferSize(), IID_ID3D12ShaderReflection, (void**)&pReflector);
		if (pReflector)
		{
			D3D12_SHADER_DESC shaderDesc;
			pReflector->GetDesc(&shaderDesc);

			// For InputLayout
			for (UINT i = 0; i < shaderDesc.InputParameters; ++i)
			{
				D3D12_SIGNATURE_PARAMETER_DESC desc;
				pReflector->GetInputParameterDesc(i, &desc);
				ParamMap.InputParameters.push_back(desc);
			}

			for (UINT i = 0; i < shaderDesc.BoundResources; ++i)
			{
				D3D12_SHADER_INPUT_BIND_DESC inputBindDesc;
				pReflector->GetResourceBindingDesc(i, &inputBindDesc);
				ParamMap.BoundResourceMap.emplace(inputBindDesc.Name, inputBindDesc);
			}

			for (UINT i = 0; i < shaderDesc.ConstantBuffers; ++i)
			{
				ID3D12ShaderReflectionConstantBuffer* pRefCB = pReflector->GetConstantBufferByIndex(i);
				D3D12_SHADER_BUFFER_DESC bufferDesc;
				pRefCB->GetDesc(&bufferDesc);
				ParamMap.ConstantBufferMap.emplace(bufferDesc.Name, bufferDesc);

				for (UINT val = 0; val < bufferDesc.Variables; ++val)
				{
					ID3D12ShaderReflectionVariable* pRefVal = pRefCB->GetVariableByIndex(val);
					D3D12_SHADER_VARIABLE_DESC variableDesc;
					pRefVal->GetDesc(&variableDesc);
					ParamMap.ShaderValiableMap.emplace(variableDesc.Name, variableDesc);
				}
			}
			pReflector->Release();
		}
	}
}

void CPBRRenderEffect::SetShaderPath(const std::string path)
{
	m_sShaderPath = path;
}

void CPBRRenderEffect::SetRootSignature(ID3D12RootSignature * rootSignature)
{
	m_pRootSignature = rootSignature;
}

UINT64 CPBRRenderEffect::CompileShader(CGraphicContext* pContext, const PBRMaterialMacroInfo& info)
{
	if (pContext)
	{
		UINT64 MatID = CalcPBRMaterialID(info);
		std::vector<std::string> MacroDefines = CalcPBRMaterialMacro(MatID);

		std::vector<D3D_SHADER_MACRO> Macros;

		for (int i = 0; i < MacroDefines.size(); ++i)
		{
			Macros.push_back({ MacroDefines[i].c_str(), "1" });
		}
		Macros.push_back({ nullptr, nullptr });

		CShaderPipeline* pPipeline = new CShaderPipeline();
		pPipeline->m_pVSShader = pContext->CompileShader(m_sShaderPath, "VSMain", "vs_5_1", Macros.data());
		pPipeline->m_pPSShader = pContext->CompileShader(m_sShaderPath, "PSMain", "ps_5_1", Macros.data());
		pPipeline->m_InputLayout = CalcInputLayoutType(info);
		pPipeline->InitParamMap();
		pPipeline->CreatePSO(m_pRootSignature, pContext);

		m_ShaderMap.emplace(MatID, pPipeline);

		//ExportPreprocessShader(m_sShaderPath, m_sShaderPath + "_" + NumberToString<UINT64>(MatID) + ".txt", Macros.data());
		return MatID;
	}
	return -1;
}

bool CPBRRenderEffect::ExportPreprocessShader(const std::string & inPath, const std::string & outPath, D3D_SHADER_MACRO * pDefines)
{
	std::ifstream inFileStream;
	inFileStream.open(inPath, std::fstream::in);

	if (inFileStream.is_open())
	{
		inFileStream.seekg(0, std::ios::end);

		UINT64 pos = inFileStream.tellg();

		char* unpreprocessShaderText = new char[pos + 1];
		memset(unpreprocessShaderText, 0, pos + 1);
		inFileStream.seekg(0, std::ios::beg);

		inFileStream.read(unpreprocessShaderText, pos + 1);
		inFileStream.close();

		ID3DBlob* pCompiledShadeTextCode = nullptr;
		ID3DBlob* pErrorBlob = nullptr;

		D3DPreprocess(unpreprocessShaderText, pos + 1, nullptr, pDefines, D3D_COMPILE_STANDARD_FILE_INCLUDE, &pCompiledShadeTextCode, &pErrorBlob);

		delete[]unpreprocessShaderText;

		if (pErrorBlob == nullptr)
		{
			void* pTextBlob = pCompiledShadeTextCode->GetBufferPointer();
			UINT64 size = pCompiledShadeTextCode->GetBufferSize();

			std::ofstream outFileStream;
			outFileStream.open(outPath, std::fstream::out | std::fstream::trunc);

			if (outFileStream.is_open())
			{
				outFileStream.write((const char*)pTextBlob, size);
				outFileStream.close();
			}
			pCompiledShadeTextCode->Release();
		}
	}

	return false;
}

CShaderPipeline* CPBRRenderEffect::GetShader(UINT64 MatId)
{
	if (MatId != -1)
	{
		CShaderPipeline* pPass = m_ShaderMap[MatId];
		return pPass;
	}
	return nullptr;
}

UINT64 CPBRRenderEffect::CalcPBRMaterialID(const PBRMaterialMacroInfo & info)
{
	UINT64 id = 0;
	if (info.bHasNormals)			id |= 1 << PBRMaterialMacroType::HAS_NORMALS;
	if (info.bHasTangents)			id |= 1 << PBRMaterialMacroType::HAS_TANGENTS;
	if (info.bHasUVs)				id |= 1 << PBRMaterialMacroType::HAS_UVS;
	if (info.bUseIBL)				id |= 1 << PBRMaterialMacroType::USE_IBL;
	if (info.bHasBaseColorMap)		id |= 1 << PBRMaterialMacroType::HAS_BASE_COLOR_MAP;
	if (info.bHasNornamMap)			id |= 1 << PBRMaterialMacroType::HAS_NORMAL_MAP;
	if (info.bHasEmissiveMap)		id |= 1 << PBRMaterialMacroType::HAS_EMISSIVE_MAP;
	if (info.bHasMetalRoughnessMap)	id |= 1 << PBRMaterialMacroType::HAS_METAL_ROUGHNESS_MAP;
	if (info.bHasOcclusionMap)		id |= 1 << PBRMaterialMacroType::HAS_OCCLUSION_MAP;
	return id;
}

std::vector<std::string> CPBRRenderEffect::CalcPBRMaterialMacro(UINT64 MatId)
{
	std::vector<std::string> macros;
	std::vector<std::string> macroNames =
	{
		"HAS_NORMALS",
		"HAS_TANGENTS",
		"HAS_UV",
		"USE_IBL",
		"HAS_BASE_COLOR_MAP",
		"HAS_NORMAL_MAP",
		"HAS_EMISSIVE_MAP",
		"HAS_METAL_ROUGHNESS_MAP",
		"HAS_OCCLUSION_MAP",
	};

	for (int i = 0; i < PBRMaterialMacroType::MACRO_TYPE_COUNT; ++i)
	{
		if (MatId & ((UINT64)1 << i))
			macros.push_back(macroNames[i]);
	}
	return macros;
}

INPUT_LAYOUT_TYPE CPBRRenderEffect::CalcInputLayoutType(const PBRMaterialMacroInfo & info)
{
	if (info.bHasNormals && info.bHasTangents && info.bHasUVs)
		return INPUT_LAYOUT_TYPE::P3N3T4UV2;
	if (info.bHasNormals && info.bHasUVs)
		return INPUT_LAYOUT_TYPE::P3N3UV2;
	if (info.bHasUVs)
		return INPUT_LAYOUT_TYPE::P3UV2;

	return INPUT_LAYOUT_TYPE::P3;
}

void CPBRRenderEffect::PBRMaterialMacroInfo::Clear()
{
	bHasNormals = false;
	bHasTangents = false;
	bHasUVs = false;

	bUseIBL = false;
	bHasBaseColorMap = false;
	bHasNornamMap = false;
	bHasEmissiveMap = false;
	bHasMetalRoughnessMap = false;
	bHasOcclusionMap = false;
}

CPBRMaterial::PBRMaterialParamater::PBRMaterialParamater()
{
	BaseColorFactor = XMFLOAT4(1.0f,1.0f, 1.0f, 1.0f);
	EmissiveColorFactor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

	MetallicFactor = 1.0f;
	RoughnessFactor = 1.0f;
	NormalScale = 1.0f;
	OcclusionStrength = 1.0f;
}

CPBRMaterial::CPBRMaterial(CPBRRenderEffect* pEffect)
{
	m_bMaterialDirty = true;

	m_pBaseColorMap = nullptr;
	m_pNormalMap = nullptr;
	m_pEmissiveMap = nullptr;
	m_pMetallicRoughnessMap = nullptr;
	m_pOcclusionMap = nullptr;

	m_pEffect = pEffect;
	m_MaterialID = -1;
	m_pRenderPass = nullptr;
}

void CPBRMaterial::InitResource(CGraphicContext * pContext)
{
	m_pBaseColorMap = pContext->CreateTexture2D(m_sBaseColorMapPath);
	m_pNormalMap = pContext->CreateTexture2D(m_sNormalMapPath);
	m_pEmissiveMap = pContext->CreateTexture2D(m_sEmissiveMapPath);
	m_pMetallicRoughnessMap = pContext->CreateTexture2D(m_sMetallicRoughnessMapPath);
	m_pOcclusionMap = pContext->CreateTexture2D(m_sOcclusionMapPath);
}

const D3D12_SHADER_VARIABLE_DESC* CPBRMaterial::GetShaderVariableDesc(const std::string & name) const
{
	CShaderPipeline* Pipeline = m_pEffect->GetShader(m_MaterialID);
	auto it = Pipeline->m_ParamaterMap.ShaderValiableMap.find(name);
	return &(it->second);
}

const D3D12_SHADER_BUFFER_DESC* CPBRMaterial::GetShaderBufferDesc(const std::string & name) const
{
	CShaderPipeline* Pipeline = m_pEffect->GetShader(m_MaterialID);
	auto it = Pipeline->m_ParamaterMap.ConstantBufferMap.find(name);
	return &(it->second);
}

void CPBRMaterial::AttachToMesh(CPBRStaticMesh* pMesh, CGraphicContext * pContext)
{
	m_MacroInfo.Clear();
	if (pMesh->m_NormalAttribute.pVertexBuffer)
		m_MacroInfo.bHasNormals = true;
	if (pMesh->m_TangentAttribute.pVertexBuffer)
		m_MacroInfo.bHasTangents = true;
	if (pMesh->m_UVAttribute.pVertexBuffer)
		m_MacroInfo.bHasUVs = true;
	
	if(m_pBaseColorMap)
		m_MacroInfo.bHasBaseColorMap = true;
	if(m_pNormalMap)
		m_MacroInfo.bHasNornamMap = true;
	else
		m_MacroInfo.bHasTangents = false;
	if(m_pEmissiveMap)
		m_MacroInfo.bHasEmissiveMap = true;
	if(m_pMetallicRoughnessMap)
		m_MacroInfo.bHasMetalRoughnessMap = true;
	if(m_pOcclusionMap)
		m_MacroInfo.bHasOcclusionMap = true;

	m_InputLayout = m_pEffect->CalcInputLayoutType(m_MacroInfo);
	m_MaterialID = m_pEffect->CompileShader(pContext, m_MacroInfo);
	m_pRenderPass = m_pEffect->GetShader(m_MaterialID);

	// 绑定资源 Material， RootSignature 一定要保存BindSlot，方便通过Reflect信息来自动获取BindSlot信息
	const static int PerMaterialBindSlot = 2;
	const D3D12_SHADER_BUFFER_DESC * bufferDesc = GetShaderBufferDesc("cbPerMaterial");
	m_MaterialShaderData.ReleaseData();
	m_MaterialShaderData.InitData(bufferDesc->Size);
	m_MaterialShaderData.UniformBufferLoc = pContext->m_pUniformBufferAllocator->Allocate(bufferDesc->Size, pContext);
	m_MaterialShaderData.RootSignatureSlot = PerMaterialBindSlot;
}

void CPBRMaterial::UpdateRenderData()
{
	if (m_bMaterialDirty)
	{
		m_MaterialShaderData.CopyToCPU(0, sizeof(PBRMaterialParamater), &m_MaterialParamater);
		m_MaterialShaderData.CopyToGPU();
	}
}

void CPBRMaterial::Bind(CGraphicContext * pContext)
{
	if (pContext)
	{
		CShaderPipeline* pPass = m_pEffect->GetShader(m_MaterialID);
		if (pPass)
		{
			pContext->m_pCommandList->SetPipelineState(pPass->m_pPSO);

			pContext->m_pCommandList->SetGraphicsRootDescriptorTable(m_MaterialShaderData.RootSignatureSlot, m_MaterialShaderData.UniformBufferLoc->GPUHandle);

			if (m_pBaseColorMap)
				pContext->m_pCommandList->SetGraphicsRootDescriptorTable(4, m_pBaseColorMap->GPUHandle);
			if (m_pNormalMap)
				pContext->m_pCommandList->SetGraphicsRootDescriptorTable(5, m_pNormalMap->GPUHandle);
			if (m_pEmissiveMap)
				pContext->m_pCommandList->SetGraphicsRootDescriptorTable(6, m_pEmissiveMap->GPUHandle);
			if (m_pMetallicRoughnessMap)
				pContext->m_pCommandList->SetGraphicsRootDescriptorTable(7, m_pMetallicRoughnessMap->GPUHandle);
			if (m_pOcclusionMap)
				pContext->m_pCommandList->SetGraphicsRootDescriptorTable(8, m_pOcclusionMap->GPUHandle);
		}
	}
}


void CPBRStaticMesh::Create(const MeshData& meshData, CGraphicContext& Context)
{
	if (meshData.Positions.size() > 0)
	{
		m_PositonAttribute.pVertexBuffer = Context.CreateDefaultBuffer(&meshData.Positions[0], meshData.Positions.size() * sizeof(XMFLOAT3));
		m_PositonAttribute.VertexView = Context.CreateVertexBufferView(m_PositonAttribute.pVertexBuffer, (UINT)meshData.Positions.size() * sizeof(XMFLOAT3), sizeof(XMFLOAT3));
	}

	if (meshData.Normals.size() > 0)
	{
		m_NormalAttribute.pVertexBuffer = Context.CreateDefaultBuffer(&meshData.Normals[0], meshData.Normals.size() * sizeof(XMFLOAT3));
		m_NormalAttribute.VertexView = Context.CreateVertexBufferView(m_NormalAttribute.pVertexBuffer, (UINT)meshData.Normals.size() * sizeof(XMFLOAT3), sizeof(XMFLOAT3));
	}

	if (meshData.Tangents.size() > 0)
	{
		m_TangentAttribute.pVertexBuffer = Context.CreateDefaultBuffer(&meshData.Tangents[0], meshData.Tangents.size() * sizeof(XMFLOAT4));
		m_TangentAttribute.VertexView = Context.CreateVertexBufferView(m_TangentAttribute.pVertexBuffer, (UINT)meshData.Tangents.size() * sizeof(XMFLOAT4), sizeof(XMFLOAT4));
	}

	if (meshData.UVs.size() > 0)
	{
		m_UVAttribute.pVertexBuffer = Context.CreateDefaultBuffer(&meshData.UVs[0], meshData.UVs.size() * sizeof(XMFLOAT2));
		m_UVAttribute.VertexView = Context.CreateVertexBufferView(m_UVAttribute.pVertexBuffer, (UINT)meshData.UVs.size() * sizeof(XMFLOAT2), sizeof(XMFLOAT2));
	}

	if (meshData.Indices.size() > 0)
	{
		m_IndexAttribute.pIndexBuffer = Context.CreateDefaultBuffer(&meshData.Indices[0], (UINT)meshData.Indices.size() * sizeof(UINT));
		m_IndexAttribute.IndexView = Context.CreateIndexBufferView(m_IndexAttribute.pIndexBuffer, (UINT)meshData.Indices.size() * sizeof(UINT), DXGI_FORMAT_R32_UINT);
	}

	PBRSubMesh subMesh = { (UINT)meshData.Indices.size(), 0, 0 };
	m_SubMeshes.push_back(subMesh);
	m_pMaterials.resize(m_SubMeshes.size());
}

void CPBRStaticMesh::SetMaterial(int materialSlot, CPBRMaterial* pMat, CGraphicContext* pContext)
{
	if (materialSlot >= 0 && materialSlot < m_SubMeshes.size())
	{
		m_pMaterials[materialSlot] = pMat;
		pMat->AttachToMesh(this, pContext);
	}
}

void CPBRStaticMesh::Bind(CGraphicContext * pContext)
{
	for (int i = 0; i < m_pMaterials.size(); ++i)
	{
		const PBRSubMesh& SubMesh = m_SubMeshes[i];
		CPBRMaterial* pMat = m_pMaterials[i];

		if (pMat)
		{
			pMat->Bind(pContext);
			BindMesh(pContext, pMat->m_InputLayout);
			pContext->m_pCommandList->DrawIndexedInstanced(SubMesh.nIndexCount, 1, SubMesh.nStartIndexLocation, SubMesh.nBaseVertexLocation, 0);
		}
	}
}

void CPBRStaticMesh::BindMesh(CGraphicContext * pContext, INPUT_LAYOUT_TYPE inputLayout)
{
	switch (inputLayout)
	{
	case P3:
	{
		D3D12_VERTEX_BUFFER_VIEW VertexBufferViews[] = { m_PositonAttribute.VertexView };
		pContext->m_pCommandList->IASetVertexBuffers(0, 1, &VertexBufferViews[0]);
		break;
	}
	case P3UV2:
	{
		D3D12_VERTEX_BUFFER_VIEW VertexBufferViews[] = { m_PositonAttribute.VertexView, m_UVAttribute.VertexView };
		pContext->m_pCommandList->IASetVertexBuffers(0, 2, &VertexBufferViews[0]);
		break;
	}
	case P3N3:
	{
		D3D12_VERTEX_BUFFER_VIEW VertexBufferViews[] = { m_PositonAttribute.VertexView, m_NormalAttribute.VertexView };
		pContext->m_pCommandList->IASetVertexBuffers(0, 2, &VertexBufferViews[0]);
		break;
	}
	case P3N3UV2:
	{
		D3D12_VERTEX_BUFFER_VIEW VertexBufferViews[] = { m_PositonAttribute.VertexView, m_NormalAttribute.VertexView, m_UVAttribute.VertexView };
		pContext->m_pCommandList->IASetVertexBuffers(0, 3, &VertexBufferViews[0]);
		break;
	}
	case P3N3T4UV2:
	{
		D3D12_VERTEX_BUFFER_VIEW VertexBufferViews[] = { m_PositonAttribute.VertexView, m_NormalAttribute.VertexView, m_TangentAttribute.VertexView, m_UVAttribute.VertexView };
		pContext->m_pCommandList->IASetVertexBuffers(0, 4, &VertexBufferViews[0]);
		break;
	}
	}

	pContext->m_pCommandList->IASetIndexBuffer(&m_IndexAttribute.IndexView);
	pContext->m_pCommandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}


CPBRGeometryNode::CPBRGeometryNode()
{
	m_pMesh = nullptr;

	m_Transform.Position = XMFLOAT3(0.0f, 0.0f, 0.0f);
	m_Transform.Scale = XMFLOAT3(1.0f, 1.0f, 1.0f);
	m_Transform.bDirty = true;
}

CPBRGeometryNode::~CPBRGeometryNode()
{

}

void CPBRGeometryNode::InitResource(CGraphicContext * pContext)
{
	const int PerObjectBindSlot = 1;

	m_NodeShaderData.ReleaseData();
	m_NodeShaderData.InitData(sizeof(PBRObjectUniformParamater));
	m_NodeShaderData.UniformBufferLoc = pContext->m_pUniformBufferAllocator->Allocate(m_NodeShaderData.Size, pContext);
	m_NodeShaderData.RootSignatureSlot = PerObjectBindSlot;
}

void CPBRGeometryNode::SetPostion(const XMFLOAT3 & v)
{
	m_Transform.Position = v;
	m_Transform.bDirty = true;
}

void CPBRGeometryNode::SetScale(const XMFLOAT3 & v)
{
	m_Transform.Scale = v;
	m_Transform.bDirty = true;
}

void CPBRGeometryNode::SetMesh(CPBRStaticMesh * pMesh)
{
	m_pMesh = pMesh;
}

void CPBRGeometryNode::Update()
{
	
}

void CPBRGeometryNode::UpdateRenderData()
{
	if (m_Transform.bDirty)
	{
		m_Transform.UpdateMatrix();

		/*const D3D12_SHADER_VARIABLE_DESC* WorldMatrixVariableDesc = m_pMaterial->GetShaderVariableDesc("g_mWorldMat");
		const D3D12_SHADER_VARIABLE_DESC* InvWorldMatrixVariableDesc = m_pMaterial->GetShaderVariableDesc("g_mInvWorldMat");

		XMFLOAT4X4 mWorldMatrixParam;
		XMStoreFloat4x4(&mWorldMatrixParam, XMMatrixTranspose(m_Transform.mWorldMat));
		m_NodeShaderData.CopyToCPU(WorldMatrixVariableDesc->StartOffset, WorldMatrixVariableDesc->Size, &mWorldMatrixParam);

		XMFLOAT4X4 mInvWorldMatrixParam;
		XMStoreFloat4x4(&mInvWorldMatrixParam, XMMatrixTranspose(m_Transform.mInvWorldMat));
		m_NodeShaderData.CopyToCPU(InvWorldMatrixVariableDesc->StartOffset, InvWorldMatrixVariableDesc->Size, &mInvWorldMatrixParam);

		m_NodeShaderData.CopyToGPU();*/

		PBRObjectUniformParamater UniformParam;
		XMStoreFloat4x4(&UniformParam.mWorldMatrixParam, XMMatrixTranspose(m_Transform.mWorldMat));
		XMStoreFloat4x4(&UniformParam.mInvWorldMatrixParam, XMMatrixTranspose(m_Transform.mInvWorldMat));
		m_NodeShaderData.CopyToCPU(0, sizeof(PBRObjectUniformParamater), &UniformParam);
		m_NodeShaderData.CopyToGPU();
	}

	for (int i = 0; i < m_pMesh->m_pMaterials.size(); ++i)
	{
		if (m_pMesh->m_pMaterials[i])
		{
			m_pMesh->m_pMaterials[i]->UpdateRenderData();
		}
	}
}

void CPBRGeometryNode::Bind(CGraphicContext * pContext)
{
	pContext->m_pCommandList->SetGraphicsRootDescriptorTable(m_NodeShaderData.RootSignatureSlot, m_NodeShaderData.UniformBufferLoc->GPUHandle);
}

void CPBRGeometryNode::Draw(CGraphicContext* pContext)
{
	Bind(pContext);
	m_pMesh->Bind(pContext);
}

void PBRTransform::UpdateMatrix()
{
	XMMATRIX mScaleMat = XMMatrixScaling(Scale.x, Scale.y, Scale.z);
	XMMATRIX mTranslateMat = XMMatrixTranslation(Position.x, Position.y, Position.z);
	mWorldMat = mScaleMat * mTranslateMat;
	mInvWorldMat = XMMatrixInverse(&XMMatrixDeterminant(mWorldMat), mWorldMat);
}
