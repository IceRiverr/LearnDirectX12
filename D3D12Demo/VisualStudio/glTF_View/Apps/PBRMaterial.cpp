
#include "PBRMaterial.h"

PBRMaterialShaderBlock::PBRMaterialShaderBlock()
{
	BaseColorFactor = XMFLOAT4(1.0f,1.0f, 1.0f, 1.0f);
	EmissiveColorFactor = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
	SpecularColorFactor = XMFLOAT4(0.04f, 0.04f, 0.04f, 1.0f);

	MetallicFactor = 1.0f;
	RoughnessFactor = 1.0f;
	NormalScale = 1.0f;
	OcclusionStrength = 1.0f;
}

CPBRMaterial::CPBRMaterial()
{

}

CRenderEffect::CRenderEffect()
{
	m_pVSShaderCode = nullptr;
	m_pPSShaderCode = nullptr;
	m_pPSO = nullptr;
}
