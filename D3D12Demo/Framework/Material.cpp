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

std::vector<D3D12_INPUT_ELEMENT_DESC> GetInputLayout(INPUT_LAYOUT_TYPE type)
{
	std::vector<D3D12_INPUT_ELEMENT_DESC> InputLayuot;
	switch (type)
	{
	case INPUT_LAYOUT_TYPE::P3:
		InputLayuot =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};
		break;
	case INPUT_LAYOUT_TYPE::P3UV2:
		InputLayuot =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD",	0, DXGI_FORMAT_R32G32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA , 0 }
		};
		break;
	case INPUT_LAYOUT_TYPE::P3N3:
		InputLayuot =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "NORMAL",	  0, DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};
		break;
	case INPUT_LAYOUT_TYPE::P3N3T4UV2:
		InputLayuot =
		{
			{ "POSITION",	0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "NORMAL",		0, DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TANGENT",	0, DXGI_FORMAT_R32G32B32A32_FLOAT, 2, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD",	0, DXGI_FORMAT_R32G32_FLOAT, 3, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA , 0 }
		};
	default:
		break;
	}
	return InputLayuot;
}
