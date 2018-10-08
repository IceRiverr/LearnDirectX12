#pragma once
#include "StaticMesh.h"


class CSkySphere
{
public:
	void SetMesh(CRenderObject* pRender, Texture2DResource* pBGMap);
	void Init(ID3D12Device* pDevice, ID3D12GraphicsCommandList* cmdList, ID3D12RootSignature* pRootSignature);
	void Draw(ID3D12GraphicsCommandList* cmdList);

private:
	CRenderObject* m_pRenderObj;

	ID3DBlob* m_pVSShaderCode_SkySphere;
	ID3DBlob* m_pPSShaderCode_SkySphere;

	ID3D12PipelineState* m_PSO;

	Texture2DResource* m_pBackGroundMap;
};

