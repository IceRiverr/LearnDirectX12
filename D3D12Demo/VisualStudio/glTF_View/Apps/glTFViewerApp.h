#pragma once
#include "WinApp.h"
#include <DirectXMath.h>
#include <vector>
#include <unordered_map>
#include "BufferManager.h"
#include "StaticMesh.h"
#include "Camera.h"
#include "InputManager.h"
#include "Light.h"
#include "Material.h"
#include "SkySphere.h"
#include "GraphicContext.h"
#include "SkyBox.h"

class CGLTFViewerApp :
	public WinApp
{
public:
	CGLTFViewerApp();
	~CGLTFViewerApp();

	virtual void Init();
	virtual void Update(double deltaTime);
	virtual void Draw();
	virtual void OnResize();
	virtual void Destroy();

	virtual LRESULT WndMsgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
	void BuildRootSignature();
	void BuildPSOs(ID3D12Device* pDevice);
	void BuildMaterials();
	void BuildStaticMeshes(ID3D12Device* pDevice, ID3D12GraphicsCommandList* cmdList);
	void BuildScene();
	void BuildHeapDescriptors();
	void InitImgui();

	void UpdateFrameBuffer(float fDeltaTime, float fTotalTime);
	void UpdateImgui();

	void DrawImgui();

	void TEST_AREA();

private:
	std::string m_ContentRootPath;
	std::string m_ShaderRootPath;

	ID3D12RootSignature* m_pRootSignature;

	CObjectConstantBuffer m_ObjectBuffer;
	CMaterialConstantBuffer m_MaterialBuffer;
	CFrameBuffer m_FrameBuffer;
	
	ID3DBlob* m_pVSShaderCode_Light;
	ID3DBlob* m_pPSShaderCode_Light;
	
	ID3DBlob* m_pVSShaderCode_Material;
	ID3DBlob* m_pPSShaderCode_Material;

	std::vector<CRenderObject*> m_RenderObjects;
	std::vector<CDirectionalLight*> m_DirLights;
	std::vector<CPointLight*> m_PointLights;
	std::vector<CSpotLight*> m_SpotLights;

	std::unordered_map<std::string, ID3D12PipelineState*> m_PSOs;
	std::unordered_map<std::string, CMaterial*> m_Materials;
	std::unordered_map<std::string, CStaticMesh*> m_StaticMeshes;

	CFPSCamera* m_pCamera;
	CInputManager m_InputMgr;

	bool m_bGuiMode;

	XMFLOAT4 clear_color;
	CMaterial* m_pBRDFMat;

	//CSkySphere* m_pSkySphere;
	CSkyBox* m_pSkyBox;

	CGraphicContext* m_pGraphicContext;
};
