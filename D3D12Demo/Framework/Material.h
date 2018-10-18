#pragma once
#include "stdafx.h"
#include "Utility.h"
#include <unordered_map>
#include "GPUResource.h"
#include "GraphicContext.h"

struct MaterialShaderBlock
{
	XMFLOAT4 BaseColor;
	
	float Roughness;
	float MetalMask;
	float F0;

	float Unused;
};

enum INPUT_LAYOUT_TYPE
{
	P3,
	P3UV2,
	P3N3,
	P3N3UV2,
	P3N3T4UV2
};

std::vector<D3D12_INPUT_ELEMENT_DESC> GetInputLayout(INPUT_LAYOUT_TYPE type);

struct MaterialResource
{
	MaterialShaderBlock ShaderBlock;

	Texture2DResource* pAldeboMap;
	Texture2DResource* pNormalMap;
	Texture2DResource* pRoughnessMap;
	Texture2DResource* pMetalicMap;
};

class CMaterial
{
public:
	MaterialShaderBlock CreateShaderBlock() const;
	void Init(CGraphicContext* pContext);

public:
	std::string m_sName;

	XMFLOAT4 m_cBaseColor;
	
	float m_fSmoothness;
	float m_fMetalMask;
	float m_fReflectance;

	std::string m_sAldeboPath;
	std::string m_sNormalPath;
	std::string m_sRoughnessPath;
	std::string m_sMetalicPath;

	MaterialResource m_MaterialResource;
	ConstantBufferAddress m_MaterialAddress;
	INPUT_LAYOUT_TYPE m_InputLayoutType;
	ID3D12PipelineState* m_pPSO;
};

class CBRDFMaterial : public CMaterial
{
public:
	std::string m_AldeboMapPath;
	std::string m_NormalMapPath;
	std::string m_SmoothnessMapPath;
	std::string m_MetalicMapPath;
};
