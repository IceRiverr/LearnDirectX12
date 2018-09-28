#include "Material_BRDF.h"
#include "ImportObj.h"

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"

CMaterialBRDFApp::CMaterialBRDFApp()
{
	m_pCBVHeap = nullptr;
	m_pRootSignature = nullptr;
	m_pVSShaderCode_Light = nullptr;
	m_pPSShaderCode_Light = nullptr;

	bool show_demo_window = false;
	bool show_another_window = false;
	clear_color = { 80.0f / 255.0f, 90.0f / 255.0f, 100.0f / 255.0f, 1.0f };
}

CMaterialBRDFApp::~CMaterialBRDFApp()
{
	// 内存析构都没有做，需要完善


}

void CMaterialBRDFApp::Init()
{
	WinApp::Init();
	InitRenderResource();
	InitImgui();

	m_pCamera = new CRotateCamera();
	m_pCamera->m_fRotateRadius = 5.0f;
	m_pCamera->Init(90.0f, m_nClientWindowWidth * 1.0f / m_nClientWindowHeight, 1.0f, 1000.0f);
}

void CMaterialBRDFApp::InitRenderResource()
{
	m_pCommandList->Reset(m_pCommandAllocator, nullptr);
	// 所有初始化命令都放到该命令之后

	BuildMaterials();
	BuildStaticMeshes(m_pDevice, m_pCommandList);
	BuildScene();

	// create cbv heap
	D3D12_DESCRIPTOR_HEAP_DESC cbHeapDesc = {};
	// obj + material + frameBuffer + imgui
	cbHeapDesc.NumDescriptors = (UINT)m_RenderObjects.size() + (UINT)m_Materials.size() + 1 + 1;
	cbHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cbHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	cbHeapDesc.NodeMask = 0;
	m_pDevice->CreateDescriptorHeap(&cbHeapDesc, IID_PPV_ARGS(&m_pCBVHeap));

	UINT nDescriptorIndex = 0;

	m_ObjectBuffer.CreateBuffer(m_pDevice, (UINT)m_RenderObjects.size());
	for (int i = 0; i < m_RenderObjects.size(); ++i)
	{
		m_RenderObjects[i]->m_ObjectAddress.nBufferIndex = i;
		m_RenderObjects[i]->m_ObjectAddress.pBuffer = m_ObjectBuffer.m_pUploadeConstBuffer;
		m_RenderObjects[i]->m_ObjectAddress.pBufferHeap = m_pCBVHeap;
		m_RenderObjects[i]->m_ObjectAddress.nHeapOffset = nDescriptorIndex;

		auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_pCBVHeap->GetCPUDescriptorHandleForHeapStart());
		handle.Offset(nDescriptorIndex, m_nSRVDescriptorSize);
		m_ObjectBuffer.CreateBufferView(m_pDevice, handle, i);
		
		nDescriptorIndex++;
	}

	m_MaterialBuffer.CreateBuffer(m_pDevice, (UINT)m_Materials.size());
	int nMaterialIndex = 0;
	for (auto it = m_Materials.begin(); it != m_Materials.end(); ++it)
	{
		it->second->m_MaterialAddress.nBufferIndex = nMaterialIndex;
		it->second->m_MaterialAddress.pBuffer = m_MaterialBuffer.m_pUploadeConstBuffer;
		it->second->m_MaterialAddress.pBufferHeap = m_pCBVHeap;
		it->second->m_MaterialAddress.nHeapOffset = nDescriptorIndex;

		auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_pCBVHeap->GetCPUDescriptorHandleForHeapStart());
		handle.Offset(nDescriptorIndex, m_nSRVDescriptorSize);
		m_MaterialBuffer.CreateBufferView(m_pDevice, handle, nMaterialIndex);

		nMaterialIndex++;
		nDescriptorIndex++;
	}

	m_FrameBuffer.CreateBuffer(m_pDevice);
	m_FrameBuffer.m_nDescriptorIndex = nDescriptorIndex++;
	auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_pCBVHeap->GetCPUDescriptorHandleForHeapStart());
	handle.Offset(m_FrameBuffer.m_nDescriptorIndex, m_nSRVDescriptorSize);
	m_FrameBuffer.CreateBufferView(m_pDevice, handle);

	m_imguiDescriptorIndex = nDescriptorIndex;

	// create root signature
	CD3DX12_DESCRIPTOR_RANGE cbvTable;
	cbvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);

	CD3DX12_DESCRIPTOR_RANGE cbvTable1;
	cbvTable1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1);

	CD3DX12_DESCRIPTOR_RANGE cbvTable2;
	cbvTable2.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 2);

	CD3DX12_ROOT_PARAMETER slotRootParameter[3];
	slotRootParameter[0].InitAsDescriptorTable(1, &cbvTable);
	slotRootParameter[1].InitAsDescriptorTable(1, &cbvTable1);
	slotRootParameter[2].InitAsDescriptorTable(1, &cbvTable2);

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(3, slotRootParameter, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ID3DBlob* serializedRootSig = nullptr;
	ID3DBlob* errorBlob = nullptr;
	D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &serializedRootSig, &errorBlob);

	m_pDevice->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(&m_pRootSignature));

	if (serializedRootSig)
	{
		serializedRootSig->Release(); serializedRootSig = nullptr;
	}

	m_PositionNormalInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL",	  0, DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	m_pVSShaderCode_Light = Graphics::CompileShader("D:\\Projects\\MyProjects\\LearnDirectX12\\D3D12Demo\\Shaders\\light_material.hlsl", "VSMain", "vs_5_0");
	m_pPSShaderCode_Light = Graphics::CompileShader("D:\\Projects\\MyProjects\\LearnDirectX12\\D3D12Demo\\Shaders\\light_material.hlsl", "PSMain", "ps_5_0");

	BuildPSOs(m_pDevice);

	m_pCommandList->Close();

	// flush command
	ID3D12CommandList* cmdLists[1] = { m_pCommandList };
	m_pCommandQueue->ExecuteCommandLists(1, cmdLists);
	FlushCommandQueue();
}

