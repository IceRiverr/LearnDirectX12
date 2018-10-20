#include "SkySphere.h"
#include "GraphicsUtility.h"


void CSkySphere::SetMesh(CRenderObject* pRender)
{
	m_pRenderObj = pRender;
}

void CSkySphere::Init(CGraphicContext* pContext)
{
	if (pContext)
	{
		// Shader
		std::string m_ShaderRootPath = "D:\\Projects\\MyProjects\\LearnDirectX12\\D3D12Demo\\Shaders\\";

		m_pVSShaderCode_SkySphere = Graphics::CompileShader(m_ShaderRootPath + "SkySphereShader.fx", "VSMain", "vs_5_0");
		m_pPSShaderCode_SkySphere = Graphics::CompileShader(m_ShaderRootPath + "SkySphereShader.fx", "PSMain", "ps_5_0");
		
		D3D12_GRAPHICS_PIPELINE_STATE_DESC OpaquePSODesc = {};
		auto InputLayout = GetInputLayout(INPUT_LAYOUT_TYPE::P3);
		OpaquePSODesc.InputLayout = { InputLayout.data(), (UINT)InputLayout.size() };
		OpaquePSODesc.pRootSignature = pContext->m_pRootSignature;
		OpaquePSODesc.VS = { m_pVSShaderCode_SkySphere->GetBufferPointer(), m_pVSShaderCode_SkySphere->GetBufferSize() };
		OpaquePSODesc.PS = { m_pPSShaderCode_SkySphere->GetBufferPointer(), m_pPSShaderCode_SkySphere->GetBufferSize() };
		OpaquePSODesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		OpaquePSODesc.RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_NONE;
		OpaquePSODesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		OpaquePSODesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		OpaquePSODesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS_EQUAL;
		OpaquePSODesc.SampleMask = UINT_MAX;
		OpaquePSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		OpaquePSODesc.NumRenderTargets = 1;
		OpaquePSODesc.RTVFormats[0] = DXGI_FORMAT_B8G8R8A8_UNORM;
		OpaquePSODesc.SampleDesc.Count = 1;
		OpaquePSODesc.SampleDesc.Quality = 0;
		OpaquePSODesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

		pContext->m_pDevice->CreateGraphicsPipelineState(&OpaquePSODesc, IID_PPV_ARGS(&m_PSO));

		// Background Image
		std::string  sContentRootPath = "D:\\Projects\\MyProjects\\LearnDirectX12\\D3D12Demo\\Content\\";
		
		std::string backgroundPath = sContentRootPath + "WellesleyGreenhouse3\\Greenhouse3_Bg.tga";
		std::string enviromnentPath = sContentRootPath + "WellesleyGreenhouse3\\Greenhouse3_Env.hdr";
		std::string reflectionPath = sContentRootPath + "WellesleyGreenhouse3\\Greenhouse3_Ref.hdr";
		
		m_pBackGroundMap = pContext->CreateTexture(backgroundPath);
		m_pEnvironmentMap = pContext->CreateTexture(enviromnentPath);
		m_pReflectionMap = pContext->CreateTexture(reflectionPath);
	}
}

void CSkySphere::Draw(ID3D12GraphicsCommandList* pCommandList)
{
	pCommandList->SetPipelineState(m_PSO);

	D3D12_VERTEX_BUFFER_VIEW VertexBufferViews[] =
	{
		m_pRenderObj->m_pStaticMesh->m_PositionBufferView,
		m_pRenderObj->m_pStaticMesh->m_UVBufferView,
	};
	pCommandList->IASetVertexBuffers(0, 2, &VertexBufferViews[0]);

	pCommandList->IASetIndexBuffer(&m_pRenderObj->m_pStaticMesh->m_IndexBufferView);
	pCommandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	pCommandList->SetGraphicsRootDescriptorTable((UINT)ROOT_SIGNATURE_INDEX::OBJECT_BUFFER_INDEX, m_pRenderObj->m_ObjectAddress.GPUHandle);
	pCommandList->SetGraphicsRootDescriptorTable((UINT)ROOT_SIGNATURE_INDEX::SKY_SPHERE_BACKGROUND_INDEX, m_pBackGroundMap->m_TextureAddress.GPUHandle);

	// Draw
	for (auto it : m_pRenderObj->m_pStaticMesh->m_SubMeshes)
	{
		SubMesh subMesh = it.second;
		pCommandList->DrawIndexedInstanced(subMesh.nIndexCount, 1, subMesh.nStartIndexLocation, subMesh.nBaseVertexLocation, 0);
	}
}
