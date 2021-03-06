#include "Material_BRDF.h"
#include "ImportObj.h"

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"

#include "DirectXTex.h"
#include "MeshFactory.h"

CMaterialBRDFApp::CMaterialBRDFApp()
{
	m_ContentRootPath = "D:\\Projects\\MyProjects\\LearnDirectX12\\D3D12Demo\\Content\\";
	m_ShaderRootPath = "D:\\Projects\\MyProjects\\LearnDirectX12\\D3D12Demo\\Shaders\\";

	m_pRootSignature = nullptr;
	m_pVSShaderCode_Light = nullptr;
	m_pPSShaderCode_Light = nullptr;

	clear_color = { 135.0f / 255.0f, 206.0f / 255.0f, 250.0f / 255.0f, 1.0f };
	m_pBRDFMat = nullptr;

	m_bGuiMode = false;
}

CMaterialBRDFApp::~CMaterialBRDFApp()
{
	// 内存析构都没有做，需要完善


}

void CMaterialBRDFApp::Init()
{
	WinApp::Init();

	m_pCommandList->Reset(m_pCommandAllocator, nullptr);
	// 所有初始化命令都放到该命令之后

	BuildRootSignature();
	BuildPSOs(m_pDevice);
	
	// 这个地方不合适
	// 这个地方不合适
	m_pGraphicContext = new CGraphicContext();
	m_pGraphicContext->Init();
	m_pGraphicContext->m_pDevice = m_pDevice;
	m_pGraphicContext->m_pCommandList = m_pCommandList;
	m_pGraphicContext->m_pRootSignature = m_pRootSignature;
	m_pGraphicContext->m_BackBufferFromat = m_BackBufferFromat;
	m_pGraphicContext->m_DSVFormat = m_DSVFormat;
	
	BuildMaterials();
	BuildStaticMeshes(m_pDevice, m_pCommandList);
	BuildScene();
	BuildHeapDescriptors();

	InitImgui();

	m_pCommandList->Close();

	// flush command
	ID3D12CommandList* cmdLists[1] = { m_pCommandList };
	m_pCommandQueue->ExecuteCommandLists(1, cmdLists);
	FlushCommandQueue();

	TEST_AREA();

	m_pCamera = new CFPSCamera();
	m_pCamera->Init(90.0f, m_nClientWindowWidth * 1.0f / m_nClientWindowHeight, 0.01f, 2000.0f);
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
		m_RenderObjects[i]->Update();
		
		ObjectShaderBlock objConstant = m_RenderObjects[i]->CreateShaderBlock();
		m_ObjectBuffer.UpdateBuffer((UINT8*)&objConstant, sizeof(objConstant), m_RenderObjects[i]->m_ObjectAddress.nBufferIndex);
	}

	for (auto it = m_Materials.begin(); it != m_Materials.end(); ++it)
	{
		CMaterial* pMat = it->second;
		MaterialShaderBlock shaderBlock = pMat->CreateShaderBlock();

		m_MaterialBuffer.UpdateBuffer((UINT8*)&shaderBlock, sizeof(shaderBlock), pMat->m_MaterialAddress.nBufferIndex);
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
			m_FrameBuffer.m_FrameData.g_Lights[i] = m_DirLights[i]->CreateShaderBlock();
		}

		int nPointLightStart = (int)m_DirLights.size();
		for (int i = 0; i < m_PointLights.size(); ++i)
		{
			m_FrameBuffer.m_FrameData.g_Lights[nPointLightStart + i] = m_PointLights[i]->CreateShaderBlock();
		}

		int nSpotLightStart = (int)(nPointLightStart + m_PointLights.size());
		for (int i = 0; i < m_SpotLights.size(); ++i)
		{
			m_FrameBuffer.m_FrameData.g_Lights[nSpotLightStart + i] = m_SpotLights[i]->CreateShaderBlock();
		}

		m_FrameBuffer.m_FrameData.g_LightNumbers[0] = (int)m_DirLights.size();
		m_FrameBuffer.m_FrameData.g_LightNumbers[1] = (int)m_PointLights.size();
		m_FrameBuffer.m_FrameData.g_LightNumbers[2] = (int)m_SpotLights.size();
		m_FrameBuffer.m_FrameData.g_LightNumbers[3] = 0;
	}

	memcpy(m_FrameBuffer.m_pCbvDataBegin, &m_FrameBuffer.m_FrameData, m_FrameBuffer.m_nConstantBufferSizeAligned);
}

