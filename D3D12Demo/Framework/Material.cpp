#include "Material.h"

MaterialShaderBlock CMaterial::CreateShaderBlock() const
{
	MaterialShaderBlock ShaderBlock = {};
	ShaderBlock.BaseColor = m_cBaseColor;
	
	ShaderBlock.Roughness = (1.0f - m_fSmoothness) *(1.0f -m_fSmoothness); // Remap
	ShaderBlock.MetalMask = m_fMetalMask;
	ShaderBlock.F0 = 0.16f * m_fReflectance *  m_fReflectance; // Remap;

	return ShaderBlock;
}
