#include "DrawBoxArrayApp.h"

DrawBoxArrayApp::DrawBoxArrayApp()
{
	m_pCBVHeap = nullptr;
	m_pRootSignature = nullptr;
	m_pVSShaderCode = nullptr;
	m_pPSShaderCode = nullptr;
	m_pPSO = nullptr;
}

DrawBoxArrayApp::~DrawBoxArrayApp()
{
	
}

void DrawBoxArrayApp::Init()
{
	WinApp::Init();
	m_pCommandList->Reset(m_pCommandAllocator, nullptr);
	// 所有初始化命令都放到该命令之后

	BuildStaticMeshes();
	BuildScene();

	// create cbv heap
	D3D12_DESCRIPTOR_HEAP_DESC cbHeapDesc = {};
	cbHeapDesc.NumDescriptors = (UINT)m_RenderObjects.size() + 1;
	cbHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cbHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	cbHeapDesc.NodeMask = 0;
	m_pDevice->CreateDescriptorHeap(&cbHeapDesc, IID_PPV_ARGS(&m_pCBVHeap));

	UINT nDescriptorIndex = 0;
	m_ConstBuffer.CreateBuffer(m_pDevice, (UINT)m_RenderObjects.size());
	for (int i = 0; i < m_RenderObjects.size(); ++i)
	{
		m_RenderObjects[i]->m_nConstantBufferIndex = nDescriptorIndex;
		auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_pCBVHeap->GetCPUDescriptorHandleForHeapStart());
		handle.Offset(m_RenderObjects[i]->m_nConstantBufferIndex, m_nSRVDescriptorSize);
		m_ConstBuffer.CreateBufferView(m_pDevice, handle, m_RenderObjects[i]->m_nConstantBufferIndex);
		nDescriptorIndex++;
	}

	m_FrameBuffer.CreateBuffer(m_pDevice);
	m_FrameBuffer.m_nDescriptorIndex = nDescriptorIndex;
	auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_pCBVHeap->GetCPUDescriptorHandleForHeapStart());
	handle.Offset(m_FrameBuffer.m_nDescriptorIndex, m_nSRVDescriptorSize);
	m_FrameBuffer.CreateBufferView(m_pDevice, handle);

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

	// Compile Shader
	UINT compileFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)
	compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	ID3DBlob* pErrorMsg = nullptr;
	D3DCompileFromFile(L"D:\\Projects\\MyProjects\\LearnDirectX12\\D3D12Demo\\Shaders\\color_view_info.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &m_pVSShaderCode, &pErrorMsg);
	if (pErrorMsg)
	{
		std::cout << "ShaderCompileError: " << std::string((char*)pErrorMsg->GetBufferPointer()) << std::endl;
		return;
	}
	
	D3DCompileFromFile(L"D:\\Projects\\MyProjects\\LearnDirectX12\\D3D12Demo\\Shaders\\color_view_info.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &m_pPSShaderCode, &pErrorMsg);
	if (pErrorMsg)
	{
		std::cout << "ShaderCompileError: " << std::string((char*)pErrorMsg->GetBufferPointer()) << std::endl;
		return;
	}

	m_InputLayout = 
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	// PSO
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = { m_InputLayout.data(), (UINT)m_InputLayout.size() };
	psoDesc.pRootSignature = m_pRootSignature; 
	psoDesc.VS = { m_pVSShaderCode->GetBufferPointer(), m_pVSShaderCode->GetBufferSize() };
	psoDesc.PS = { m_pPSShaderCode->GetBufferPointer(), m_pPSShaderCode->GetBufferSize() };
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = m_BackBufferFromat;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleDesc.Quality = 0;
	psoDesc.DSVFormat = m_DSVFormat;

	m_pDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pPSO));

	m_pCommandList->Close();

	// flush command
	ID3D12CommandList* cmdLists[1] = { m_pCommandList };
	m_pCommandQueue->ExecuteCommandLists(1, cmdLists);
	FlushCommandQueue();
}

