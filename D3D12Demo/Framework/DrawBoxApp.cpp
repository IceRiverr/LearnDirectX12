#include "DrawBoxApp.h"

DrawBoxApp::DrawBoxApp()
{
	m_pCBVHeap = nullptr;
	m_pRootSignature = nullptr;
}

DrawBoxApp::~DrawBoxApp()
{

}

void DrawBoxApp::Init()
{
	// create cbv heap
	D3D12_DESCRIPTOR_HEAP_DESC cbHeapDesc = {};
	cbHeapDesc.NumDescriptors = 1;
	cbHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cbHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	cbHeapDesc.NodeMask = 0;

	m_pDevice->CreateDescriptorHeap(&cbHeapDesc, IID_PPV_ARGS(&m_pCBVHeap));

	// create const buffer ±ØÐë256b¶ÔÆë
	int nConstantBufferSizeAligned = (sizeof(ObjectConstants) + 255) & (~255);
	m_pDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(nConstantBufferSizeAligned),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_pUploadeConstBuffer));

	D3D12_GPU_VIRTUAL_ADDRESS cbAddress = m_pUploadeConstBuffer->GetGPUVirtualAddress();

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
	cbvDesc.BufferLocation = 0;
	cbvDesc.SizeInBytes = nConstantBufferSizeAligned;

	m_pDevice->CreateConstantBufferView(&cbvDesc, m_pCBVHeap->GetCPUDescriptorHandleForHeapStart());

	// create root signature
	CD3DX12_ROOT_PARAMETER slotRootParameter[1];

	CD3DX12_DESCRIPTOR_RANGE cbvTable;
	cbvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);

	slotRootParameter[0].InitAsDescriptorTable(1, &cbvTable);

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(1, slotRootParameter, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

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

void DrawBoxApp::Update(double deltaTime)
{
}

void DrawBoxApp::Draw()
{
	DemoApp::Draw();
}
