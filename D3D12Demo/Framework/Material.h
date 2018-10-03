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

	ConstantBufferAddress m_MaterialAddress;
};

class CBRDFMaterial : public CMaterial
{
public:
	std::string m_AldeboMapPath;
	std::string m_NormalMapPath;
	std::string m_SmoothnessMapPath;
	std::string m_MetalicMapPath;
};
