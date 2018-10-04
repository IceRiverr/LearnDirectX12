#include "Material_BRDF.h"
#include "ImportObj.h"

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"

#include "DirectXTex.h"

CMaterialBRDFApp::CMaterialBRDFApp()
{
	m_ContentRootPath = "D:\\Projects\\MyProjects\\LearnDirectX12\\D3D12Demo\\Content\\";
	m_ShaderRootPath = "D:\\Projects\\MyProjects\\LearnDirectX12\\D3D12Demo\\Shaders\\";

	m_pCBVHeap = nullptr;
	m_pRootSignature = nullptr;
	m_pVSShaderCode_Light = nullptr;
	m_pPSShaderCode_Light = nullptr;

	clear_color = { 135.0f / 255.0f, 206.0f / 255.0f, 250.0f / 255.0f, 1.0f };
	m_pBRDFMat = nullptr;

	m_bGuiMode = false;

	m_pAldeboMap = nullptr;
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

	m_pCamera = new CFPSCamera();
	m_pCamera->Init(90.0f, m_nClientWindowWidth * 1.0f / m_nClientWindowHeight, 1.0f, 1000.0f);
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

// https://github.com/Microsoft/DirectXTex/wiki/CreateTexture
// https://github.com/Microsoft/DirectXTex/wiki/GenerateMipMaps
void CMaterialBRDFApp::CreateTextureResources()
{
	{
		std::wstring wImagePath = StringToWString(m_ContentRootPath + "Gun\\Textures\\Cerberus_A.tga");
		m_pAldeboMap = Graphics::CreateTexture2DResourceFromFile(m_pDevice, m_pCommandList, wImagePath);
		m_Textures.emplace("Cerberus_A", m_pAldeboMap);
	}

	{
		std::wstring wImagePath = StringToWString(m_ContentRootPath + "Gun\\Textures\\Cerberus_N.tga");
		m_pNormalMap = Graphics::CreateTexture2DResourceFromFile(m_pDevice, m_pCommandList, wImagePath);
		m_Textures.emplace("Cerberus_N", m_pNormalMap);
	}

	{
		std::wstring wImagePath = StringToWString(m_ContentRootPath + "Gun\\Textures\\Cerberus_R.tga");
		m_pRoughnessMap = Graphics::CreateTexture2DResourceFromFile(m_pDevice, m_pCommandList, wImagePath);
		m_Textures.emplace("Cerberus_R", m_pRoughnessMap);
	}

	{
		std::wstring wImagePath = StringToWString(m_ContentRootPath + "Gun\\Textures\\Cerberus_M.tga");
		m_pMetalicMap = Graphics::CreateTexture2DResourceFromFile(m_pDevice, m_pCommandList, wImagePath);
		m_Textures.emplace("m_pMetalicMap", m_pMetalicMap);
	}
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

	for (int i = 0; i < m_RenderObjects.size(); ++i)
	{
		CRenderObject* pObj = m_RenderObjects[i];

		if (pObj->m_pStaticMesh->m_pMaterial->m_sName == "BRDF_Color")
		{
			m_pCommandList->SetPipelineState(m_PSOs["Light_PSO"]);
			D3D12_VERTEX_BUFFER_VIEW VertexBufferViews[] =
			{
				pObj->m_pStaticMesh->m_PositionBufferView,
				pObj->m_pStaticMesh->m_NormalBufferView,
			};

			m_pCommandList->IASetVertexBuffers(0, 2, &VertexBufferViews[0]);
		}
		else if (pObj->m_pStaticMesh->m_pMaterial->m_sName == "BRDF_Texture")
		{
			m_pCommandList->SetPipelineState(m_PSOs["Material_PSO"]);
			D3D12_VERTEX_BUFFER_VIEW VertexBufferViews[] =
			{
				pObj->m_pStaticMesh->m_PositionBufferView,
				pObj->m_pStaticMesh->m_NormalBufferView,
				pObj->m_pStaticMesh->m_TangentBufferView,
				pObj->m_pStaticMesh->m_UVBufferView,
			};

			m_pCommandList->IASetVertexBuffers(0, 4, &VertexBufferViews[0]);

			auto handle = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_pCBVHeap->GetGPUDescriptorHandleForHeapStart());
			handle.Offset(m_pAldeboMap->m_TextureAddress.nHeapOffset, m_nSRVDescriptorSize);
			m_pCommandList->SetGraphicsRootDescriptorTable(3, handle);
		}
		
		m_pCommandList->IASetIndexBuffer(&pObj->m_pStaticMesh->m_IndexBufferView);
		m_pCommandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		auto handle1 = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_pCBVHeap->GetGPUDescriptorHandleForHeapStart());
		handle1.Offset(pObj->m_ObjectAddress.nHeapOffset, m_nSRVDescriptorSize);
		m_pCommandList->SetGraphicsRootDescriptorTable(0, handle1);

		auto handle = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_pCBVHeap->GetGPUDescriptorHandleForHeapStart());
		handle.Offset(m_FrameBuffer.m_nDescriptorIndex, m_nSRVDescriptorSize);
		m_pCommandList->SetGraphicsRootDescriptorTable(1, handle);

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