void CMaterialBRDFApp::BuildMaterials()
{
	CMaterial* pMat = new CMaterial();
	pMat->m_sName = "Blue";
	pMat->m_cDiffuseColor = XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);
	m_Materials.emplace(pMat->m_sName, pMat);
}

void CMaterialBRDFApp::Update(double deltaTime)
{
	static double dTotalTime = 0.0f;
	dTotalTime += deltaTime;

	if (m_InputMgr.IsKeyJustDown('F'))
	{
		std::cout << "F click" << std::endl;
	}

	if (m_InputMgr.IsKeyHoldOrDown('G'))
	{
		std::cout << "G hold" << std::endl;
	}

	if (m_InputMgr.IsKeyUp('G'))
	{
		std::cout << "G Up" << std::endl;
	}

	m_pCamera->Update(deltaTime, m_InputMgr);
	
	for (int i = 0; i < m_RenderObjects.size(); ++i)
	{
		XMMATRIX mWorldMat = m_RenderObjects[i]->m_mWorldMatrix;
		//XMMATRIX mRotateMat = XMMatrixRotationY((float)fTotalTime * (i + j + 1) * 0.2f);
		//mWorldMat = mRotateMat * mWorldMat;

		ConstantShaderBlock objConstant;
		XMStoreFloat4x4(&objConstant.mWorldMat, XMMatrixTranspose(mWorldMat));
		m_ObjectBuffer.UpdateBuffer((UINT8*)&objConstant, sizeof(objConstant), m_RenderObjects[i]->m_ObjectAddress.nBufferIndex);
	}

	for (auto it = m_Materials.begin(); it != m_Materials.end(); ++it)
	{
		CMaterial* pMat = it->second;
		
		MaterialShaderBlock matBlock;
		matBlock.DiffuseColor = pMat->m_cDiffuseColor;
		m_MaterialBuffer.UpdateBuffer((UINT8*)&matBlock, sizeof(matBlock), pMat->m_MaterialAddress.nBufferIndex);
	}

	UpdateFrameBuffer((float)deltaTime, (float)dTotalTime);

	UpdateImgui();

	m_InputMgr.ResetInputInfos();
}

