
#include "SkyBox.h"
#include "GraphicsUtility.h"
#include "DirectXTex.h"

void CSkyBox::SetMesh(CRenderObject * pRender)
{
	m_pRenderObj = pRender;
}

void CSkyBox::Init(CGraphicContext * pContext)
{
	if (pContext)
	{
		// Shader
		std::string m_ShaderRootPath = "D:\\Projects\\MyProjects\\LearnDirectX12\\D3D12Demo\\Shaders\\";

		m_pVSShaderCode = Graphics::CompileShader(m_ShaderRootPath + "SkyBoxShader.fx", "VSMain", "vs_5_0");
		m_pPSShaderCode = Graphics::CompileShader(m_ShaderRootPath + "SkyBoxShader.fx", "PSMain", "ps_5_0");

		D3D12_GRAPHICS_PIPELINE_STATE_DESC OpaquePSODesc = {};
		auto InputLayout = GetInputLayout(INPUT_LAYOUT_TYPE::P3);
		OpaquePSODesc.InputLayout = { InputLayout.data(), (UINT)InputLayout.size() };
		OpaquePSODesc.pRootSignature = pContext->m_pRootSignature;
		OpaquePSODesc.VS = { m_pVSShaderCode->GetBufferPointer(), m_pVSShaderCode->GetBufferSize() };
		OpaquePSODesc.PS = { m_pPSShaderCode->GetBufferPointer(), m_pPSShaderCode->GetBufferSize() };
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
		BuildCubeMap(pContext);
	}
}

void CSkyBox::Draw(ID3D12GraphicsCommandList * pCommandList)
{
	pCommandList->SetPipelineState(m_PSO);

	D3D12_VERTEX_BUFFER_VIEW VertexBufferViews[] =
	{
		m_pRenderObj->m_pStaticMesh->m_PositionBufferView,
		//m_pRenderObj->m_pStaticMesh->m_UVBufferView,
	};
	pCommandList->IASetVertexBuffers(0, 1, &VertexBufferViews[0]);

	pCommandList->IASetIndexBuffer(&m_pRenderObj->m_pStaticMesh->m_IndexBufferView);
	pCommandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	pCommandList->SetGraphicsRootDescriptorTable((UINT)ROOT_SIGNATURE_INDEX::OBJECT_BUFFER_INDEX, m_pRenderObj->m_ObjectAddress.GPUHandle);
	pCommandList->SetGraphicsRootDescriptorTable((UINT)ROOT_SIGNATURE_INDEX::SKY_SPHERE_BACKGROUND_INDEX, m_TextureAddress.GPUHandle);

	// Draw
	for (auto it : m_pRenderObj->m_pStaticMesh->m_SubMeshes)
	{
		SubMesh subMesh = it.second;
		pCommandList->DrawIndexedInstanced(subMesh.nIndexCount, 1, subMesh.nStartIndexLocation, subMesh.nBaseVertexLocation, 0);
	}
}

void CSkyBox::BuildCubeMap(CGraphicContext * pContext)
{
	// Background Image
	std::string  sContentRootPath = "D:\\Projects\\MyProjects\\LearnDirectX12\\D3D12Demo\\Content\\";
	// (+X, -X, +Y, -Y, +Z, -Z) 
	std::string cubeBoxPath[6] =
	{
		sContentRootPath + "WellesleyGreenhouse3\\cubemap\\px.png",
		sContentRootPath + "WellesleyGreenhouse3\\cubemap\\nx.png",
		sContentRootPath + "WellesleyGreenhouse3\\cubemap\\py.png",
		sContentRootPath + "WellesleyGreenhouse3\\cubemap\\ny.png",
		sContentRootPath + "WellesleyGreenhouse3\\cubemap\\pz.png",
		sContentRootPath + "WellesleyGreenhouse3\\cubemap\\nz.png"
	};

	/*std::string cubeBoxPath[6] =
	{
		sContentRootPath + "PaperMill_Ruins_E\\cubemap\\px.png",
		sContentRootPath + "PaperMill_Ruins_E\\cubemap\\nx.png",
		sContentRootPath + "PaperMill_Ruins_E\\cubemap\\py.png",
		sContentRootPath + "PaperMill_Ruins_E\\cubemap\\ny.png",
		sContentRootPath + "PaperMill_Ruins_E\\cubemap\\pz.png",
		sContentRootPath + "PaperMill_Ruins_E\\cubemap\\nz.png"
	};*/

	{
		auto baseImage = std::make_unique<ScratchImage>();
		HRESULT hr = LoadFromWICFile(StringToWString(cubeBoxPath[0]).c_str(), WIC_FLAGS_NONE, nullptr, *baseImage);
		auto metadata = baseImage->GetMetadata();
		if (hr == S_OK)
		{
			D3D12_RESOURCE_DESC desc = {};
			desc.Width = static_cast<UINT>(metadata.width);
			desc.Height = static_cast<UINT>(metadata.height);
			desc.MipLevels = static_cast<UINT16>(metadata.mipLevels);
			desc.DepthOrArraySize = 6;
			desc.Format = metadata.format;
			desc.Flags = D3D12_RESOURCE_FLAG_NONE;
			desc.SampleDesc.Count = 1;
			desc.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(metadata.dimension);

			CD3DX12_HEAP_PROPERTIES defaultHeapProperties(D3D12_HEAP_TYPE_DEFAULT);

			hr = pContext->m_pDevice->CreateCommittedResource(
				&defaultHeapProperties,
				D3D12_HEAP_FLAG_NONE,
				&desc,
				D3D12_RESOURCE_STATE_COPY_DEST,
				nullptr,
				IID_PPV_ARGS(&m_pCubeMap));
		}
	}

	if (m_pCubeMap != nullptr)
	{
		for (int i = 0; i < 6; ++i)
		{
			auto baseImage = std::make_unique<ScratchImage>();
			HRESULT hr = LoadFromWICFile(StringToWString(cubeBoxPath[i]).c_str(), WIC_FLAGS_NONE, nullptr, *baseImage);

			if (hr == S_OK)
			{
				std::vector<D3D12_SUBRESOURCE_DATA> subresources;
				hr = PrepareUpload(pContext->m_pDevice, baseImage->GetImages(), baseImage->GetImageCount(), baseImage->GetMetadata(), subresources);

				const UINT64 uploadBufferSize = GetRequiredIntermediateSize(m_pCubeMap, 0, (UINT)subresources.size());

				hr = pContext->m_pDevice->CreateCommittedResource(
					&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
					D3D12_HEAP_FLAG_NONE,
					&CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
					D3D12_RESOURCE_STATE_GENERIC_READ,
					nullptr,
					IID_PPV_ARGS(&m_pUpdateBuffer[i]));

				UpdateSubresources(pContext->m_pCommandList, m_pCubeMap, m_pUpdateBuffer[i], 0, i, (UINT)subresources.size(), subresources.data());
			}
		}
	}

	// SRV
	DescriptorAddress address = pContext->m_pSRVAllocator->Allocate(1, pContext->m_pDevice);
	m_TextureAddress.pBufferHeap = address.pHeap;
	m_TextureAddress.CPUHandle = address.CpuHandle;
	m_TextureAddress.GPUHandle = address.GpuHandle;

	// Create SRV
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = m_pCubeMap->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = m_pCubeMap->GetDesc().MipLevels;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

	pContext->m_pDevice->CreateShaderResourceView(m_pCubeMap, &srvDesc, m_TextureAddress.CPUHandle);
}

