#pragma once
#include <d3d12.h>
#include "d3dx12.h"
#include <dxgi1_6.h>
#include <Windows.h>
#include <DirectXMath.h>
#include <d3dcompiler.h>
#include <iostream>
#include <string>

#include "Utility.h"
#include "GraphicsUtility.h"

using namespace DirectX;
using namespace Graphics;

class WinApp
{
public:
	WinApp();
	~WinApp();

	virtual void Init();
	virtual void Update(double deltaTime);
	virtual void Draw();
	virtual void OnResize();

	virtual LRESULT WndMsgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	void InitD3D12();
	ID3D12Resource* GetCurrentBackBuffer();
	D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentBackBufferView();
	D3D12_CPU_DESCRIPTOR_HANDLE GetDepthStencilView();

	// Graphics API
	void EnableDebugLayer();
	void FlushCommandQueue();

	void InitConsoleWindow();
	void ParseCommandLineArguments();

	bool CheckTearingSupported();

	void SetFullScreen(bool bFullScreen);

	void LogAdapters(IDXGIFactory* pFactory);

	void QueryFeatureData(ID3D12Device* pDevice);

public:
	HWND m_hWnd = nullptr;
	HINSTANCE m_hInstance = nullptr;
	UINT m_nClientWindowWidth;
	UINT m_nClientWindowHeight;

protected:
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

	bool m_bVSync; // V-SYNC 垂直同步
	bool m_bTearingSupported;
	bool m_bFullScreen;
	RECT m_WindowRect; //全屏之前的窗口
};