void CMaterialBRDFApp::UpdateFrameBuffer(float fDeltaTime, float fTotalTime)
{
	m_FrameBuffer.m_FrameData = {};

	m_FrameBuffer.m_FrameData.g_mView = m_pCamera->m_CameraInfo.mViewMatrix;
	m_FrameBuffer.m_FrameData.g_mInvView = m_pCamera->m_CameraInfo.mInvViewMatrix;
	m_FrameBuffer.m_FrameData.g_mProj = m_pCamera->m_CameraInfo.mProjMatrix;
	m_FrameBuffer.m_FrameData.g_mInvProj = m_pCamera->m_CameraInfo.mInvProjMatrix;
	m_FrameBuffer.m_FrameData.g_mViewProj = m_pCamera->m_CameraInfo.mViewProjMatrix;
	m_FrameBuffer.m_FrameData.g_mInvViewProj = m_pCamera->m_CameraInfo.mInvViewProjMatrix;
	m_FrameBuffer.m_FrameData.g_vEyePosition = m_pCamera->m_CameraInfo.vEyePositon;
	m_FrameBuffer.m_FrameData.g_RenderTargetSize = { (float)m_nClientWindowWidth, (float)m_nClientWindowHeight };
	m_FrameBuffer.m_FrameData.g_InvRenderTargetSize = { 1.0f / (float)m_nClientWindowWidth, 1.0f / (float)m_nClientWindowHeight };
	m_FrameBuffer.m_FrameData.g_fNearZ = m_pCamera->m_CameraInfo.fNearZ;
	m_FrameBuffer.m_FrameData.g_fFarZ = m_pCamera->m_CameraInfo.fFarZ;
	m_FrameBuffer.m_FrameData.g_fTotalTime = std::fmodf(fTotalTime, 1.0f);
	m_FrameBuffer.m_FrameData.g_fDeltaTime = fDeltaTime;

	if (m_DirLights.size() + m_PointLights.size() + m_SpotLights.size() <= 16)
	{
		for (int i = 0; i < m_DirLights.size(); ++i)
		{
			LightInfo lightInfo = {};
			CDirectionalLight* pLight = m_DirLights[i];
			XMStoreFloat4(&lightInfo.LightDirection, pLight->m_vDirection);
			lightInfo.LightColor = pLight->m_Color;

			m_FrameBuffer.m_FrameData.g_Lights[i] = lightInfo;
		}

		int nPointLightStart = (int)m_DirLights.size();
		for (int i = 0; i < m_PointLights.size(); ++i)
		{
			LightInfo lightInfo = {};
			CPointLight* pLight = m_PointLights[i];
			XMStoreFloat4(&lightInfo.LightPosition, pLight->m_vPosition);
			lightInfo.RefDist = pLight->m_fRefDist;;
			lightInfo.MaxRadius = pLight->m_fMaxRadius;
			lightInfo.LightColor = pLight->m_Color;

			m_FrameBuffer.m_FrameData.g_Lights[nPointLightStart + i] = lightInfo;
		}

		int nSpotLightStart = (int)(nPointLightStart + m_PointLights.size());
		for (int i = 0; i < m_SpotLights.size(); ++i)
		{
			LightInfo lightInfo = {};
			CSpotLight* pLight = m_SpotLights[i];
			XMStoreFloat4(&lightInfo.LightDirection, pLight->m_vDirection);
			XMStoreFloat4(&lightInfo.LightPosition, pLight->m_vPosition);
			lightInfo.LightColor = pLight->m_Color;

			lightInfo.RefDist = pLight->m_fRefDist;
			lightInfo.MaxRadius = pLight->m_fMaxRadius;
			lightInfo.MinAngle = std::cosf(MathUtility::ToRadian(pLight->m_fMinAngle));
			lightInfo.MaxAngle = std::cosf(MathUtility::ToRadian(pLight->m_fMaxAngle));
			m_FrameBuffer.m_FrameData.g_Lights[nSpotLightStart + i] = lightInfo;
		}

		m_FrameBuffer.m_FrameData.g_LightNumbers[0] = (int)m_DirLights.size();
		m_FrameBuffer.m_FrameData.g_LightNumbers[1] = (int)m_PointLights.size();
		m_FrameBuffer.m_FrameData.g_LightNumbers[2] = (int)m_SpotLights.size();
		m_FrameBuffer.m_FrameData.g_LightNumbers[3] = 0;
	}

	memcpy(m_FrameBuffer.m_pCbvDataBegin, &m_FrameBuffer.m_FrameData, m_FrameBuffer.m_nConstantBufferSizeAligned);
}

void CMaterialBRDFApp::UpdateImgui()
{
	// Start the Dear ImGui frame
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	// 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
	if (show_demo_window)
		ImGui::ShowDemoWindow(&show_demo_window);

	// 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
	{
		static float f = 0.0f;
		static int counter = 0;

		ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

		ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
		ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
		ImGui::Checkbox("Another Window", &show_another_window);

		ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f    
		ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

		if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
			counter++;
		ImGui::SameLine();
		ImGui::Text("counter = %d", counter);

		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::End();
	}

	// 3. Show another simple window.
	if (show_another_window)
	{
		ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
		ImGui::Text("Hello from another window!");
		if (ImGui::Button("Close Me"))
			show_another_window = false;
		ImGui::End();
	}
}

