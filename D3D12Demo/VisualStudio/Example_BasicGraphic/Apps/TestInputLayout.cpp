#include "TestInputLayout.h"
#include "ImportObj.h"

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"

TestInputLayoutApp::TestInputLayoutApp()
{
	m_pCBVHeap = nullptr;
	m_pRootSignature = nullptr;
	m_pVSShaderCode = nullptr;
	m_pPSShaderCode = nullptr;

	bool show_demo_window = false;
	bool show_another_window = false;
	clear_color = { 80.0f / 255.0f, 90.0f / 255.0f, 100.0f / 255.0f, 1.0f };
}

TestInputLayoutApp::~TestInputLayoutApp()
{
	
}

void TestInputLayoutApp::Init()
{
	WinApp::Init();
	InitRenderResource();
	InitImgui();

	m_pCamera = new CRotateCamera();
	m_pCamera->Init(90.0f, m_nClientWindowWidth * 1.0f / m_nClientWindowHeight, 1.0f, 1000.0f);
}

void TestInputLayoutApp::InitRenderResource()
{
	m_pCommandList->Reset(m_pCommandAllocator, nullptr);
	// 所有初始化命令都放到该命令之后

	BuildStaticMeshes(m_pDevice, m_pCommandList);
	BuildScene();

	// create cbv heap
	D3D12_DESCRIPTOR_HEAP_DESC cbHeapDesc = {};
	// obj + frameBuffer + imgui
	cbHeapDesc.NumDescriptors = (UINT)m_RenderObjects.size() + 1 + 1;
	cbHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cbHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	cbHeapDesc.NodeMask = 0;
	m_pDevice->CreateDescriptorHeap(&cbHeapDesc, IID_PPV_ARGS(&m_pCBVHeap));

	UINT nDescriptorIndex = 0;
	m_ConstBuffer.CreateBuffer(m_pDevice, (UINT)m_RenderObjects.size());
	for (int i = 0; i < m_RenderObjects.size(); ++i)
	{
		m_RenderObjects[i]->m_ObjectAddress.nBufferIndex = i;
		m_RenderObjects[i]->m_ObjectAddress.pBuffer = m_ConstBuffer.m_pUploadeConstBuffer;
		m_RenderObjects[i]->m_ObjectAddress.pBufferHeap = m_pCBVHeap;

		auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_pCBVHeap->GetCPUDescriptorHandleForHeapStart());
		handle.Offset(nDescriptorIndex, m_nSRVDescriptorSize);
		m_ConstBuffer.CreateBufferView(m_pDevice, handle, i);

		auto ghandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_pCBVHeap->GetGPUDescriptorHandleForHeapStart());
		ghandle.Offset(nDescriptorIndex, m_nSRVDescriptorSize);

		m_RenderObjects[i]->m_ObjectAddress.CPUHandle = handle;
		m_RenderObjects[i]->m_ObjectAddress.GPUHandle = ghandle;

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

	CD3DX12_ROOT_PARAMETER slotRootParameter[2];
	slotRootParameter[0].InitAsDescriptorTable(1, &cbvTable);
	slotRootParameter[1].InitAsDescriptorTable(1, &cbvTable1);

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(2, slotRootParameter, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

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

	m_pVSShaderCode = Graphics::CompileShader("D:\\Projects\\MyProjects\\LearnDirectX12\\D3D12Demo\\Shaders\\color_view_info.fx", "VSMain", "vs_5_0");
	m_pPSShaderCode = Graphics::CompileShader("D:\\Projects\\MyProjects\\LearnDirectX12\\D3D12Demo\\Shaders\\color_view_info.fx", "PSMain", "ps_5_0");

	m_pVSShaderCode_Position = Graphics::CompileShader("D:\\Projects\\MyProjects\\LearnDirectX12\\D3D12Demo\\Shaders\\position_color.fx", "VSMain", "vs_5_0");
	m_pPSShaderCode_Position = Graphics::CompileShader("D:\\Projects\\MyProjects\\LearnDirectX12\\D3D12Demo\\Shaders\\position_color.fx", "PSMain", "ps_5_0");

	m_InputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	m_SimplePositionInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	BuildPSOs(m_pDevice);

	m_pCommandList->Close();

	// flush command
	ID3D12CommandList* cmdLists[1] = { m_pCommandList };
	m_pCommandQueue->ExecuteCommandLists(1, cmdLists);
	FlushCommandQueue();
}

