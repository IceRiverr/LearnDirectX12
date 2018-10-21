#pragma once
#include "StaticMesh.h"
#include "GraphicContext.h"

// Equirectangular mapping
// https://en.wikipedia.org/wiki/Equirectangular_projection

void convert_xyz_to_equirectangular_uv(float x, float y, float z, float* u, float* v);

class CSkySphere
{
public:
	void SetMesh(CRenderObject* pRender);
	void Init(CGraphicContext* pContext);
	void Draw(ID3D12GraphicsCommandList* cmdList);

public:
	Texture2DResource * m_pBackGroundMap;
	Texture2DResource* m_pEnvironmentMap;
	Texture2DResource* m_pReflectionMap;

private:
	CRenderObject* m_pRenderObj;

	ID3DBlob* m_pVSShaderCode_SkySphere;
	ID3DBlob* m_pPSShaderCode_SkySphere;

	ID3D12PipelineState* m_PSO;
};