void CMaterialBRDFApp::DrawImgui()
{
	ImGui::Render();
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_pCommandList);
}

void CMaterialBRDFApp::Draw()
{
	m_pCommandAllocator->Reset();

	m_pCommandList->Reset(m_pCommandAllocator, m_PSOs["Light_PSO"]);

	m_pCommandList->RSSetViewports(1, &m_ScreenViewPort);
	m_pCommandList->RSSetScissorRects(1, &m_ScissorRect);

	m_pCommandList->ResourceBarrier(
		1, &CD3DX12_RESOURCE_BARRIER::Transition(
			GetCurrentBackBuffer(),
			D3D12_RESOURCE_STATE_PRESENT,
			D3D12_RESOURCE_STATE_RENDER_TARGET));

	// Begin Draw
	m_pCommandList->ClearRenderTargetView(
		GetCurrentBackBufferView(),
		(float*)&clear_color, 0, nullptr);

	m_pCommandList->ClearDepthStencilView(
		GetDepthStencilView(),
		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
		1.0, 0, 0, nullptr);

	m_pCommandList->OMSetRenderTargets(1, &GetCurrentBackBufferView(), true, &GetDepthStencilView());

	ID3D12DescriptorHeap* pDescriptorHeap[] = { m_pCBVHeap };
	m_pCommandList->SetDescriptorHeaps(1, pDescriptorHeap);
	m_pCommandList->SetGraphicsRootSignature(m_pRootSignature);

	auto handle = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_pCBVHeap->GetGPUDescriptorHandleForHeapStart());
	handle.Offset(m_FrameBuffer.m_nDescriptorIndex, m_nSRVDescriptorSize);
	m_pCommandList->SetGraphicsRootDescriptorTable(1, handle);

	for (int i = 0; i < m_RenderObjects.size(); ++i)
	{
		CRenderObject* pObj = m_RenderObjects[i];

		m_pCommandList->SetPipelineState(m_PSOs["Light_PSO"]);
		D3D12_VERTEX_BUFFER_VIEW VertexBufferViews[] =
		{
			pObj->m_pStaticMesh->m_PositionBufferView,
			pObj->m_pStaticMesh->m_NormalBufferView,
		};

		m_pCommandList->IASetVertexBuffers(0, 2, &VertexBufferViews[0]);
		m_pCommandList->IASetIndexBuffer(&pObj->m_pStaticMesh->m_IndexBufferView);
		m_pCommandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		auto handle1 = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_pCBVHeap->GetGPUDescriptorHandleForHeapStart());
		handle1.Offset(pObj->m_ObjectAddress.nHeapOffset, m_nSRVDescriptorSize);
		m_pCommandList->SetGraphicsRootDescriptorTable(0, handle1);

		auto handle2 = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_pCBVHeap->GetGPUDescriptorHandleForHeapStart());
		handle2.Offset(pObj->m_pStaticMesh->m_pMaterial->m_MaterialAddress.nHeapOffset, m_nSRVDescriptorSize);
		m_pCommandList->SetGraphicsRootDescriptorTable(2, handle2);

		for (auto it : pObj->m_pStaticMesh->m_SubMeshes)
		{
			SubMesh subMesh = it.second;
			m_pCommandList->DrawIndexedInstanced(subMesh.nIndexCount, 1, subMesh.nStartIndexLocation, subMesh.nBaseVertexLocation, 0);
		}
	}

	// Imgui
	DrawImgui();

	// End Draw
	m_pCommandList->ResourceBarrier(
		1, &CD3DX12_RESOURCE_BARRIER::Transition(
			GetCurrentBackBuffer(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PRESENT));

	m_pCommandList->Close();

	ID3D12CommandList* cmdLists[1] = { m_pCommandList };

	m_pCommandQueue->ExecuteCommandLists(1, cmdLists);

	m_pSwapChian->Present(0, 0);

	m_nCurrentSwapChainIndex = (m_nCurrentSwapChainIndex + 1) % m_nSwapChainBufferCount;

	FlushCommandQueue();
}

void CMaterialBRDFApp::OnResize()
{
	WinApp::OnResize();
	ImGui_ImplDX12_InvalidateDeviceObjects();
	ImGui_ImplDX12_CreateDeviceObjects();

	if (m_pCamera)
	{
		m_pCamera->SetAspectRatio(m_nClientWindowWidth * 1.0f / m_nClientWindowHeight);
	}
}