void DrawBoxArrayApp::Update(double deltaTime)
{
	static double fTotalTime = 0.0f;
	fTotalTime += deltaTime;

	XMVECTOR eyePos = XMVectorSet(0.0, 0.0f, -20.0f, 1.0f);
	XMVECTOR target = XMVectorZero();
	XMVECTOR upDir = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	XMMATRIX mView = XMMatrixLookAtLH(eyePos, target, upDir);
	XMMATRIX mProj = XMMatrixPerspectiveFovLH(90.0f / 180.0f * 3.14159f, m_nClientWindowWidth * 1.0f / m_nClientWindowHeight, 1.0f, 1000.0f);
	XMMATRIX mViewProj = XMMatrixMultiply(mView, mProj);

	XMMATRIX mInvView = XMMatrixInverse(&XMMatrixDeterminant(mView), mView);
	XMMATRIX mInvProj = XMMatrixInverse(&XMMatrixDeterminant(mProj), mProj);
	XMMATRIX mInvViewProj = XMMatrixInverse(&XMMatrixDeterminant(mViewProj), mViewProj);

	// 渲染10x10 100个方格
	for (int i = 0; i < m_RenderObjects.size(); ++i)
	{
		XMMATRIX mWorldMat = m_RenderObjects[i]->m_mWorldMatrix;
		//XMMATRIX mRotateMat = XMMatrixRotationY((float)fTotalTime * (i + j + 1) * 0.2f);
		//mWorldMat = mRotateMat * mWorldMat;

		CRenderObject::ConstantElement objConstant;
		XMStoreFloat4x4(&objConstant.mWorldMat, XMMatrixTranspose(mWorldMat));
		m_ConstBuffer.UpdateBuffer((UINT8*)&objConstant, sizeof(objConstant), m_RenderObjects[i]->m_nConstantBufferIndex);
	}

	// Frame Buffer
	XMStoreFloat4x4(&m_FrameBuffer.m_FrameBufferData.g_mView, XMMatrixTranspose(mView));
	XMStoreFloat4x4(&m_FrameBuffer.m_FrameBufferData.g_mInvView, XMMatrixTranspose(mInvView));
	XMStoreFloat4x4(&m_FrameBuffer.m_FrameBufferData.g_mProj, XMMatrixTranspose(mProj));
	XMStoreFloat4x4(&m_FrameBuffer.m_FrameBufferData.g_mInvProj, XMMatrixTranspose(mInvProj));
	XMStoreFloat4x4(&m_FrameBuffer.m_FrameBufferData.g_mViewProj, XMMatrixTranspose(mViewProj));
	XMStoreFloat4x4(&m_FrameBuffer.m_FrameBufferData.g_mInvViewProj, XMMatrixTranspose(mInvViewProj));
	XMStoreFloat3(&m_FrameBuffer.m_FrameBufferData.g_vEyePosition, eyePos);
	m_FrameBuffer.m_FrameBufferData.PAD_1 = 0.0f;
	m_FrameBuffer.m_FrameBufferData.g_InvRenderTargetSize = { (float)m_nClientWindowWidth, (float)m_nClientWindowHeight};
	m_FrameBuffer.m_FrameBufferData.g_InvRenderTargetSize = { 1.0f / (float)m_nClientWindowWidth, 1.0f / (float)m_nClientWindowHeight };
	m_FrameBuffer.m_FrameBufferData.g_fNearZ = 1.0f;
	m_FrameBuffer.m_FrameBufferData.g_fFarZ = 1000.0f;
	m_FrameBuffer.m_FrameBufferData.g_fTotalTime = (float)std::fmodf((float)fTotalTime, 1.0f);
	m_FrameBuffer.m_FrameBufferData.g_fDeltaTime = (float)deltaTime;
	memcpy(m_FrameBuffer.m_pCbvDataBegin, &m_FrameBuffer.m_FrameBufferData, m_FrameBuffer.m_nConstantBufferSizeAligned);
}

void DrawBoxArrayApp::Draw()
{
	m_pCommandAllocator->Reset();
	m_pCommandList->Reset(m_pCommandAllocator, m_pPSO);

	m_pCommandList->RSSetViewports(1, &m_ScreenViewPort);
	m_pCommandList->RSSetScissorRects(1, &m_ScissorRect);

	m_pCommandList->ResourceBarrier(
		1, &CD3DX12_RESOURCE_BARRIER::Transition(
			GetCurrentBackBuffer(),
			D3D12_RESOURCE_STATE_PRESENT,
			D3D12_RESOURCE_STATE_RENDER_TARGET));

	// Begin Draw

	float clearColors[4] = { 80.0f/255.0f, 90.0f / 255.0f, 100.0f/255.0f, 1.0f };
	m_pCommandList->ClearRenderTargetView(
		GetCurrentBackBufferView(),
		clearColors, 0, nullptr);

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
		m_pCommandList->IASetVertexBuffers(0, 1, &(pObj->m_pStaticMesh->GetVertexBufferView()));
		m_pCommandList->IASetIndexBuffer(&(pObj->m_pStaticMesh->GetIndexBufferView()));
		m_pCommandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		
		auto handle1 = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_pCBVHeap->GetGPUDescriptorHandleForHeapStart());
		handle1.Offset(pObj->m_nConstantBufferIndex, m_nSRVDescriptorSize);
		m_pCommandList->SetGraphicsRootDescriptorTable(0, handle1);

		for (auto it : pObj->m_pStaticMesh->m_SubMeshes)
		{
			SubMesh subMesh = it.second;
			
			m_pCommandList->DrawIndexedInstanced(subMesh.nIndexCount, 1, subMesh.nStartIndexLocation, subMesh.nBaseVertexLocation, 0);
		}
	}

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

