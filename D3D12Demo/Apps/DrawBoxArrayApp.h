#pragma once
#include "DemoApp.h"
#include <DirectXMath.h>
#include <vector>
#include "FrameBuffer.h"

class DrawBoxArrayApp :
	public DemoApp
{
public:
	struct Vertex
	{
		XMFLOAT3 position;
		XMFLOAT4 color;
	};

	struct ObjectConstants
	{
		XMFLOAT4X4 mWorldMat;

	};

	DrawBoxArrayApp();
	~DrawBoxArrayApp();

	virtual void Init();
	virtual void Update(double deltaTime);
	virtual void Draw();
	virtual void OnResize();

private:
	ID3D12DescriptorHeap * m_pCBVHeap;
	ID3D12RootSignature* m_pRootSignature;

	UINT m_nTotalConstantBuferByteSize;
	UINT m_nConstantBufferSizeAligned;
	ID3D12Resource* m_pUploadeConstBuffer;
	UINT8* m_pCbvDataBegin;

	CFrameBuffer m_FrameBuffer;

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
	int m_nBoxCount;
};