void CMaterialBRDFApp::InitRenderResource()
{
	m_pCommandList->Reset(m_pCommandAllocator, nullptr);
	// 所有初始化命令都放到该命令之后

	CreateTextureResources();
	BuildMaterials();
	BuildStaticMeshes(m_pDevice, m_pCommandList);
	BuildScene();
	BuildHeapDescriptors();

	// create root signature
	CD3DX12_DESCRIPTOR_RANGE cbvTable;
	cbvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);

	CD3DX12_DESCRIPTOR_RANGE cbvTable1;
	cbvTable1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1);

	CD3DX12_DESCRIPTOR_RANGE cbvTable2;
	cbvTable2.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 2);

	CD3DX12_DESCRIPTOR_RANGE textureTable;
	textureTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 0);

	CD3DX12_ROOT_PARAMETER slotRootParameter[4];
	slotRootParameter[0].InitAsDescriptorTable(1, &cbvTable);
	slotRootParameter[1].InitAsDescriptorTable(1, &cbvTable1);
	slotRootParameter[2].InitAsDescriptorTable(1, &cbvTable2);
	slotRootParameter[3].InitAsDescriptorTable(1, &textureTable, D3D12_SHADER_VISIBILITY_PIXEL);

	StaticSamplerStates StaticSamplers;
	StaticSamplers.CreateStaticSamplers();

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(4, slotRootParameter, STATIC_SAMPLER_TYPE::SAMPLER_COUNT, StaticSamplers.Samplers.data(), D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

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

	m_pVSShaderCode_Light = Graphics::CompileShader(m_ShaderRootPath + "light_material.hlsl", "VSMain", "vs_5_0");
	m_pPSShaderCode_Light = Graphics::CompileShader(m_ShaderRootPath + "light_material.hlsl", "PSMain", "ps_5_0");

	m_PositionNomralUVInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL",	  0, DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL",	  1, DXGI_FORMAT_R32G32B32A32_FLOAT, 2, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD",  0, DXGI_FORMAT_R32G32_FLOAT, 3, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA , 0}
	};

	m_pVSShaderCode_Material = Graphics::CompileShader(m_ShaderRootPath + "Mat_DefaultShader.hlsl", "VSMain", "vs_5_0");
	m_pPSShaderCode_Material = Graphics::CompileShader(m_ShaderRootPath + "Mat_DefaultShader.hlsl", "PSMain", "ps_5_0");

	BuildPSOs(m_pDevice);

	m_pCommandList->Close();

	// flush command
	ID3D12CommandList* cmdLists[1] = { m_pCommandList };
	m_pCommandQueue->ExecuteCommandLists(1, cmdLists);
	FlushCommandQueue();
}

void CMaterialBRDFApp::BuildMaterials()
{
	{
		m_pBRDFMat = new CMaterial();
		m_pBRDFMat->m_sName = "BRDF_Color";
		m_pBRDFMat->m_cBaseColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		m_pBRDFMat->m_fSmoothness = 0.2f;
		m_pBRDFMat->m_fMetalMask = 0.0f;
		m_pBRDFMat->m_fReflectance = 0.2f;
		m_Materials.emplace(m_pBRDFMat->m_sName, m_pBRDFMat);
	}

	{
		CMaterial* pMat = new CMaterial();
		pMat->m_sName = "BRDF_Texture";
		pMat->m_cBaseColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		pMat->m_fSmoothness = 0.2f;
		pMat->m_fMetalMask = 0.0f;
		pMat->m_fReflectance = 0.8f;
		m_Materials.emplace(pMat->m_sName, pMat);

		
	}
}