void DrawBoxArrayApp::OnResize()
{
	WinApp::OnResize();
	
	XMMATRIX p = XMMatrixPerspectiveFovLH(90.0f /180.0f * 3.14159f, m_nClientWindowWidth * 1.0f / m_nClientWindowHeight, 1.0f, 1000.0f);

	XMStoreFloat4x4(&m_ProjMat, p);
}

void DrawBoxArrayApp::BuildStaticMeshes()
{
	// Box
	// Build Box
	CStaticMesh* pBoxMesh = new CStaticMesh();
	m_StaticMeshes.emplace("BoxMesh", pBoxMesh);
	std::vector<Vertex> vertices =
	{
		Vertex({ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(1.0f, 0.5f, 0.5f, 1.0f) }),
		Vertex({ XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT4(1.0f, 0.5f, 0.5f, 1.0f) }),
		Vertex({ XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT4(1.0f, 0.5f, 0.5f, 1.0f) }),
		Vertex({ XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT4(1.0f, 0.5f, 0.5f, 1.0f) }),
		Vertex({ XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT4(1.0f, 0.5f, 0.5f, 1.0f) }),
		Vertex({ XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT4(1.0f, 0.5f, 0.5f, 1.0f) }),
		Vertex({ XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT4(1.0f, 0.5f, 0.5f, 1.0f) }),
		Vertex({ XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT4(1.0f, 0.5f, 0.5f, 1.0f) })
	};

	std::vector<UINT16> indices =
	{
		// front face
		0, 1, 2,
		0, 2, 3,

		// back face
		4, 6, 5,
		4, 7, 6,

		// left face
		4, 5, 1,
		4, 1, 0,

		// right face
		3, 2, 6,
		3, 6, 7,

		// top face
		1, 5, 6,
		1, 6, 2,

		// bottom face
		4, 0, 3,
		4, 3, 7
	};

	pBoxMesh->m_nVertexSizeInBytes = (UINT)vertices.size() * sizeof(Vertex);
	pBoxMesh->m_nVertexStrideInBytes = sizeof(Vertex);
	pBoxMesh->m_nIndexSizeInBytes = (UINT)indices.size() * sizeof(UINT16);
	pBoxMesh->m_IndexFormat = DXGI_FORMAT_R16_UINT;

	pBoxMesh->m_pVertexBufferGPU = CreateDefaultBuffer(m_pDevice, m_pCommandList, &vertices[0], pBoxMesh->m_nVertexSizeInBytes, &pBoxMesh->m_pVertexBufferUpload);
	pBoxMesh->m_pIndexBuferGPU = CreateDefaultBuffer(m_pDevice, m_pCommandList, &indices[0], pBoxMesh->m_nIndexSizeInBytes, &pBoxMesh->m_pIndexBufferUpload);
	pBoxMesh->AddSubMesh("Box1", (UINT)indices.size(), 0, 0);
}

void DrawBoxArrayApp::BuildScene()
{
	// Box Matrix
	CStaticMesh* pBoxMesh = m_StaticMeshes["BoxMesh"];
	if (pBoxMesh)
	{
		for (int j = 0; j < 10; ++j)
		{
			for (int i = 0; i < 10; ++i)
			{
				CRenderObject* pObj = new CRenderObject();
				pObj->m_pStaticMesh = pBoxMesh;
				pObj->m_WorldTransform.Position = XMFLOAT3((i - 5.0f) * 4.0f, (j - 5.0f) * 4.0f, 0.0f);
				pObj->m_mWorldMatrix = XMMatrixTranslation((i - 5.0f) * 4.0f, (j - 5.0f) * 4.0f, 0.0f);
				m_RenderObjects.push_back(pObj);
			}
		}
	}
}