void CMaterialBRDFApp::Destroy()
{
	WinApp::Destroy();
	FlushCommandQueue();

	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CMaterialBRDFApp::WndMsgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
		return true;

	switch (message)
	{
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);
		// TODO: 在此处添加使用 hdc 的任何绘图代码...
		EndPaint(hWnd, &ps);
		return 0;
	}
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_ACTIVATE:
		if (LOWORD(wParam) == WA_INACTIVE)
		{
			std::cout << "InActive" << std::endl;
		}
		else
		{
			std::cout << "Active" << std::endl;
		}
		return 0;
	case WM_SIZE:
	{
		RECT clientWindow = {};
		GetClientRect(m_hWnd, &clientWindow);

		int windowWidth = (int)(clientWindow.right - clientWindow.left);
		int windowHeight = (int)(clientWindow.bottom - clientWindow.top);

		if (m_nClientWindowWidth != windowWidth || m_nClientWindowHeight != windowHeight)
		{
			m_nClientWindowWidth = windowWidth;
			m_nClientWindowHeight = windowHeight;
			OnResize();
		}
		return 0;
	}
	case WM_ENTERSIZEMOVE:
		// 用户开始拖动resize bar
		return 0;
	case WM_EXITSIZEMOVE:
		OnResize();
		// 用户停止拖动
		return 0;
	case WM_SYSKEYDOWN:
	case WM_KEYDOWN:
	{
		KeyInfo keyState = GetKeyInfo(wParam, lParam);
		keyState.state = KeyState::Down;
		m_InputMgr.m_KeyInfos[keyState.key] = keyState;

		switch (wParam)
		{
		case VK_ESCAPE:
			PostQuitMessage(0);
			break;
		case VK_F12: // 这个命令会导致断点，不知道什么原因
					 //SetFullScreen(!m_bFullScreen);
			break;
		case 'P':
			SetFullScreen(!m_bFullScreen);
			break;
		case 'A':
			//std::cout << "A";
			break;
		}
		return 0;
	}
	case WM_SYSKEYUP:
	case WM_KEYUP:
	{
		KeyInfo keyState = GetKeyInfo(wParam, lParam);
		keyState.state = KeyState::Up;
		m_InputMgr.m_KeyInfos[keyState.key] = keyState;

		return 0;
	}
	case WM_LBUTTONDOWN:
	{
		//std::cout << "L Button Down\n";
		return 0;
	}
	case WM_RBUTTONDOWN:
	{
		//std::cout << "R Button Down\n";
		return 0;
	}
	case WM_MOUSEMOVE:
	{
		int x = LOWORD(lParam);//鼠标的横坐标
		int y = HIWORD(lParam);//鼠标的纵坐标

		m_InputMgr.m_nMouseX = x;
		m_InputMgr.m_nMouseY = x;

		m_InputMgr.m_nDelteMouseX = x - m_InputMgr.m_nLastMouseX;
		m_InputMgr.m_nDeltaMouseY = y - m_InputMgr.m_nLastMouseY;

		m_InputMgr.m_nLastMouseX = x;
		m_InputMgr.m_nLastMouseY = y;
		return 0;
	}
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
}

