
#include "PBRMaterial.h"
#include <d3d12shader.h>
#include <fstream>
#include <sstream>

PBRMaterialShaderBlockDef::PBRMaterialShaderBlockDef()
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

	// 给材质参数分配资源
	RenderPass* pPass = m_pEffect->GetShader(m_MaterialID);
	if (pPass)
	{
		const D3D12_SHADER_BUFFER_DESC * bufferDesc = pPass->m_pPSShaderCode->FindUniformBufferDesc("cbPerMaterial");
		UniformBufferLocation* BufferLoc = pContext->m_pUniformBufferAllocator->Allocate(bufferDesc->Size);
	}
}

const D3D12_SHADER_VARIABLE_DESC* CPBRMaterial::GetShaderVariableDesc(const std::string & name) const
{
	return nullptr;
}

const D3D12_SHADER_BUFFER_DESC* CPBRMaterial::GetShaderBufferDesc(const std::string & name) const
{
	return nullptr;
}

UINT CPBRMaterial::GetRootSignatureSlot(const std::string & name) const
{
	return 0;
}

void CPBRMaterial::AttachToMesh(CStaticMesh* pMesh, CGraphicContext * pContext)
{
	m_MacroInfo.Clear();
	if (pMesh->m_pNormalBufferGPU)
		m_MacroInfo.bHasNormals = true;
	if (pMesh->m_pTangentBufferGPU)
		m_MacroInfo.bHasTangents = true;
	if (pMesh->m_pUVBufferGPU)
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
	
	m_MaterialID = m_pEffect->CompileShader(pContext, m_MacroInfo);
	m_pRenderPass = m_pEffect->GetShader(m_MaterialID);
}

