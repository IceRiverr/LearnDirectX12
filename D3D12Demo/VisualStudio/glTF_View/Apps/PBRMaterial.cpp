
#include "PBRMaterial.h"

PBRMaterialShaderBlock::PBRMaterialShaderBlock()
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

	// AllocateBuffer
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

void CPBRRenderEffect::SetShaderPath(const std::string path)
{
	m_sShaderPath = path;
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
		pPass->m_pVSShaderCode = pContext->CompileShader(m_sShaderPath, "VSMain", "vs_5_0", Macros.data());
		pPass->m_pPSShaderCode = pContext->CompileShader(m_sShaderPath, "PSMain", "ps_5_0", Macros.data());

		D3D12_GRAPHICS_PIPELINE_STATE_DESC OpaquePSODesc = {};
		INPUT_LAYOUT_TYPE IAType = CalcInputLayoutType(info);
		auto InputLayout = GetInputLayout(IAType);
		OpaquePSODesc.InputLayout = { InputLayout.data(), (UINT)InputLayout.size() };
		OpaquePSODesc.pRootSignature = pContext->m_pRootSignature;
		OpaquePSODesc.VS = { pPass->m_pVSShaderCode->GetBufferPointer(), pPass->m_pVSShaderCode->GetBufferSize() };
		OpaquePSODesc.PS = { pPass->m_pPSShaderCode->GetBufferPointer(), pPass->m_pPSShaderCode->GetBufferSize() };
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