void CMaterialBRDFApp::BuildStaticMeshes(ID3D12Device* pDevice, ID3D12GraphicsCommandList* cmdList)
{
	
	// Obj
	{
		CStaticMesh* pMesh = new CStaticMesh();
		m_StaticMeshes.emplace("Plane_Obj", pMesh);
		CImportor_Obj impoortor;
		impoortor.SetPath("D:\\Projects\\MyProjects\\LearnDirectX12\\D3D12Demo\\Content\\UVSphere.obj"); // smooth_box plane  scene_simple
		impoortor.Import();

		MeshData* pMeshData = impoortor.m_MeshObjs[0];

		pMesh->m_pPositionBufferGPU = CreateDefaultBuffer(pDevice, cmdList, &pMeshData->Positions[0], pMeshData->Positions.size() * sizeof(XMFLOAT3), &pMesh->m_pPositionBufferUpload);
		pMesh->m_pNormalBufferGPU = CreateDefaultBuffer(pDevice, cmdList, &pMeshData->Normals[0], pMeshData->Normals.size() * sizeof(XMFLOAT3), &pMesh->m_pNormalBufferUpload);
		pMesh->m_pIndexBuferGPU = CreateDefaultBuffer(pDevice, cmdList, &pMeshData->Indices[0], (UINT)pMeshData->Indices.size() * sizeof(UINT), &pMesh->m_pIndexBufferUpload);

		pMesh->m_PositionBufferView = Graphics::CreateVertexBufferView(pMesh->m_pPositionBufferGPU, (UINT)pMeshData->Positions.size() * sizeof(XMFLOAT3), sizeof(XMFLOAT3));
		pMesh->m_NormalBufferView = Graphics::CreateVertexBufferView(pMesh->m_pNormalBufferGPU, (UINT)pMeshData->Normals.size() * sizeof(XMFLOAT3), sizeof(XMFLOAT3));
		pMesh->m_IndexBufferView = Graphics::CreateIndexBufferView(pMesh->m_pIndexBuferGPU, (UINT)pMeshData->Indices.size() * sizeof(UINT), DXGI_FORMAT_R32_UINT);

		pMesh->AddSubMesh("Sub0", (UINT)pMeshData->Indices.size(), 0, 0);
		pMesh->m_pMaterial = m_Materials["Blue"];

		impoortor.Clear();
	}
}

void CMaterialBRDFApp::BuildScene()
{
	{
		CStaticMesh* pSphereMesh = m_StaticMeshes["Plane_Obj"];
		if (pSphereMesh)
		{
			CRenderObject* pObj = new CRenderObject();
			pObj->m_pStaticMesh = pSphereMesh;
			pObj->m_WorldTransform.Position = XMFLOAT3(0.0f, 0.0f, 0.0f);
			pObj->m_mWorldMatrix = XMMatrixTranslation(0.0f, 0.0f, 0.0f);
			m_RenderObjects.push_back(pObj);
		}
	}

	{
		CDirectionalLight* pLight = new CDirectionalLight();
		m_DirLights.push_back(pLight);
		pLight->m_Color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		pLight->m_vDirection = XMVectorSet(0.0f, -1.0f, 0.0f, 1.0f);
	}
}

void CMaterialBRDFApp::BuildPSOs(ID3D12Device* pDevice)
{
	ID3D12PipelineState* pOpaquePSO = nullptr;
	D3D12_GRAPHICS_PIPELINE_STATE_DESC OpaquePSODesc = {};
	OpaquePSODesc.InputLayout = { m_PositionNormalInputLayout.data(), (UINT)m_PositionNormalInputLayout.size() };
	OpaquePSODesc.pRootSignature = m_pRootSignature;
	OpaquePSODesc.VS = { m_pVSShaderCode_Light->GetBufferPointer(), m_pVSShaderCode_Light->GetBufferSize() };
	OpaquePSODesc.PS = { m_pPSShaderCode_Light->GetBufferPointer(), m_pPSShaderCode_Light->GetBufferSize() };
	OpaquePSODesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	OpaquePSODesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	OpaquePSODesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	OpaquePSODesc.SampleMask = UINT_MAX;
	OpaquePSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	OpaquePSODesc.NumRenderTargets = 1;
	OpaquePSODesc.RTVFormats[0] = m_BackBufferFromat;
	OpaquePSODesc.SampleDesc.Count = 1;
	OpaquePSODesc.SampleDesc.Quality = 0;
	OpaquePSODesc.DSVFormat = m_DSVFormat;
	//OpaquePSODesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	//OpaquePSODesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

	pDevice->CreateGraphicsPipelineState(&OpaquePSODesc, IID_PPV_ARGS(&pOpaquePSO));
	m_PSOs.emplace("Light_PSO", pOpaquePSO);
}

void CMaterialBRDFApp::InitImgui()
{
	// Setup Dear ImGui binding
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;

	ImGui_ImplWin32_Init(m_hWnd);
	
	auto cpuHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_pCBVHeap->GetCPUDescriptorHandleForHeapStart());
	cpuHandle.Offset(m_imguiDescriptorIndex, m_nSRVDescriptorSize);
	
	auto gpuHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_pCBVHeap->GetGPUDescriptorHandleForHeapStart());
	gpuHandle.Offset(m_imguiDescriptorIndex, m_nSRVDescriptorSize);

	ImGui_ImplDX12_Init(m_pDevice, 1, DXGI_FORMAT_R8G8B8A8_UNORM, cpuHandle, gpuHandle);

	// Setup style
	ImGui::StyleColorsDark();
}