void convert_cube_uv_to_xyz(int index, float u, float v, float * x, float * y, float * z)
{
	// convert range 0 to 1 to -1 to 1
	float uc = 2.0f * u - 1.0f;
	float vc = 2.0f * v - 1.0f;
	switch (index)
	{
	case 0: *x = 1.0f; *y = vc; *z = -uc; break;	// POSITIVE X
	case 1: *x = -1.0f; *y = vc; *z = uc; break;	// NEGATIVE X
	case 2: *x = uc; *y = 1.0f; *z = -vc; break;	// POSITIVE Y
	case 3: *x = uc; *y = -1.0f; *z = vc; break;	// NEGATIVE Y
	case 4: *x = uc; *y = vc; *z = 1.0f; break;		// POSITIVE Z
	case 5: *x = -uc; *y = vc; *z = -1.0f; break;	// NEGATIVE Z
	}
}

void convert_xyz_to_cube_uv(float x, float y, float z, int * index, float * u, float * v)
{
	float absX = fabs(x);
	float absY = fabs(y);
	float absZ = fabs(z);

	int isXPositive = x > 0 ? 1 : 0;
	int isYPositive = y > 0 ? 1 : 0;
	int isZPositive = z > 0 ? 1 : 0;

	float maxAxis, uc, vc;

	// POSITIVE X
	if (isXPositive && absX >= absY && absX >= absZ) {
		// u (0 to 1) goes from +z to -z
		// v (0 to 1) goes from -y to +y
		maxAxis = absX;
		uc = -z;
		vc = y;
		*index = 0;
	}
	// NEGATIVE X
	if (!isXPositive && absX >= absY && absX >= absZ) {
		// u (0 to 1) goes from -z to +z
		// v (0 to 1) goes from -y to +y
		maxAxis = absX;
		uc = z;
		vc = y;
		*index = 1;
	}
	// POSITIVE Y
	if (isYPositive && absY >= absX && absY >= absZ) {
		// u (0 to 1) goes from -x to +x
		// v (0 to 1) goes from +z to -z
		maxAxis = absY;
		uc = x;
		vc = -z;
		*index = 2;
	}
	// NEGATIVE Y
	if (!isYPositive && absY >= absX && absY >= absZ) {
		// u (0 to 1) goes from -x to +x
		// v (0 to 1) goes from -z to +z
		maxAxis = absY;
		uc = x;
		vc = z;
		*index = 3;
	}
	// POSITIVE Z
	if (isZPositive && absZ >= absX && absZ >= absY) {
		// u (0 to 1) goes from -x to +x
		// v (0 to 1) goes from -y to +y
		maxAxis = absZ;
		uc = x;
		vc = y;
		*index = 4;
	}
	// NEGATIVE Z
	if (!isZPositive && absZ >= absX && absZ >= absY) {
		// u (0 to 1) goes from +x to -x
		// v (0 to 1) goes from -y to +y
		maxAxis = absZ;
		uc = -x;
		vc = y;
		*index = 5;
	}

	// Convert range from -1 to 1 to 0 to 1
	*u = 0.5f * (uc / maxAxis + 1.0f);
	*v = 0.5f * (vc / maxAxis + 1.0f);
}

