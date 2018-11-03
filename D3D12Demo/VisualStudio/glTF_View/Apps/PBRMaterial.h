
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

class CPBRStaticMesh;

struct PBRShaderDataBlock
{
	PBRShaderDataBlock();
	void InitData(UINT size);
	void ReleaseData();
	void CopyToCPU(UINT offset, UINT size, void* v);
	void CopyToGPU();

	UINT8* Data;
	UINT Size;
	UINT RootSignatureSlot;
	UniformBufferLocation* UniformBufferLoc;
};

struct ShaderParamaterMap
{
	std::unordered_map<std::string, D3D12_SHADER_VARIABLE_DESC> ShaderValiableMap;
	std::vector<D3D12_SIGNATURE_PARAMETER_DESC> InputParameters;
	std::unordered_map<std::string, D3D12_SHADER_BUFFER_DESC> ConstantBufferMap;
	std::unordered_map<std::string, D3D12_SHADER_INPUT_BIND_DESC> BoundResourceMap;
};

class CShaderPipeline
{
public:
	CShaderPipeline();

	void InitParamMap();
	void CreatePSO(ID3D12RootSignature* pRootSignature, CGraphicContext* pContext);
	
	ID3DBlob * m_pVSShader;
	ID3DBlob * m_pPSShader;
	ID3D12PipelineState* m_pPSO;

	INPUT_LAYOUT_TYPE m_InputLayout;
	ShaderParamaterMap m_ParamaterMap;

private:
	void ReflectShader(ID3DBlob* pShader, ShaderParamaterMap& ParamMap);
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
	void SetRootSignature(ID3D12RootSignature* rootSignature);
	UINT64 CompileShader(CGraphicContext* pContext, const PBRMaterialMacroInfo& info);
	
	bool ExportPreprocessShader(const std::string& inPath, const std::string& outPath, D3D_SHADER_MACRO* pDefines);

	CShaderPipeline* GetShader(UINT64 MatId);

	static UINT64 CalcPBRMaterialID(const PBRMaterialMacroInfo& info);
	static std::vector<std::string> CalcPBRMaterialMacro(UINT64 MatId);
	static INPUT_LAYOUT_TYPE CalcInputLayoutType(const PBRMaterialMacroInfo& info);

public:
	std::unordered_map<UINT64, CShaderPipeline*> m_ShaderMap;
	std::string m_sShaderPath;
	ID3D12RootSignature* m_pRootSignature;
};

class CPBRMaterial
{
public:
	struct PBRMaterialParamater
	{
		PBRMaterialParamater();

		XMFLOAT4 BaseColorFactor;
		XMFLOAT4 EmissiveColorFactor;

		float MetallicFactor;
		float RoughnessFactor;
		float NormalScale;
		float OcclusionStrength;
	};
public:
	CPBRMaterial(CPBRRenderEffect* pEffect);
	
	void InitResource(CGraphicContext* pContext);

	const D3D12_SHADER_VARIABLE_DESC* GetShaderVariableDesc(const std::string& name) const;
	const D3D12_SHADER_BUFFER_DESC* GetShaderBufferDesc(const std::string& name) const;
	UINT GetRootSignatureSlot(const std::string & name) const;
	void AttachToMesh(CPBRStaticMesh* pMesh, CGraphicContext* pContext);

	void UpdateRenderData();
	void Bind(CGraphicContext* pContext);

public:
	std::string m_Name;
	INPUT_LAYOUT_TYPE m_InputLayout;
	bool m_bMaterialDirty;
	PBRMaterialParamater m_MaterialParamater;

	std::string m_sBaseColorMapPath;
	std::string m_sNormalMapPath;
	std::string m_sEmissiveMapPath;
	std::string m_sMetallicRoughnessMapPath;
	std::string m_sOcclusionMapPath;

public:
	PBRShaderDataBlock m_MaterialShaderData;

	Texture2D* m_pBaseColorMap;
	Texture2D* m_pNormalMap;
	Texture2D* m_pEmissiveMap;
	Texture2D* m_pMetallicRoughnessMap;
	Texture2D* m_pOcclusionMap;

	CPBRRenderEffect* m_pEffect;
	UINT64 m_MaterialID;
	
	CShaderPipeline* m_pRenderPass;
	CPBRRenderEffect::PBRMaterialMacroInfo m_MacroInfo;
};

class CPBRMaterialInstance
{
public:
	CPBRMaterial* m_pMaterial;
	UINT64 m_MaterialID;
	CShaderPipeline* m_pRenderPass;
};

struct VertexAttribute
{
	std::string Name;
	DXGI_FORMAT Format;
	
	ID3D12Resource* pVertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW VertexView;
};

struct IndexAttribute
{
	DXGI_FORMAT Format;

	ID3D12Resource* pIndexBuffer;
	D3D12_INDEX_BUFFER_VIEW IndexView;
};

struct PBRSubMesh
{
	UINT nIndexCount;
	UINT nStartIndexLocation;
	INT nBaseVertexLocation;
};

class CPBRStaticMesh
{
public:
	void Create(const MeshData& meshData, CGraphicContext& Context);
	void SetMaterial(int materialSlot, CPBRMaterial* pMat, CGraphicContext* pContext);
	void Bind(CGraphicContext* pContext);
	void BindMesh(CGraphicContext* pContext, INPUT_LAYOUT_TYPE inputLayout);

public:
	std::vector<PBRSubMesh> m_SubMeshes;
	std::vector<CPBRMaterial*> m_pMaterials;

public:
	VertexAttribute m_PositonAttribute;
	VertexAttribute m_NormalAttribute;
	VertexAttribute m_UVAttribute;
	VertexAttribute m_TangentAttribute;

	IndexAttribute m_IndexAttribute;
};

struct PBRTransform
{
	void UpdateMatrix();

	bool bDirty;

	XMFLOAT3 Position;
	XMFLOAT4 Rotation;
	XMFLOAT3 Scale;

	XMMATRIX mWorldMat;
	XMMATRIX mInvWorldMat;
};

struct PBRObjectUniformParamater
{
	XMFLOAT4X4 mWorldMatrixParam;
	XMFLOAT4X4 mInvWorldMatrixParam;
};

class CPBRGeometryNode
{
public:
	CPBRGeometryNode();
	~CPBRGeometryNode();

	void InitResource(CGraphicContext* pContext);

	void SetPostion(const XMFLOAT3& v);
	void SetScale(const XMFLOAT3& v);

	void SetMesh(CPBRStaticMesh* pMesh);
	void Update();
	void UpdateRenderData();
	void Bind(CGraphicContext* pContext);
	void Draw(CGraphicContext* pContext);

private:
	CPBRStaticMesh* m_pMesh;
	PBRTransform m_Transform;

	PBRShaderDataBlock m_NodeShaderData;
};
