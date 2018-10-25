#pragma once
#include "WinApp.h"
#include <DirectXMath.h>
#include <vector>
#include "GraphicContext.h"

class DrawBoxApp :
	public WinApp
{
public:
	struct Vertex
	{
		XMFLOAT3 position;
		XMFLOAT4 color;
	};

	struct ObjectConstants
	{
		XMFLOAT4X4 WorldViewProj;

	};

	DrawBoxApp();
	~DrawBoxApp();

	virtual void Init();
	virtual void Update(double deltaTime);
	virtual void Draw();
	virtual void OnResize();

private:
	ID3D12DescriptorHeap * m_pCBVHeap;
	ID3D12RootSignature* m_pRootSignature;

	UINT m_nConstantBuferByteSize;
	ID3D12Resource* m_pUploadeConstBuffer;
	ObjectConstants m_ConstantBufferData;
	UINT8* m_pCbvDataBegin;

	ID3DBlob* m_pVSShaderCode;
	ID3DBlob* m_pPSShaderCode;

	std::vector<D3D12_INPUT_ELEMENT_DESC> m_InputLayout;
	ID3D12PipelineState* m_pPSO;

	ID3D12Resource* m_pVertexBufferGPU;
	ID3D12Resource* m_pVertexBufferUpload;
	ID3D12Resource* m_pIndexBuferGPU;
	ID3D12Resource* m_pIndexBufferUpload;

	D3D12_VERTEX_BUFFER_VIEW m_vbView;
	D3D12_INDEX_BUFFER_VIEW m_ibView;

	UINT m_nBoxIndexCount;
	XMFLOAT4X4A m_ProjMat;
	UINT m_nBoxCount;

	CGraphicContext* m_pGraphicContext;
};

