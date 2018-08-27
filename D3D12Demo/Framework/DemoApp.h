#pragma once
#include <d3d12.h>
#include "d3dx12.h"
#include <dxgi1_6.h>
#include <Windows.h>
#include <DirectXMath.h>
#include <d3dcompiler.h>
#include <iostream>
#include <string>

using namespace DirectX;

class DemoApp
{
public:
	DemoApp();
	~DemoApp();

	virtual void Init();
	virtual void Update(double deltaTime);
	virtual void Draw();
	virtual void OnResize();

	virtual LRESULT WndMsgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	void InitConsoleWindow();
	void InitD3D12();
	void LogAdapters(IDXGIFactory* pFactory);
	ID3D12Resource* GetCurrentBackBuffer();
	D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentBackBufferView();
	D3D12_CPU_DESCRIPTOR_HANDLE GetDepthStencilView();

	// Graphics API
	void FlushCommandQueue();

	ID3D12Resource* CreateDefaultBuffer(ID3D12Device* pDevice, ID3D12GraphicsCommandList* cmdList, const void* initData, UINT64 byteSize, ID3D12Resource** ppUploadBuffer);

public:
	HWND m_hWindow = nullptr;
	HINSTANCE m_hInstance = nullptr;

protected:
	UINT m_nClientWindowWidth;
	UINT m_nClientWindowHeight;

	IDXGIFactory1* m_pFactory;
	ID3D12Device* m_pDevice;

	IDXGISwapChain* m_pSwapChian;
	const static UINT m_nSwapChainBufferCount = 2;
	UINT m_nCurrentSwapChainIndex = 0;
	DXGI_FORMAT m_BackBufferFromat;
	ID3D12Resource* m_pSwapChainBuffers[m_nSwapChainBufferCount];
	
	DXGI_FORMAT m_DSVFormat;
	ID3D12Resource* m_pDSVBuffer;

	ID3D12CommandQueue* m_pCommandQueue;
	ID3D12CommandAllocator* m_pCommandAllocator;
	ID3D12GraphicsCommandList* m_pCommandList;

	ID3D12DescriptorHeap* m_pRTVHeap;
	ID3D12DescriptorHeap* m_pDSVHeap;
	UINT m_nRTVDescriptorSize;
	UINT m_nDSVDescriptorSize;
	UINT m_nSRVDescriptorSize;

	D3D12_VIEWPORT m_ScreenViewPort;
	RECT m_ScissorRect;

	UINT64 m_CurrentFence;
	ID3D12Fence* m_pFence;
};

