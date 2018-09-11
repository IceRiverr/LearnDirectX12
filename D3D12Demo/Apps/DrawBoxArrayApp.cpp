#include "DrawBoxArrayApp.h"

DrawBoxArrayApp::DrawBoxArrayApp()
{
	m_pCBVHeap = nullptr;
	m_pRootSignature = nullptr;
	m_pVSShaderCode = nullptr;
	m_pPSShaderCode = nullptr;
}

DrawBoxArrayApp::~DrawBoxArrayApp()
{
	
}

void DrawBoxArrayApp::Init()
{
	WinApp::Init();
	m_pCommandList->Reset(m_pCommandAllocator, nullptr);
	// 所有初始化命令都放到该命令之后

	BuildStaticMeshes(m_pDevice, m_pCommandList);
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
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	BuildPSOs(m_pDevice);

	m_pCommandList->Close();

	// flush command
	ID3D12CommandList* cmdLists[1] = { m_pCommandList };
	m_pCommandQueue->ExecuteCommandLists(1, cmdLists);
	FlushCommandQueue();
}

void DrawBoxArrayApp::Update(double deltaTime)
{
	static double dTotalTime = 0.0f;
	dTotalTime += deltaTime;

	XMFLOAT3 f3EyePos;
	f3EyePos.x = 20.0f * (float)std::cos(dTotalTime * XM_PI * 0.4f);
	f3EyePos.y = 5.0f;
	f3EyePos.z = 20.0f * (float)std::sin(dTotalTime * XM_PI * 0.4f);

	XMVECTOR eyePos = XMVectorSet(f3EyePos.x, f3EyePos.y, f3EyePos.z, 1.0f);
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
	XMStoreFloat4x4(&m_FrameBuffer.m_FrameData.g_mView, XMMatrixTranspose(mView));
	XMStoreFloat4x4(&m_FrameBuffer.m_FrameData.g_mInvView, XMMatrixTranspose(mInvView));
	XMStoreFloat4x4(&m_FrameBuffer.m_FrameData.g_mProj, XMMatrixTranspose(mProj));
	XMStoreFloat4x4(&m_FrameBuffer.m_FrameData.g_mInvProj, XMMatrixTranspose(mInvProj));
	XMStoreFloat4x4(&m_FrameBuffer.m_FrameData.g_mViewProj, XMMatrixTranspose(mViewProj));
	XMStoreFloat4x4(&m_FrameBuffer.m_FrameData.g_mInvViewProj, XMMatrixTranspose(mInvViewProj));
	XMStoreFloat3(&m_FrameBuffer.m_FrameData.g_vEyePosition, eyePos);
	m_FrameBuffer.m_FrameData.PAD_0 = 0.0f;
	m_FrameBuffer.m_FrameData.g_InvRenderTargetSize = { (float)m_nClientWindowWidth, (float)m_nClientWindowHeight};
	m_FrameBuffer.m_FrameData.g_InvRenderTargetSize = { 1.0f / (float)m_nClientWindowWidth, 1.0f / (float)m_nClientWindowHeight };
	m_FrameBuffer.m_FrameData.g_fNearZ = 1.0f;
	m_FrameBuffer.m_FrameData.g_fFarZ = 1000.0f;
	m_FrameBuffer.m_FrameData.g_fTotalTime = (float)std::fmodf((float)dTotalTime, 1.0f);
	m_FrameBuffer.m_FrameData.g_fDeltaTime = (float)deltaTime;
	memcpy(m_FrameBuffer.m_pCbvDataBegin, &m_FrameBuffer.m_FrameData, m_FrameBuffer.m_nConstantBufferSizeAligned);
}

