
#pragma once
#include <DirectXMath.h>
#include <d3d12.h>
#include "d3dx12.h"
#include <string>
#include "BufferManager.h"

using namespace DirectX;

struct PBRMaterialShaderBlock
{
	PBRMaterialShaderBlock();

	XMFLOAT4 BaseColorFactor;
	XMFLOAT4 EmissiveColorFactor;
	XMFLOAT4 SpecularColorFactor;

	float MetallicFactor;
	float RoughnessFactor;
	float NormalScale;
	float OcclusionStrength;
};

class CRenderEffect
{
public:
	CRenderEffect();
public:
	ID3DBlob* m_pVSShaderCode;
	ID3DBlob* m_pPSShaderCode;
	ID3D12PipelineState* m_pPSO;
	
};

class CPBRMaterial
{
public:
	CPBRMaterial();

public:
	std::string m_Name;
	PBRMaterialShaderBlock m_ShaderBlock;
	ConstantBufferAddress m_BufferAddress;
};

typedef TBaseConstantBuffer<PBRMaterialShaderBlock> CPBRMaterialConstantBuffer;