void CMaterialBRDFApp::DrawImgui()
{
	ImGui::Render();
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_pCommandList);
}


void CMaterialBRDFApp::TEST_AREA()
{
	CD3DX12_RESOURCE_DESC descs[] =
	{
		CD3DX12_RESOURCE_DESC::Buffer(1024 * sizeof(float)),
		CD3DX12_RESOURCE_DESC::Buffer(1),
		CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, 1, 1),
	};

	D3D12_RESOURCE_ALLOCATION_INFO allocationInfos = m_pDevice->GetResourceAllocationInfo(0, 3, descs);

	int a = 1;
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

	auto DescriptorHeaps = m_pGraphicContext->m_pSRVAllocator->GetHeaps();
	m_pCommandList->SetDescriptorHeaps((UINT)DescriptorHeaps.size() , DescriptorHeaps.data());
	m_pCommandList->SetGraphicsRootSignature(m_pRootSignature);

	m_pCommandList->SetGraphicsRootConstantBufferView(0, m_FrameBuffer.m_pUploadeConstBuffer->GetGPUVirtualAddress());

	m_pCommandList->SetGraphicsRootDescriptorTable(5, m_pSkySphere->m_pEnvironmentMap->GPUHandle);
	m_pCommandList->SetGraphicsRootDescriptorTable(6, m_pSkySphere->m_pReflectionMap->GPUHandle);
	m_pCommandList->SetGraphicsRootDescriptorTable(7, m_pSkySphere->m_pBackGroundMap->GPUHandle);

	for (int i = 0; i < m_RenderObjects.size() - 1; ++i)
	{
		CRenderObject* pObj = m_RenderObjects[i];
		pObj->Draw(m_pCommandList);
	}

	m_pSkySphere->Draw(m_pCommandList);

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

void CMaterialBRDFApp::BuildRootSignature()
{
	// create root signature
	CD3DX12_DESCRIPTOR_RANGE cbvTable1;
	cbvTable1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1);

	CD3DX12_DESCRIPTOR_RANGE cbvTable2;
	cbvTable2.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 2);

	CD3DX12_DESCRIPTOR_RANGE textureTable;
	textureTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 0);

	CD3DX12_DESCRIPTOR_RANGE range1;
	range1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 10);

	CD3DX12_DESCRIPTOR_RANGE HDREnvTable;
	HDREnvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 4);

	CD3DX12_DESCRIPTOR_RANGE HDRRefTable;
	HDRRefTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 5);

	CD3DX12_DESCRIPTOR_RANGE HDRBGRefTable;
	HDRBGRefTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 6);

	CD3DX12_ROOT_PARAMETER slotRootParameter[8];
	slotRootParameter[0].InitAsConstantBufferView(0);
	slotRootParameter[1].InitAsDescriptorTable(1, &cbvTable1);
	slotRootParameter[2].InitAsDescriptorTable(1, &cbvTable2);
	slotRootParameter[3].InitAsDescriptorTable(1, &textureTable, D3D12_SHADER_VISIBILITY_PIXEL); // 如果一个Material中有些有texutre，有些没有纹理，应该怎么处理
	slotRootParameter[4].InitAsDescriptorTable(1, &range1, D3D12_SHADER_VISIBILITY_PIXEL);
	slotRootParameter[5].InitAsDescriptorTable(1, &HDREnvTable, D3D12_SHADER_VISIBILITY_PIXEL); 
	slotRootParameter[6].InitAsDescriptorTable(1, &HDRRefTable, D3D12_SHADER_VISIBILITY_PIXEL); 
	slotRootParameter[7].InitAsDescriptorTable(1, &HDRBGRefTable, D3D12_SHADER_VISIBILITY_PIXEL);

	StaticSamplerStates StaticSamplers;
	StaticSamplers.CreateStaticSamplers();

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(8, slotRootParameter, STATIC_SAMPLER_TYPE::SAMPLER_COUNT, StaticSamplers.Samplers.data(), D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

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
}