void DrawBoxArrayApp::Draw()
{
	m_pCommandAllocator->Reset();

	//m_pCommandList->Reset(m_pCommandAllocator, m_PSOs["OpaquePSO"]);
	m_pCommandList->Reset(m_pCommandAllocator, m_PSOs["WireframePSO"]);

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

		D3D12_VERTEX_BUFFER_VIEW VertexBufferViews[2] =
		{
			pObj->m_pStaticMesh->m_PositionBufferView,
			pObj->m_pStaticMesh->m_VertexColorBufferView
		};

		m_pCommandList->IASetVertexBuffers(0, 2, &VertexBufferViews[0]);
		m_pCommandList->IASetIndexBuffer(&pObj->m_pStaticMesh->m_IndexBufferView);
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

void DrawBoxArrayApp::BuildStaticMeshes(ID3D12Device* pDevice, ID3D12GraphicsCommandList* cmdList)
{
	// Box
	{
		
		// Build Box
		CStaticMesh* pBoxMesh = new CStaticMesh();
		m_StaticMeshes.emplace("BoxMesh", pBoxMesh);
		std::vector<XMFLOAT3> positions =
		{
			XMFLOAT3(-1.0f, -1.0f, -1.0f),
			XMFLOAT3(-1.0f, +1.0f, -1.0f),
			XMFLOAT3(+1.0f, +1.0f, -1.0f),
			XMFLOAT3(+1.0f, -1.0f, -1.0f),
			XMFLOAT3(-1.0f, -1.0f, +1.0f),
			XMFLOAT3(-1.0f, +1.0f, +1.0f),
			XMFLOAT3(+1.0f, +1.0f, +1.0f),
			XMFLOAT3(+1.0f, -1.0f, +1.0f)
		};

		std::vector<XMFLOAT4> vtxColors =
		{
			XMFLOAT4(1.0f, 0.5f, 0.5f, 1.0f),
			XMFLOAT4(1.0f, 0.5f, 0.5f, 1.0f),
			XMFLOAT4(1.0f, 0.5f, 0.5f, 1.0f),
			XMFLOAT4(1.0f, 0.5f, 0.5f, 1.0f),
			XMFLOAT4(1.0f, 0.5f, 0.5f, 1.0f),
			XMFLOAT4(1.0f, 0.5f, 0.5f, 1.0f),
			XMFLOAT4(1.0f, 0.5f, 0.5f, 1.0f),
			XMFLOAT4(1.0f, 0.5f, 0.5f, 1.0f)
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
		pBoxMesh->m_pPositionBufferGPU = CreateDefaultBuffer(pDevice, cmdList, &positions[0], positions.size() * sizeof(XMFLOAT3), &pBoxMesh->m_pPositionBufferUpload);
		pBoxMesh->m_pVertexColorBufferGPU = CreateDefaultBuffer(pDevice, cmdList, &vtxColors[0], vtxColors.size() * sizeof(XMFLOAT4), &pBoxMesh->m_pVertexColorBufferUpload);
		pBoxMesh->m_pIndexBuferGPU = CreateDefaultBuffer(pDevice, cmdList, &indices[0], (UINT)indices.size() * sizeof(UINT16), &pBoxMesh->m_pIndexBufferUpload);

		pBoxMesh->m_PositionBufferView = Graphics::CreateVertexBufferView(pBoxMesh->m_pPositionBufferGPU, (UINT)positions.size() * sizeof(XMFLOAT3), sizeof(XMFLOAT3));
		pBoxMesh->m_VertexColorBufferView = Graphics::CreateVertexBufferView(pBoxMesh->m_pVertexColorBufferGPU, (UINT)vtxColors.size() * sizeof(XMFLOAT4), sizeof(XMFLOAT4));
		pBoxMesh->m_IndexBufferView = Graphics::CreateIndexBufferView(pBoxMesh->m_pIndexBuferGPU, (UINT)indices.size() * sizeof(UINT16), DXGI_FORMAT_R16_UINT);

		pBoxMesh->AddSubMesh("Sub0", (UINT)indices.size(), 0, 0);

	}
	
	// sphere
	{
		CStaticMesh* pSphereMesh = new CStaticMesh();
		m_StaticMeshes.emplace("SphereMesh", pSphereMesh);
		std::vector<XMFLOAT3> positions;
		std::vector<XMFLOAT4> vtxColors;
		std::vector<UINT16> indices;

		Graphics::CreateUVSphereMesh(32, 16, positions, indices);

		for (int i = 0; i < positions.size(); ++i)
		{
			vtxColors.push_back(XMFLOAT4(0.0f, 0.0f, 0.8f, 1.0f));
		}

		pSphereMesh->m_pPositionBufferGPU = CreateDefaultBuffer(pDevice, cmdList, &positions[0], positions.size() * sizeof(XMFLOAT3), &pSphereMesh->m_pPositionBufferUpload);
		pSphereMesh->m_pVertexColorBufferGPU = CreateDefaultBuffer(pDevice, cmdList, &vtxColors[0], vtxColors.size() * sizeof(XMFLOAT4), &pSphereMesh->m_pVertexColorBufferUpload);
		pSphereMesh->m_pIndexBuferGPU = CreateDefaultBuffer(pDevice, cmdList, &indices[0], (UINT)indices.size() * sizeof(UINT16), &pSphereMesh->m_pIndexBufferUpload);

		pSphereMesh->m_PositionBufferView = Graphics::CreateVertexBufferView(pSphereMesh->m_pPositionBufferGPU, (UINT)positions.size() * sizeof(XMFLOAT3), sizeof(XMFLOAT3));
		pSphereMesh->m_VertexColorBufferView = Graphics::CreateVertexBufferView(pSphereMesh->m_pVertexColorBufferGPU, (UINT)vtxColors.size() * sizeof(XMFLOAT4), sizeof(XMFLOAT4));
		pSphereMesh->m_IndexBufferView = Graphics::CreateIndexBufferView(pSphereMesh->m_pIndexBuferGPU, (UINT)indices.size() * sizeof(UINT16), DXGI_FORMAT_R16_UINT);

		pSphereMesh->AddSubMesh("Sub0", (UINT)indices.size(), 0, 0);
	}
}

void DrawBoxArrayApp::BuildScene()
{
	// Box Matrix
	CStaticMesh* pBoxMesh = m_StaticMeshes["BoxMesh"];
	CStaticMesh* pSphereMesh = m_StaticMeshes["SphereMesh"];
	if (pBoxMesh)
	{
		for (int j = 0; j < 10; ++j)
		{
			for (int i = 0; i < 10; ++i)
			{
				CRenderObject* pObj = new CRenderObject();
				if (i == j)
				{
					pObj->m_pStaticMesh = pSphereMesh;
				}
				else
				{
					pObj->m_pStaticMesh = pBoxMesh;
				}
				pObj->m_WorldTransform.Position = XMFLOAT3((i - 5.0f) * 4.0f, (j - 5.0f) * 4.0f, 0.0f);
				pObj->m_mWorldMatrix = XMMatrixTranslation((i - 5.0f) * 4.0f, (j - 5.0f) * 4.0f, 0.0f);
				m_RenderObjects.push_back(pObj);
			}
		}
	}
}

void DrawBoxArrayApp::BuildPSOs(ID3D12Device* pDevice)
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

	// 
	ID3D12PipelineState* pWireframePSO = nullptr;
	D3D12_GRAPHICS_PIPELINE_STATE_DESC wireframePSODesc = OpaquePSODesc;
	wireframePSODesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	wireframePSODesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	pDevice->CreateGraphicsPipelineState(&wireframePSODesc, IID_PPV_ARGS(&pWireframePSO));
	m_PSOs.emplace("WireframePSO", pWireframePSO);
}