void TestInputLayoutApp::Update(double deltaTime)
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

	// 渲染10x10 100个方格
	for (int i = 0; i < m_RenderObjects.size(); ++i)
	{
		XMMATRIX mWorldMat = m_RenderObjects[i]->m_mWorldMatrix;
		//XMMATRIX mRotateMat = XMMatrixRotationY((float)fTotalTime * (i + j + 1) * 0.2f);
		//mWorldMat = mRotateMat * mWorldMat;

		ObjectShaderBlock objConstant;
		XMStoreFloat4x4(&objConstant.mWorldMat, XMMatrixTranspose(mWorldMat));
		m_ConstBuffer.UpdateBuffer((UINT8*)&objConstant, sizeof(objConstant), m_RenderObjects[i]->m_ObjectAddress.nBufferIndex);
	}

	UpdateFrameBuffer((float)deltaTime, (float)dTotalTime);

	UpdateImgui();

	m_InputMgr.ResetInputInfos();
}

void TestInputLayoutApp::UpdateFrameBuffer(float fDeltaTime, float fTotalTime)
{
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
	memcpy(m_FrameBuffer.m_pCbvDataBegin, &m_FrameBuffer.m_FrameData, m_FrameBuffer.m_nConstantBufferSizeAligned);
}

void TestInputLayoutApp::UpdateImgui()
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

void TestInputLayoutApp::DrawImgui()
{
	ImGui::Render();
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_pCommandList);
}

