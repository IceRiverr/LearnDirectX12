
#pragma once
#include <DirectXMath.h>
#include <d3d12.h>
#include "d3dx12.h"
#include <string>
#include "BufferManager.h"
#include <unordered_map>
#include <vector>
#include "GraphicContext.h"

using namespace DirectX;

struct PBRMaterialShaderBlock
{
	PBRMaterialShaderBlock();

	XMFLOAT4 BaseColorFactor;
	XMFLOAT4 EmissiveColorFactor;

	float MetallicFactor;
	float RoughnessFactor;
	float NormalScale;
	float OcclusionStrength;
};

struct RenderPass
{
	ID3DBlob* m_pVSShaderCode;
	ID3DBlob* m_pPSShaderCode;
	ID3D12PipelineState* m_pPSO;
};

class CPBRRenderEffect
{
public:
	enum PBRMaterialMacroType
	{
		HAS_NORMALS = 0,
		HAS_TANGENTS,
		HAS_UVS,
		USE_IBL,
		HAS_BASE_COLOR_MAP,
		HAS_NORMAL_MAP,
		HAS_EMISSIVE_MAP,
		HAS_METAL_ROUGHNESS_MAP,
		HAS_OCCLUSION_MAP,

		MACRO_TYPE_COUNT
	};

	struct PBRMaterialMacroInfo
	{
		void Clear();

		bool bHasNormals;
		bool bHasTangents;
		bool bHasUVs;

		bool bUseIBL;
		bool bHasBaseColorMap;
		bool bHasNornamMap;
		bool bHasEmissiveMap;
		bool bHasMetalRoughnessMap;
		bool bHasOcclusionMap;
	};
public:
	void SetShaderPath(const std::string path);
	UINT64 CompileShader(CGraphicContext* pContext, const PBRMaterialMacroInfo& info);
	RenderPass* GetShader(UINT64 MatId);

	static UINT64 CalcPBRMaterialID(const PBRMaterialMacroInfo& info);
	static std::vector<std::string> CalcPBRMaterialMacro(UINT64 MatId);
	static INPUT_LAYOUT_TYPE CalcInputLayoutType(const PBRMaterialMacroInfo& info);

private:
	std::unordered_map<UINT64, RenderPass*> m_ShaderMap;
	std::string m_sShaderPath;
};

class CPBRMaterial
{
public:
	CPBRMaterial(CPBRRenderEffect* pEffect);
	
	void InitResource(CGraphicContext* pContext);

	// 如果同一个材质被附着给不同的对象，则应该怎么处理不同的InputLayout，这样处理肯定有问题
	void AttachToMesh(CStaticMesh* pMesh, CGraphicContext* pContext);

public:
	std::string m_Name;

	PBRMaterialShaderBlock m_ShaderBlock;

	std::string m_sBaseColorMapPath;
	std::string m_sNormalMapPath;
	std::string m_sEmissiveMapPath;
	std::string m_sMetallicRoughnessMapPath;
	std::string m_sOcclusionMapPath;

public:
	ConstantBufferAddress m_BufferAddress;

	Texture2D* m_pBaseColorMap;
	Texture2D* m_pNormalMap;
	Texture2D* m_pEmissiveMap;
	Texture2D* m_pMetallicRoughnessMap;
	Texture2D* m_pOcclusionMap;

	CPBRRenderEffect* m_pEffect;
	UINT64 m_MaterialID;
	
	// 放在这里有问题，先保证正确
	RenderPass* m_pRenderPass;
	CPBRRenderEffect::PBRMaterialMacroInfo m_MacroInfo;
};

typedef TBaseConstantBuffer<PBRMaterialShaderBlock> CPBRMaterialConstantBuffer;