void CMaterialBRDFApp::BuildStaticMeshes(ID3D12Device* pDevice, ID3D12GraphicsCommandList* cmdList)
{
	{
		CStaticMesh* pMesh = new CStaticMesh();
		pMesh->ImportFromFile(m_ContentRootPath + "plane.obj", pDevice, cmdList); // smooth_box plane  scene_simple
		m_StaticMeshes.emplace("Plane_Obj", pMesh);
		pMesh->m_pMaterial = m_Materials["BRDF_Texture"];
	}
	
	{
		std::string gun_model_path = m_ContentRootPath + "Gun\\gun.obj";
		std::string smooth_box_path = m_ContentRootPath + "smooth_box.obj";
		CStaticMesh* pMesh = new CStaticMesh();
		pMesh->ImportFromFile(gun_model_path, pDevice, cmdList);
		m_StaticMeshes.emplace("Smooth_box", pMesh);
		pMesh->m_pMaterial = m_Materials["BRDF_Texture"];
	}

	{
		CStaticMesh* pMesh = new CStaticMesh();
		pMesh->ImportFromFile(m_ContentRootPath + "UVSphere.obj", pDevice, cmdList);
		m_StaticMeshes.emplace("UVSphere", pMesh);
		pMesh->m_pMaterial = m_Materials["BRDF_Color"];
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
			pObj->m_Transform.Position = XMFLOAT3(0.0f, 1.0f, 0.0f);

			m_RenderObjects.push_back(pObj);
		}
	}

	{
		CDirectionalLight* pLight = new CDirectionalLight();
		m_DirLights.push_back(pLight);
		pLight->m_Color = XMFLOAT3(1.0f, 1.0f, 1.0f);
		pLight->m_fIntensity = 3.14f;
		pLight->m_vDirection = XMVectorSet(1.0f, -1.0f, 0.0f, 1.0f);
	}

	{
		CPointLight* pLight0 = new CPointLight();
		m_PointLights.push_back(pLight0);
		pLight0->m_Color = XMFLOAT3(1.0f, 1.0f, 1.0f);
		pLight0->m_fIntensity = 50.0f; //3.14f;
		pLight0->m_vPosition = XMVectorSet(10.0f, 5.0f, 0.0f, 1.0f);
		pLight0->m_fMaxRadius = 10.0f;
		pLight0->m_fRefDist = 1.0f;
	}
}

void CMaterialBRDFApp::BuildHeapDescriptors()
{
	// create cbv heap
	D3D12_DESCRIPTOR_HEAP_DESC cbHeapDesc = {};
	// obj + material + textures + frameBuffer + imgui
	cbHeapDesc.NumDescriptors = 
		(UINT)m_RenderObjects.size() + 
		(UINT)m_Materials.size() + 
		(UINT)m_Textures.size() + 
		1 + 1;
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

	for (auto it = m_Textures.begin(); it != m_Textures.end(); ++it)
	{
		Texture2DResource* pResource = it->second;
		pResource->m_TextureAddress.pBufferHeap = m_pCBVHeap;
		pResource->m_TextureAddress.nHeapOffset = nDescriptorIndex;

		auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_pCBVHeap->GetCPUDescriptorHandleForHeapStart());
		handle.Offset(nDescriptorIndex, m_nSRVDescriptorSize);
		
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = pResource->pTexture->GetDesc().Format;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels = pResource->pTexture->GetDesc().MipLevels;
		srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

		m_pDevice->CreateShaderResourceView(pResource->pTexture, &srvDesc, handle);

		nDescriptorIndex++;
	}

	m_FrameBuffer.CreateBuffer(m_pDevice);
	m_FrameBuffer.m_nDescriptorIndex = nDescriptorIndex++;
	auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_pCBVHeap->GetCPUDescriptorHandleForHeapStart());
	handle.Offset(m_FrameBuffer.m_nDescriptorIndex, m_nSRVDescriptorSize);
	m_FrameBuffer.CreateBufferView(m_pDevice, handle);

	m_imguiDescriptorIndex = nDescriptorIndex;
}

void CMaterialBRDFApp::BuildPSOs(ID3D12Device* pDevice)
{
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
	
	{
		ID3D12PipelineState* pOpaquePSO = nullptr;
		D3D12_GRAPHICS_PIPELINE_STATE_DESC OpaquePSODesc = {};
		OpaquePSODesc.InputLayout = { m_PositionNomralUVInputLayout.data(), (UINT)m_PositionNomralUVInputLayout.size() };
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