void CMaterialBRDFApp::BuildPSOs(ID3D12Device* pDevice)
{
	m_pVSShaderCode_Light = m_pGraphicContext->CompileShader(m_ShaderRootPath + "light_material.fx", "VSMain", "vs_5_0");
	m_pPSShaderCode_Light = m_pGraphicContext->CompileShader(m_ShaderRootPath + "light_material.fx", "PSMain", "ps_5_0");
	m_pVSShaderCode_Material = m_pGraphicContext->CompileShader(m_ShaderRootPath + "Mat_DefaultShader.fx", "VSMain", "vs_5_0");
	m_pPSShaderCode_Material = m_pGraphicContext->CompileShader(m_ShaderRootPath + "Mat_DefaultShader.fx", "PSMain", "ps_5_0");
	
	{
		ID3D12PipelineState* pOpaquePSO = nullptr;
		D3D12_GRAPHICS_PIPELINE_STATE_DESC OpaquePSODesc = {};
		auto InputLayout = GetInputLayout(INPUT_LAYOUT_TYPE::P3N3);
		OpaquePSODesc.InputLayout = { InputLayout.data(), (UINT)InputLayout.size() };
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

	{
		ID3D12PipelineState* pOpaquePSO = nullptr;
		D3D12_GRAPHICS_PIPELINE_STATE_DESC OpaquePSODesc = {};
		auto InputLayout = GetInputLayout(INPUT_LAYOUT_TYPE::P3N3T4UV2);
		OpaquePSODesc.InputLayout = { InputLayout.data(), (UINT)InputLayout.size() };
		OpaquePSODesc.pRootSignature = m_pRootSignature;
		OpaquePSODesc.VS = { m_pVSShaderCode_Material->GetBufferPointer(), m_pVSShaderCode_Material->GetBufferSize() };
		OpaquePSODesc.PS = { m_pPSShaderCode_Material->GetBufferPointer(), m_pPSShaderCode_Material->GetBufferSize() };
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
		m_PSOs.emplace("Material_PSO", pOpaquePSO);
	}
}

void CMaterialBRDFApp::BuildMaterials()
{
	{
		m_pBRDFMat = new CMaterial();
		m_pBRDFMat->m_sName = "BRDF_Color";
		m_pBRDFMat->m_pPSO = m_PSOs["Light_PSO"];
		m_pBRDFMat->m_InputLayoutType = INPUT_LAYOUT_TYPE::P3N3;
		m_pBRDFMat->m_cBaseColor = XMFLOAT4(244.0f/255.0f, 159.0f/255.0f, 8.0f / 255.0f, 1.0f);
		m_pBRDFMat->m_fSmoothness = 0.2f;
		m_pBRDFMat->m_fMetalMask = 0.7f;
		m_pBRDFMat->m_fReflectance = 0.8f;
		m_Materials.emplace(m_pBRDFMat->m_sName, m_pBRDFMat);
	}

	{
		CMaterial* pMat = new CMaterial();
		pMat->m_sName = "BRDF_Texture";
		pMat->m_pPSO = m_PSOs["Material_PSO"];
		pMat->m_InputLayoutType = INPUT_LAYOUT_TYPE::P3N3T4UV2;
		pMat->m_cBaseColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		pMat->m_fSmoothness = 0.2f;
		pMat->m_fMetalMask = 0.0f;
		pMat->m_fReflectance = 0.8f;
		pMat->m_sAldeboPath = m_ContentRootPath + "Gun\\Textures\\Cerberus_A.tga";
		pMat->m_sNormalPath = m_ContentRootPath + "Gun\\Textures\\Cerberus_N.tga";
		pMat->m_sRoughnessPath = m_ContentRootPath + "Gun\\Textures\\Cerberus_R.tga";
		pMat->m_sMetalicPath = m_ContentRootPath + "Gun\\Textures\\Cerberus_M.tga";
		pMat->Init(m_pGraphicContext);

		m_Materials.emplace(pMat->m_sName, pMat);
	}

	{
		CMaterial* pMat = new CMaterial();
		pMat->m_sName = "Metal_Mat";
		pMat->m_pPSO = m_PSOs["Material_PSO"];
		pMat->m_InputLayoutType = INPUT_LAYOUT_TYPE::P3N3T4UV2;
		pMat->m_cBaseColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		pMat->m_fSmoothness = 0.2f;
		pMat->m_fMetalMask = 0.0f;
		pMat->m_fReflectance = 0.8f;
		pMat->m_sAldeboPath =    m_ContentRootPath + "MetalSpottyDiscoloration001\\SPECULAR\\4K\\MetalGoldBrushed001_REFL_3K_SPECULAR.jpg";
		pMat->m_sNormalPath =    m_ContentRootPath + "MetalSpottyDiscoloration001\\SPECULAR\\4K\\MetalGoldBrushed001_NRM_3K_SPECULAR.jpg";
		pMat->m_sRoughnessPath = m_ContentRootPath + "MetalSpottyDiscoloration001\\SPECULAR\\4K\\MetalGoldBrushed001_GLOSS_3K_SPECULAR.jpg";
		pMat->m_sMetalicPath =   m_ContentRootPath + "MetalSpottyDiscoloration001\\SPECULAR\\4K\\MetalGoldBrushed001_GLOSS_3K_SPECULAR.jpg";
		pMat->Init(m_pGraphicContext);

		m_Materials.emplace(pMat->m_sName, pMat);
	}
}

void CMaterialBRDFApp::BuildStaticMeshes(ID3D12Device* pDevice, ID3D12GraphicsCommandList* cmdList)
{
	{
		CImportor_Obj impoortor;
		impoortor.SetPath(m_ContentRootPath + "plane.obj"); // smooth_box plane  scene_simple
		impoortor.Import();
		MeshData* pMeshData = impoortor.m_MeshObjs[0];

		CStaticMesh* pMesh = new CStaticMesh();
		pMesh->CreateBuffer(pMeshData, *m_pGraphicContext);
		m_StaticMeshes.emplace("Plane_Obj", pMesh);
		pMesh->m_pMaterial = m_Materials["BRDF_Color"];
	}
	
	{
		std::string gun_model_path = m_ContentRootPath + "Gun\\gun.obj";
		//std::string smooth_box_path = m_ContentRootPath + "smooth_box.obj";

		CImportor_Obj impoortor;
		impoortor.SetPath(gun_model_path);
		impoortor.Import();
		MeshData* pMeshData = impoortor.m_MeshObjs[0];

		CStaticMesh* pMesh = new CStaticMesh();
		pMesh->CreateBuffer(pMeshData, *m_pGraphicContext);
		m_StaticMeshes.emplace("Smooth_box", pMesh);
		pMesh->m_pMaterial = m_Materials["BRDF_Texture"];
	}

	{
		CImportor_Obj impoortor;
		impoortor.SetPath(m_ContentRootPath + "UVSphere.obj");
		impoortor.Import();
		MeshData* pMeshData = impoortor.m_MeshObjs[0];

		CStaticMesh* pMesh = new CStaticMesh();
		pMesh->CreateBuffer(pMeshData, *m_pGraphicContext);
		m_StaticMeshes.emplace("UVSphere", pMesh);
		pMesh->m_pMaterial = m_Materials["BRDF_Color"];
	}

	{
		
		MeshData meshData;
		std::vector<XMFLOAT3> positions;
		std::vector<UINT16> indices;
		CMeshFactory::CreateUVSphereMesh(64, 32, positions, indices);

		for (int i = 0; i < positions.size(); ++i)
		{
			meshData.Positions.push_back(positions[i]);
		}
		for (int i = 0; i < indices.size(); ++i)
		{
			meshData.Indices.push_back(indices[i]);
		}

		CStaticMesh* pSphereMesh = new CStaticMesh();
		pSphereMesh->CreateBuffer(&meshData, *m_pGraphicContext);
		m_StaticMeshes.emplace("SphereMesh", pSphereMesh);
		
		pSphereMesh->AddSubMesh("Sub0", (UINT)indices.size(), 0, 0);
	}
}

void CMaterialBRDFApp::BuildScene()
{
	
	{
		CStaticMesh* pSphereMesh = nullptr;
		pSphereMesh = m_StaticMeshes["Plane_Obj"];
		if (pSphereMesh)
		{
			CRenderObject* pObj = new CRenderObject();
			pObj->m_pStaticMesh = pSphereMesh;
			pObj->m_Transform.Position = XMFLOAT3(0.0f, 0.0f, 0.0f);
			pObj->m_Transform.Scale = XMFLOAT3(10.0f, 10.0f, 10.0f);

			m_RenderObjects.push_back(pObj);
		}
	}

	{
		CStaticMesh* pSphereMesh = m_StaticMeshes["Smooth_box"];
		if (pSphereMesh)
		{
			CRenderObject* pObj = new CRenderObject();
			pObj->m_pStaticMesh = pSphereMesh;
			pObj->m_Transform.Position = XMFLOAT3(4.0f, 2.0f, 0.0f);
			pObj->m_Transform.Scale = XMFLOAT3(5.0f, 5.0f, 5.0f);

			m_RenderObjects.push_back(pObj);
		}
	}

	{
		CStaticMesh* pSphereMesh = m_StaticMeshes["UVSphere"];
		if (pSphereMesh)
		{
			CRenderObject* pObj = new CRenderObject();
			pObj->m_pStaticMesh = pSphereMesh;
			pObj->m_Transform.Position = XMFLOAT3(0.0f, 2.0f, 0.0f);
			pObj->m_Transform.Scale = XMFLOAT3(2.0f, 2.0f, 2.0f);
			m_RenderObjects.push_back(pObj);
		}
	}

	{
		CStaticMesh* pSphereMesh = m_StaticMeshes["SphereMesh"];
		if (pSphereMesh)
		{
			CRenderObject* pObj = new CRenderObject();
			pObj->m_pStaticMesh = pSphereMesh;
			pObj->m_Transform.Position = XMFLOAT3(0.0f, 0.0f, 0.0f);
			pObj->m_Transform.Scale = XMFLOAT3(1000.0f, 1000.0f, 1000.0f);

			m_RenderObjects.push_back(pObj);
			
			// 整个系统还没有处理好，应该SkySphere可以作为一个整体来用
			// 需要有一个中央的资源分配器，方便进行查询，以及资源的重复创建
			m_pSkySphere = new CSkySphere();
			m_pSkySphere->SetMesh(pObj);
			m_pSkySphere->Init(m_pGraphicContext);
		}
	}

	{
		CDirectionalLight* pLight = new CDirectionalLight();
		m_DirLights.push_back(pLight);
		pLight->m_Color = XMFLOAT3(1.0f, 1.0f, 1.0f);
		pLight->m_fIntensity = 3.14f;
		pLight->m_vDirection = XMVectorSet(1.0f, -1.0f, 0.0f, 1.0f);
	}

	//{
	//	CPointLight* pLight0 = new CPointLight();
	//	m_PointLights.push_back(pLight0);
	//	pLight0->m_Color = XMFLOAT3(1.0f, 1.0f, 1.0f);
	//	pLight0->m_fIntensity = 50.0f; //3.14f;
	//	pLight0->m_vPosition = XMVectorSet(10.0f, 5.0f, 0.0f, 1.0f);
	//	pLight0->m_fMaxRadius = 10.0f;
	//	pLight0->m_fRefDist = 1.0f;
	//}

	//{
	//	CPointLight* pLight0 = new CPointLight();
	//	m_PointLights.push_back(pLight0);
	//	pLight0->m_Color = XMFLOAT3(1.0f, 1.0f, 1.0f);
	//	pLight0->m_fIntensity = 50.0f; //3.14f;
	//	pLight0->m_vPosition = XMVectorSet(0.0f, 5.0f, 0.0f, 1.0f);
	//	pLight0->m_fMaxRadius = 10.0f;
	//	pLight0->m_fRefDist = 1.0f;
	//}
}

void CMaterialBRDFApp::BuildHeapDescriptors()
{
	m_ObjectBuffer.CreateBuffer(m_pDevice, (UINT)m_RenderObjects.size());

	auto DescriptorAlloctor = m_pGraphicContext->m_pSRVAllocator;

	for (int i = 0; i < m_RenderObjects.size(); ++i)
	{
		DescriptorAddress address = DescriptorAlloctor->Allocate(1, m_pGraphicContext->m_pDevice);

		m_RenderObjects[i]->m_ObjectAddress.nBufferIndex = i;
		m_RenderObjects[i]->m_ObjectAddress.pBuffer = m_ObjectBuffer.m_pUploadeConstBuffer;
		m_RenderObjects[i]->m_ObjectAddress.pBufferHeap = address.pHeap;
		m_RenderObjects[i]->m_ObjectAddress.CPUHandle = address.CpuHandle;
		m_RenderObjects[i]->m_ObjectAddress.GPUHandle = address.GpuHandle;
		
		m_ObjectBuffer.CreateBufferView(m_pDevice, address.CpuHandle, i);
	}

	m_MaterialBuffer.CreateBuffer(m_pDevice, (UINT)m_Materials.size());
	int nMaterialIndex = 0;
	for (auto it = m_Materials.begin(); it != m_Materials.end(); ++it)
	{
		DescriptorAddress address = DescriptorAlloctor->Allocate(1, m_pGraphicContext->m_pDevice);

		it->second->m_MaterialAddress.nBufferIndex = nMaterialIndex;
		it->second->m_MaterialAddress.pBuffer = m_MaterialBuffer.m_pUploadeConstBuffer;
		it->second->m_MaterialAddress.pBufferHeap = address.pHeap;
		it->second->m_MaterialAddress.CPUHandle = address.CpuHandle;
		it->second->m_MaterialAddress.GPUHandle = address.GpuHandle;
		
		m_MaterialBuffer.CreateBufferView(m_pDevice, address.CpuHandle, nMaterialIndex);
		nMaterialIndex++;
	}

	DescriptorAddress address = DescriptorAlloctor->Allocate(1, m_pGraphicContext->m_pDevice);
	m_FrameBuffer.CreateBuffer(m_pDevice);
	m_FrameBuffer.CreateBufferView(m_pDevice, address.CpuHandle);
}

void CMaterialBRDFApp::InitImgui()
{
	// Setup Dear ImGui binding
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;

	ImGui_ImplWin32_Init(m_hWnd);
	
	DescriptorAddress address = m_pGraphicContext->m_pSRVAllocator->Allocate(1, m_pGraphicContext->m_pDevice);
	ImGui_ImplDX12_Init(m_pDevice, 1, DXGI_FORMAT_R8G8B8A8_UNORM, address.CpuHandle, address.GpuHandle);

	// Setup style
	ImGui::StyleColorsDark();
}

void CMaterialBRDFApp::UpdateImgui()
{
	// Start the Dear ImGui frame
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	
	{
		ImGui::Begin("BRDF Property");                          // Create a window called "Hello, world!" and append into it.
		//ImGui::Text("Property");               // Display some text (you can use a format strings too)

		ImGui::ColorEdit4("Base Color", (float*)&m_pBRDFMat->m_cBaseColor);
		ImGui::SliderFloat("Smoothness", &m_pBRDFMat->m_fSmoothness, 0.0f, 1.0f);
		ImGui::SliderFloat("MetalMask", &m_pBRDFMat->m_fMetalMask, 0.0f, 1.0f);
		ImGui::SliderFloat("Reflectance", &m_pBRDFMat->m_fReflectance, 0.0f, 1.0f);

		ImGui::Dummy(ImVec2(0.0, 8.0f));
		ImGui::Text("F9: GUI Mode");
		ImGui::Text("Status: %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::End();
	}
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
		case VK_F9:
			m_bGuiMode = !m_bGuiMode;
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
		if (m_bGuiMode == false)
		{
			int x = LOWORD(lParam);//鼠标的横坐标
			int y = HIWORD(lParam);//鼠标的纵坐标

			m_InputMgr.m_nMouseX = x;
			m_InputMgr.m_nMouseY = x;

			m_InputMgr.m_nDelteMouseX = x - m_InputMgr.m_nLastMouseX;
			m_InputMgr.m_nDeltaMouseY = y - m_InputMgr.m_nLastMouseY;

			m_InputMgr.m_nLastMouseX = x;
			m_InputMgr.m_nLastMouseY = y;
		}
		
		return 0;
	}
	case WM_MOUSEWHEEL:
	{
		int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
		m_InputMgr.m_nMouseZDelta = zDelta;
		return 0;
	}
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
}