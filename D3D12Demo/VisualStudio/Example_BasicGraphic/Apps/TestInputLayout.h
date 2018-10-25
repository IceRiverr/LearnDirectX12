#pragma once
#include "WinApp.h"
#include <DirectXMath.h>
#include <vector>
#include <unordered_map>
#include "BufferManager.h"
#include "StaticMesh.h"
#include "Camera.h"
#include "InputManager.h"
#include "GraphicContext.h"

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
	virtual void Destroy();

	virtual LRESULT WndMsgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
	void InitRenderResource();
	void BuildStaticMeshes(ID3D12Device* pDevice, ID3D12GraphicsCommandList* cmdList);
	void BuildScene();
	void BuildPSOs(ID3D12Device* pDevice);
	void InitImgui();

	void UpdateFrameBuffer(float fDeltaTime, float fTotalTime);
	void UpdateImgui();

	void DrawImgui();

private:
	ID3D12DescriptorHeap * m_pCBVHeap;
	UINT m_CBVHeapSize;
	ID3D12RootSignature* m_pRootSignature;

	CObjectConstantBuffer m_ConstBuffer;
	CFrameBuffer m_FrameBuffer;

	ID3DBlob* m_pVSShaderCode;
	ID3DBlob* m_pPSShaderCode;

	ID3DBlob* m_pVSShaderCode_Position;
	ID3DBlob* m_pPSShaderCode_Position;

	std::vector<D3D12_INPUT_ELEMENT_DESC> m_InputLayout;
	std::vector<D3D12_INPUT_ELEMENT_DESC> m_SimplePositionInputLayout;

	std::unordered_map<std::string, CStaticMesh*> m_StaticMeshes;
	std::vector<CRenderObject*> m_RenderObjects;
	std::unordered_map<std::string, ID3D12PipelineState*> m_PSOs;

	CBaseCamera* m_pCamera;
	CInputManager m_InputMgr;

	UINT m_imguiDescriptorIndex;

	// Test imgui
	bool show_demo_window;
	bool show_another_window;
	XMFLOAT4 clear_color;

	CGraphicContext* m_pGraphicContext;
};
