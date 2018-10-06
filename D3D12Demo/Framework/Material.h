#pragma once
#include "stdafx.h"
#include "Utility.h"
#include <unordered_map>
#include "GPUResource.h"

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
	P3N3,
	P3N3T4UV2,
};

std::vector<D3D12_INPUT_ELEMENT_DESC> GetInputLayout(INPUT_LAYOUT_TYPE type);

class CMaterial
{
public:
	MaterialShaderBlock CreateShaderBlock() const;

public:
	std::string m_sName;

	XMFLOAT4 m_cBaseColor;
	
	float m_fSmoothness;
	float m_fMetalMask;
	float m_fReflectance;

	Texture2DResource* m_pAldeboMap;
	Texture2DResource* m_pNormalMap;
	Texture2DResource* m_pRoughnessMap;
	Texture2DResource* m_pMetalicMap;

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