void CPBRMaterial::Bind(CGraphicContext * pContext, CRenderObject * pObj)
{
	if (pContext && pObj)
	{
		RenderPass* pPass = m_pEffect->GetShader(m_MaterialID);
		if (pPass)
		{
			pContext->m_pCommandList->SetPipelineState(pPass->m_pPSO);
			pContext->m_pCommandList->SetGraphicsRootDescriptorTable(0, m_pBaseColorMap->GPUHandle);
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
			Macros.push_back({ MacroDefines[i].c_str(), "1"});
		}
		Macros.push_back({ nullptr, nullptr });

		RenderPass* pPass = new RenderPass();
		ID3DBlob* vsCode = pContext->CompileShader(m_sShaderPath, "VSMain", "vs_5_0", Macros.data());
		ID3DBlob* psCode = pContext->CompileShader(m_sShaderPath, "PSMain", "ps_5_0", Macros.data());
		pPass->m_pVSShaderCode = std::make_shared<CShader>(vsCode);
		pPass->m_pPSShaderCode = std::make_shared<CShader>(psCode);
		pPass->Reflect(); 
		pPass->MergeParameters();

		ExportPreprocessShader(m_sShaderPath, m_sShaderPath + "_" + NumberToString<UINT64>(MatID) + ".txt", Macros.data());

		D3D12_GRAPHICS_PIPELINE_STATE_DESC OpaquePSODesc = {};
		INPUT_LAYOUT_TYPE IAType = CalcInputLayoutType(info);
		auto InputLayout = GetInputLayout(IAType);
		OpaquePSODesc.InputLayout = { InputLayout.data(), (UINT)InputLayout.size() };
		OpaquePSODesc.pRootSignature = m_pRootSignature;
		OpaquePSODesc.VS = { pPass->m_pVSShaderCode->GetShaderCode()->GetBufferPointer(), pPass->m_pVSShaderCode->GetShaderCode()->GetBufferSize() };
		OpaquePSODesc.PS = { pPass->m_pPSShaderCode->GetShaderCode()->GetBufferPointer(), pPass->m_pPSShaderCode->GetShaderCode()->GetBufferSize() };
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
		pContext->m_pDevice->CreateGraphicsPipelineState(&OpaquePSODesc, IID_PPV_ARGS(&pPass->m_pPSO));

		m_ShaderMap.emplace(MatID, pPass);
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

RenderPass* CPBRRenderEffect::GetShader(UINT64 MatId)
{
	if (MatId != -1)
	{
		RenderPass* pPass = m_ShaderMap[MatId];
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

void RenderPass::Reflect()
{
	if (m_pVSShaderCode)
		m_pVSShaderCode->Reflect();
	if (m_pPSShaderCode)
		m_pPSShaderCode->Reflect();
}

void RenderPass::MergeParameters()
{

}

CShader::CShader(ID3DBlob * code)
{
	m_pShaderCode = code;
}

CShader::~CShader()
{
	if (m_pShaderCode)
		m_pShaderCode->Release();
}

void CShader::Reflect()
{
	if (m_pShaderCode)
	{
		ID3D12ShaderReflection* pReflector = nullptr;
		D3DReflect(m_pShaderCode->GetBufferPointer(), m_pShaderCode->GetBufferSize(), IID_ID3D12ShaderReflection, (void**)&pReflector);
		if (pReflector)
		{
			D3D12_SHADER_DESC shaderDesc;
			pReflector->GetDesc(&shaderDesc);

			// For InputLayout
			for (UINT i = 0; i < shaderDesc.InputParameters; ++i)
			{
				D3D12_SIGNATURE_PARAMETER_DESC desc;
				pReflector->GetInputParameterDesc(i, &desc);
				m_InputParameters.push_back(desc);
			}

			for (UINT i = 0; i < shaderDesc.BoundResources; ++i)
			{
				D3D12_SHADER_INPUT_BIND_DESC inputBindDesc;
				pReflector->GetResourceBindingDesc(i, &inputBindDesc);
				m_BoundResources.push_back(inputBindDesc);
			}

			for (UINT i = 0; i < shaderDesc.ConstantBuffers; ++i)
			{
				ID3D12ShaderReflectionConstantBuffer* pRefCB = pReflector->GetConstantBufferByIndex(i);
				D3D12_SHADER_BUFFER_DESC bufferDesc;
				pRefCB->GetDesc(&bufferDesc);
				m_ConstantBuffers.push_back(bufferDesc);

				for (UINT val = 0; val < bufferDesc.Variables; ++val)
				{
					ID3D12ShaderReflectionVariable* pRefVal = pRefCB->GetVariableByIndex(val);
					D3D12_SHADER_VARIABLE_DESC variableDesc;
					pRefVal->GetDesc(&variableDesc);
					m_ShaderValiableMap.emplace(variableDesc.Name, variableDesc);
				}
			}

			pReflector->Release();
		}
	}
}

void CShader::SetFloat(std::string & name, float value)
{
	// 设值是需要考虑字节对齐，比如16位对齐，则数据应该怎么安排，下面的写法可能有点问题
	unsigned char buffer[32];
	auto it = m_ShaderValiableMap.find(name);
	if (it != m_ShaderValiableMap.end())
	{
		memcpy(buffer + it->second.StartOffset, &value, sizeof(value));
	}
}

const D3D12_SHADER_BUFFER_DESC * CShader::FindUniformBufferDesc(const std::string& bufferName) const
{
	for (int i = 0; i < m_ConstantBuffers.size(); ++i)
	{
		if(strcmp(m_ConstantBuffers[i].Name, bufferName.c_str()) == 0)
		{
			return &m_ConstantBuffers[i];
		}
	}
	return nullptr;
}

CPBRGeometryNode::CPBRGeometryNode()
{
	m_pMesh = nullptr;
	m_pMaterial = nullptr;
}

CPBRGeometryNode::~CPBRGeometryNode()
{
}

void CPBRGeometryNode::InitResource(CGraphicContext * pContext)
{
	if (m_pMaterial)
	{
		const D3D12_SHADER_BUFFER_DESC* BufferDesc = m_pMaterial->GetShaderBufferDesc("cbPerObject");
		m_NodeShaderData.ReleaseData();
		m_NodeShaderData.InitData(BufferDesc->Size);
		m_NodeShaderData.UniformBufferLoc = pContext->m_pUniformBufferAllocator->Allocate(m_NodeShaderData.Size);
		m_NodeShaderData.RootSignatureSlot = m_pMaterial->GetRootSignatureSlot("cbPerObject");
	}
}

void CPBRGeometryNode::SetPostion(const XMFLOAT3 & v)
{
	m_Transform.Position = v;
}

void CPBRGeometryNode::SetScale(const XMFLOAT3 & v)
{
	m_Transform.Scale = v;
}

void CPBRGeometryNode::SetMesh(CPBRStaticMesh * pMesh)
{
	m_pMesh = m_pMesh;
}

void CPBRGeometryNode::SetMaterial(CPBRMaterial * pMat)
{
	if (pMat)
	{
		m_pMaterial = pMat;
	}
}

void CPBRGeometryNode::Update()
{
	if (m_Transform.bDirty)
	{
		m_Transform.UpdateMatrix();

		const D3D12_SHADER_VARIABLE_DESC* WorldMatrixVariableDesc = m_pMaterial->GetShaderVariableDesc("g_mWorldMat");
		const D3D12_SHADER_VARIABLE_DESC* InvWorldMatrixVariableDesc = m_pMaterial->GetShaderVariableDesc("g_mInvWorldMat");

		XMFLOAT4X4 mWorldMatrixParam;
		XMStoreFloat4x4(&mWorldMatrixParam, XMMatrixTranspose(m_Transform.mWorldMat));
		m_NodeShaderData.CopyToCPU(WorldMatrixVariableDesc->StartOffset, WorldMatrixVariableDesc->Size, &mWorldMatrixParam);

		XMFLOAT4X4 mInvWorldMatrixParam;
		XMStoreFloat4x4(&mInvWorldMatrixParam, XMMatrixTranspose(m_Transform.mInvWorldMat));
		m_NodeShaderData.CopyToCPU(InvWorldMatrixVariableDesc->StartOffset, InvWorldMatrixVariableDesc->Size, &mInvWorldMatrixParam);

		m_NodeShaderData.CopyToGPU();
	}
	m_pMaterial->Update();
}

void CPBRGeometryNode::Draw(CGraphicContext* pContext)
{
	m_pMaterial->Bind(nullptr, nullptr);
	m_pMesh->BindMesh(pContext);

	pContext->m_pCommandList->SetGraphicsRootDescriptorTable(m_NodeShaderData.RootSignatureSlot, m_NodeShaderData.UniformBufferLoc->GPUHandle);
	
}

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

void PBRTransform::UpdateMatrix()
{
	XMMATRIX mScaleMat = XMMatrixScaling(Scale.x, Scale.y, Scale.z);
	XMMATRIX mTranslateMat = XMMatrixTranslation(Position.x, Position.y, Position.z);
	mWorldMat = mScaleMat * mTranslateMat;
	mInvWorldMat = XMMatrixInverse(&XMMatrixDeterminant(mWorldMat), mWorldMat);
}

void CPBRStaticMesh::BindMesh(CGraphicContext * pContext)
{
	switch (m_DataLayout)
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