void TestInputLayoutApp::Draw()
{
	m_pCommandAllocator->Reset();

	m_pCommandList->Reset(m_pCommandAllocator, m_PSOs["WireframePSO"]);

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
	m_pCommandList->SetGraphicsRootDescriptorTable(0, handle);

	for (int i = 0; i < m_RenderObjects.size(); ++i)
	{
		CRenderObject* pObj = m_RenderObjects[i];
		if (pObj->m_pStaticMesh->m_pPositionBufferGPU && pObj->m_pStaticMesh->m_pVertexColorBufferGPU)
		{
			m_pCommandList->SetPipelineState(m_PSOs["WireframePSO"]);
			D3D12_VERTEX_BUFFER_VIEW VertexBufferViews[2] =
			{
				pObj->m_pStaticMesh->m_PositionBufferView,
				pObj->m_pStaticMesh->m_VertexColorBufferView
			};

			m_pCommandList->IASetVertexBuffers(0, 2, &VertexBufferViews[0]);
			m_pCommandList->IASetIndexBuffer(&pObj->m_pStaticMesh->m_IndexBufferView);
			m_pCommandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			m_pCommandList->SetGraphicsRootDescriptorTable(1, pObj->m_ObjectAddress.GPUHandle);

			for (auto it : pObj->m_pStaticMesh->m_SubMeshes)
			{
				SubMesh subMesh = it.second;
				m_pCommandList->DrawIndexedInstanced(subMesh.nIndexCount, 1, subMesh.nStartIndexLocation, subMesh.nBaseVertexLocation, 0);
			}
		}
		else if (pObj->m_pStaticMesh->m_pPositionBufferGPU)
		{
			m_pCommandList->SetPipelineState(m_PSOs["SimplePosition_PSO"]);
			D3D12_VERTEX_BUFFER_VIEW VertexBufferViews[] =
			{
				pObj->m_pStaticMesh->m_PositionBufferView,
			};

			m_pCommandList->IASetVertexBuffers(0, 1, &VertexBufferViews[0]);
			m_pCommandList->IASetIndexBuffer(&pObj->m_pStaticMesh->m_IndexBufferView);
			m_pCommandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			m_pCommandList->SetGraphicsRootDescriptorTable(1, pObj->m_ObjectAddress.GPUHandle);

			for (auto it : pObj->m_pStaticMesh->m_SubMeshes)
			{
				SubMesh subMesh = it.second;
				m_pCommandList->DrawIndexedInstanced(subMesh.nIndexCount, 1, subMesh.nStartIndexLocation, subMesh.nBaseVertexLocation, 0);
			}
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

void TestInputLayoutApp::OnResize()
{
	WinApp::OnResize();
	ImGui_ImplDX12_InvalidateDeviceObjects();
	ImGui_ImplDX12_CreateDeviceObjects();

	if (m_pCamera)
	{
		m_pCamera->SetAspectRatio(m_nClientWindowWidth * 1.0f / m_nClientWindowHeight);
	}
}

void TestInputLayoutApp::Destroy()
{
	WinApp::Destroy();
	FlushCommandQueue();

	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT TestInputLayoutApp::WndMsgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
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

void TestInputLayoutApp::BuildStaticMeshes(ID3D12Device* pDevice, ID3D12GraphicsCommandList* cmdList)
{
	// Box
	{
		// Build Box
		CStaticMesh* pBoxMesh = new CStaticMesh();
		m_StaticMeshes.emplace("BoxMesh", pBoxMesh);
		std::vector<XMFLOAT3> positions;
		std::vector<UINT16> indices;
		std::vector<XMFLOAT4> vtxColors;

		Graphics::CreateBox(positions, indices);

		for (int i = 0; i < positions.size(); ++i)
		{
			float r = i * 1.0f / positions.size();
			vtxColors.push_back(XMFLOAT4(r, 0.5f, 0.5f, 1.0f));
		}

		pBoxMesh->m_pPositionBufferGPU = CreateDefaultBuffer(pDevice, cmdList, &positions[0], positions.size() * sizeof(XMFLOAT3), &pBoxMesh->m_pPositionBufferUpload);
		pBoxMesh->m_pVertexColorBufferGPU = CreateDefaultBuffer(pDevice, cmdList, &vtxColors[0], vtxColors.size() * sizeof(XMFLOAT4), &pBoxMesh->m_pVertexColorBufferUpload);
		pBoxMesh->m_pIndexBuferGPU = CreateDefaultBuffer(pDevice, cmdList, &indices[0], (UINT)indices.size() * sizeof(UINT16), &pBoxMesh->m_pIndexBufferUpload);

		pBoxMesh->m_PositionBufferView = Graphics::CreateVertexBufferView(pBoxMesh->m_pPositionBufferGPU, (UINT)positions.size() * sizeof(XMFLOAT3), sizeof(XMFLOAT3));
		pBoxMesh->m_VertexColorBufferView = Graphics::CreateVertexBufferView(pBoxMesh->m_pVertexColorBufferGPU, (UINT)vtxColors.size() * sizeof(XMFLOAT4), sizeof(XMFLOAT4));
		pBoxMesh->m_IndexBufferView = Graphics::CreateIndexBufferView(pBoxMesh->m_pIndexBuferGPU, (UINT)indices.size() * sizeof(UINT16), DXGI_FORMAT_R16_UINT);

		pBoxMesh->AddSubMesh("Box1", (UINT)indices.size(), 0, 0);
	}

	// Shpere
	{
		CStaticMesh* pSphereMesh = new CStaticMesh();
		m_StaticMeshes.emplace("SphereMesh", pSphereMesh);
		std::vector<XMFLOAT3> positions;
		std::vector<UINT16> indices;

		Graphics::CreateUVSphereMesh(32, 16, positions, indices);

		pSphereMesh->m_pPositionBufferGPU = CreateDefaultBuffer(pDevice, cmdList, &positions[0], positions.size() * sizeof(XMFLOAT3), &pSphereMesh->m_pPositionBufferUpload);
		pSphereMesh->m_pIndexBuferGPU = CreateDefaultBuffer(pDevice, cmdList, &indices[0], (UINT)indices.size() * sizeof(UINT16), &pSphereMesh->m_pIndexBufferUpload);

		pSphereMesh->m_PositionBufferView = Graphics::CreateVertexBufferView(pSphereMesh->m_pPositionBufferGPU, (UINT)positions.size() * sizeof(XMFLOAT3), sizeof(XMFLOAT3));
		pSphereMesh->m_IndexBufferView = Graphics::CreateIndexBufferView(pSphereMesh->m_pIndexBuferGPU, (UINT)indices.size() * sizeof(UINT16), DXGI_FORMAT_R16_UINT);

		pSphereMesh->AddSubMesh("Sub0", (UINT)indices.size(), 0, 0);
	}

	// Obj
	{
		CStaticMesh* pMesh = new CStaticMesh();
		m_StaticMeshes.emplace("Plane_Obj", pMesh);
		CImportor_Obj impoortor;
		impoortor.SetPath("D:\\Projects\\MyProjects\\LearnDirectX12\\D3D12Demo\\Content\\plane.obj"); // smooth_box plane  scene_simple
		impoortor.Import();

		MeshData* pMeshData = impoortor.m_MeshObjs[0];

		pMesh->m_pPositionBufferGPU = CreateDefaultBuffer(pDevice, cmdList, &pMeshData->Positions[0], pMeshData->Positions.size() * sizeof(XMFLOAT3), &pMesh->m_pPositionBufferUpload);
		pMesh->m_pIndexBuferGPU = CreateDefaultBuffer(pDevice, cmdList, &pMeshData->Indices[0], (UINT)pMeshData->Indices.size() * sizeof(UINT), &pMesh->m_pIndexBufferUpload);

		pMesh->m_PositionBufferView = Graphics::CreateVertexBufferView(pMesh->m_pPositionBufferGPU, (UINT)pMeshData->Positions.size() * sizeof(XMFLOAT3), sizeof(XMFLOAT3));
		pMesh->m_IndexBufferView = Graphics::CreateIndexBufferView(pMesh->m_pIndexBuferGPU, (UINT)pMeshData->Indices.size() * sizeof(UINT), DXGI_FORMAT_R32_UINT);

		pMesh->AddSubMesh("Sub0", (UINT)pMeshData->Indices.size(), 0, 0);

		impoortor.Clear();
	}
}

void TestInputLayoutApp::BuildScene()
{
	{
		// Box Matrix
		CStaticMesh* pBoxMesh = m_StaticMeshes["BoxMesh"];
		if (pBoxMesh)
		{
			for (int j = 0; j < 10; ++j)
			{
				CRenderObject* pObj = new CRenderObject();
				pObj->m_pStaticMesh = pBoxMesh;
				pObj->m_Transform.Position = XMFLOAT3(0.0f, (j - 5.0f) * 4.0f, 0.0f);
				pObj->m_mWorldMatrix = XMMatrixTranslation(0.0f, (j - 5.0f) * 4.0f, 0.0f);
				m_RenderObjects.push_back(pObj);
			}
		}
	}
	
	{
		CStaticMesh* pSphereMesh = m_StaticMeshes["SphereMesh"];
		if (pSphereMesh)
		{
			CRenderObject* pObj = new CRenderObject();
			pObj->m_pStaticMesh = pSphereMesh;
			pObj->m_Transform.Position = XMFLOAT3(5.0f, 0.0f, 0.0f);
			pObj->m_mWorldMatrix = XMMatrixTranslation(5.0f, 0.0f, 0.0f);
			m_RenderObjects.push_back(pObj);
		}
	}

	{
		CStaticMesh* pSphereMesh = m_StaticMeshes["Plane_Obj"];
		if (pSphereMesh)
		{
			CRenderObject* pObj = new CRenderObject();
			pObj->m_pStaticMesh = pSphereMesh;
			pObj->m_Transform.Position = XMFLOAT3(10.0f, 0.0f, 0.0f);
			pObj->m_mWorldMatrix = XMMatrixTranslation(10.0f, 0.0f, 0.0f);
			m_RenderObjects.push_back(pObj);
		}
	}
}

void TestInputLayoutApp::BuildPSOs(ID3D12Device* pDevice)
{
	ID3D12PipelineState* pOpaquePSO = nullptr;
	D3D12_GRAPHICS_PIPELINE_STATE_DESC OpaquePSODesc = {};
	OpaquePSODesc.InputLayout = { m_InputLayout.data(), (UINT)m_InputLayout.size() };
	OpaquePSODesc.pRootSignature = m_pRootSignature;
	OpaquePSODesc.VS = { m_pVSShaderCode->GetBufferPointer(), m_pVSShaderCode->GetBufferSize() };
	OpaquePSODesc.PS = { m_pPSShaderCode->GetBufferPointer(), m_pPSShaderCode->GetBufferSize() };
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
	pDevice->CreateGraphicsPipelineState(&OpaquePSODesc, IID_PPV_ARGS(&pOpaquePSO));
	m_PSOs.emplace("OpaquePSO", pOpaquePSO);
	
	{
		ID3D12PipelineState* pWireframePSO = nullptr;
		D3D12_GRAPHICS_PIPELINE_STATE_DESC wireframePSODesc = OpaquePSODesc;
		wireframePSODesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
		wireframePSODesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
		pDevice->CreateGraphicsPipelineState(&wireframePSODesc, IID_PPV_ARGS(&pWireframePSO));
		m_PSOs.emplace("WireframePSO", pWireframePSO);
	}

	{
		// SimplePosition
		ID3D12PipelineState* pSimplePositionPSO = nullptr;
		D3D12_GRAPHICS_PIPELINE_STATE_DESC SimplePositionPSDesc = OpaquePSODesc;
		SimplePositionPSDesc.InputLayout = { m_SimplePositionInputLayout.data(), (UINT)m_SimplePositionInputLayout.size() };
		SimplePositionPSDesc.VS = { m_pVSShaderCode_Position->GetBufferPointer(), m_pVSShaderCode_Position->GetBufferSize() };
		SimplePositionPSDesc.PS = { m_pPSShaderCode_Position->GetBufferPointer(), m_pPSShaderCode_Position->GetBufferSize() };
		SimplePositionPSDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
		SimplePositionPSDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
		pDevice->CreateGraphicsPipelineState(&SimplePositionPSDesc, IID_PPV_ARGS(&pSimplePositionPSO));
		m_PSOs.emplace("SimplePosition_PSO", pSimplePositionPSO);
	}
}

void TestInputLayoutApp::InitImgui()
{
	// Setup Dear ImGui binding
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls

	ImGui_ImplWin32_Init(m_hWnd);
	
	auto cpuHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_pCBVHeap->GetCPUDescriptorHandleForHeapStart());
	cpuHandle.Offset(m_imguiDescriptorIndex, m_nSRVDescriptorSize);
	
	auto gpuHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_pCBVHeap->GetGPUDescriptorHandleForHeapStart());
	gpuHandle.Offset(m_imguiDescriptorIndex, m_nSRVDescriptorSize);

	ImGui_ImplDX12_Init(m_pDevice, 1, DXGI_FORMAT_R8G8B8A8_UNORM, cpuHandle, gpuHandle);

	// Setup style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();
	//ImGui::StyleColorsLight();

	// Load Fonts
	// - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them. 
	// - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple. 
	// - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
	// - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
	// - Read 'misc/fonts/README.txt' for more instructions and details.
	// - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
	//io.Fonts->AddFontDefault();
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
	//ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
	//IM_ASSERT(font != NULL);
}
