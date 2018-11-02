
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

struct PBRMaterialShaderBlockDef
{
	PBRMaterialShaderBlockDef();

	XMFLOAT4 BaseColorFactor;
	XMFLOAT4 EmissiveColorFactor;

	float MetallicFactor;
	float RoughnessFactor;
	float NormalScale;
	float OcclusionStrength;
};

struct BufferBlock
{
	unsigned char* Data;
	UINT BindPoint;
	UINT SignatureSlot;
};

class CShader
{
public:
	CShader(ID3DBlob* code);
	~CShader();

	ID3DBlob* GetShaderCode() { return m_pShaderCode; }
	
	void Reflect();
	void SetFloat(std::string& name, float value);
	
	const D3D12_SHADER_BUFFER_DESC* FindUniformBufferDesc(const std::string& bufferName) const;

public:
	std::unordered_map<std::string, D3D12_SHADER_VARIABLE_DESC> m_ShaderValiableMap;
	std::vector<D3D12_SIGNATURE_PARAMETER_DESC> m_InputParameters; // 可以用来推导InputLayout
	std::vector<D3D12_SHADER_BUFFER_DESC> m_ConstantBuffers;
	std::vector<D3D12_SHADER_INPUT_BIND_DESC> m_BoundResources;

private:
	ID3DBlob * m_pShaderCode;
};

struct RenderPass
{
	void Reflect();
	void MergeParameters();

	std::shared_ptr<CShader> m_pVSShaderCode;
	std::shared_ptr<CShader> m_pPSShaderCode;
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
	void SetRootSignature(ID3D12RootSignature* rootSignature);
	UINT64 CompileShader(CGraphicContext* pContext, const PBRMaterialMacroInfo& info);
	
	bool ExportPreprocessShader(const std::string& inPath, const std::string& outPath, D3D_SHADER_MACRO* pDefines);

	RenderPass* GetShader(UINT64 MatId);

	static UINT64 CalcPBRMaterialID(const PBRMaterialMacroInfo& info);
	static std::vector<std::string> CalcPBRMaterialMacro(UINT64 MatId);
	static INPUT_LAYOUT_TYPE CalcInputLayoutType(const PBRMaterialMacroInfo& info);

public:
	std::unordered_map<UINT64, RenderPass*> m_ShaderMap;
	std::string m_sShaderPath;
	ID3D12RootSignature* m_pRootSignature;
};

class CPBRMaterial
{
public:
	CPBRMaterial(CPBRRenderEffect* pEffect);
	
	void InitResource(CGraphicContext* pContext);

	const D3D12_SHADER_VARIABLE_DESC* GetShaderVariableDesc(const std::string& name) const;
	const D3D12_SHADER_BUFFER_DESC* GetShaderBufferDesc(const std::string& name) const;
	UINT GetRootSignatureSlot(const std::string & name) const;

	// 如果同一个材质被附着给不同的对象，则应该怎么处理不同的InputLayout，这样处理肯定有问题
	void AttachToMesh(CStaticMesh* pMesh, CGraphicContext* pContext);

	void Update();
	void Bind(CGraphicContext* pContext, CRenderObject* pObj);

public:
	std::string m_Name;

	PBRMaterialShaderBlockDef m_ShaderBlock;

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

	// 应该还有一段Buffer块
	BufferBlock m_MatBlock;

	CPBRRenderEffect* m_pEffect;
	UINT64 m_MaterialID;
	
	// 放在这里有问题，先保证正确
	RenderPass* m_pRenderPass;
	CPBRRenderEffect::PBRMaterialMacroInfo m_MacroInfo;
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
	void BindMesh(CGraphicContext* pContext);

private:
	INPUT_LAYOUT_TYPE m_DataLayout;

	VertexAttribute m_PositonAttribute;
	VertexAttribute m_NormalAttribute;
	VertexAttribute m_UVAttribute;
	VertexAttribute m_TangentAttribute;

	IndexAttribute m_IndexAttribute;
	
	std::vector<PBRSubMesh> m_SubMeshes;
	std::vector<CPBRMaterial*> m_pMaterials;
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

class CPBRGeometryNode
{
public:
	CPBRGeometryNode();
	~CPBRGeometryNode();

	void InitResource(CGraphicContext* pContext);

	void SetPostion(const XMFLOAT3& v);
	void SetScale(const XMFLOAT3& v);

	void SetMesh(CPBRStaticMesh* pMesh);
	void SetMaterial(CPBRMaterial* pMat);
	void Update();
	void Draw(CGraphicContext* pContext);

private:
	CPBRStaticMesh* m_pMesh;
	CPBRMaterial* m_pMaterial;
	PBRTransform m_Transform;

	PBRShaderDataBlock m_NodeShaderData;
};

typedef TBaseConstantBuffer<PBRMaterialShaderBlockDef> CPBRMaterialConstantBuffer;