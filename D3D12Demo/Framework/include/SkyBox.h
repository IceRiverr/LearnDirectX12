#pragma once
#include "stdafx.h"
#include "StaticMesh.h"

// https://en.wikipedia.org/wiki/Cube_mapping
// Cube Mapping

void convert_cube_uv_to_xyz(int index, float u, float v, float* x, float* y, float* z);

void convert_xyz_to_cube_uv(float x, float y, float z, int* index, float* u, float* v);

class CSkyBox
{
public:
	void SetMesh(CRenderObject* pRender);
	void Init(CGraphicContext* pContext);
	void Draw(ID3D12GraphicsCommandList* cmdList);

	void BuildCubeMap(CGraphicContext * pContext);

private:
	ID3DBlob* m_pVSShaderCode;
	ID3DBlob* m_pPSShaderCode;
	ID3D12PipelineState* m_PSO;

	CRenderObject* m_pRenderObj;

	ID3D12Resource* m_pCubeMap;
	ID3D12Resource* m_pUpdateBuffer[6];
	ConstantBufferAddress m_TextureAddress;
};
