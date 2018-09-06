#pragma once
#include "WinApp.h"
#include <DirectXMath.h>
#include <vector>
#include <unordered_map>
#include "BufferManager.h"
#include "StaticMesh.h"

class TestInputLayoutApp :
	public WinApp
{
public:
	TestInputLayoutApp();
	~TestInputLayoutApp();

	virtual void Init();
	virtual void Update(double deltaTime);
	virtual void Draw();
	virtual void OnResize();

private:
	void BuildStaticMeshes(ID3D12Device* pDevice, ID3D12GraphicsCommandList* cmdList);
	void BuildScene();
	void BuildPSOs(ID3D12Device* pDevice);

	
private:
	ID3D12DescriptorHeap * m_pCBVHeap;
	UINT m_CBVHeapSize;
	ID3D12RootSignature* m_pRootSignature;

	CConstantBuffer m_ConstBuffer;
	CFrameBuffer m_FrameBuffer;

	ID3DBlob* m_pVSShaderCode;
	ID3DBlob* m_pPSShaderCode;

	std::vector<D3D12_INPUT_ELEMENT_DESC> m_InputLayout;

	std::unordered_map<std::string, CStaticMesh*> m_StaticMeshes;
	std::vector<CRenderObject*> m_RenderObjects;
	std::unordered_map<std::string, ID3D12PipelineState*> m_PSOs;

	XMFLOAT4X4A m_ProjMat;
};
