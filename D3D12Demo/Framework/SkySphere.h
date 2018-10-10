#pragma once
#include "StaticMesh.h"
#include "GraphicContext.h"

class CSkySphere
{
public:
	void SetBGPath(const std::string& imagePath);
	void SetMesh(CRenderObject* pRender);
	void Init(CGraphicContext* pContext);
	void Draw(ID3D12GraphicsCommandList* cmdList);

private:
	CRenderObject* m_pRenderObj;

	ID3DBlob* m_pVSShaderCode_SkySphere;
	ID3DBlob* m_pPSShaderCode_SkySphere;

	ID3D12PipelineState* m_PSO;

	std::string m_BackGroundMapPath;
	Texture2DResource* m_pBackGroundMap;
};

