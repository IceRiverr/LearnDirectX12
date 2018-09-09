
#include "WinApp.h"
#include "Utility.h"
#include <Resource.h>
#include <dxgi1_5.h>

// 处理链接错误的问题
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")

WinApp::WinApp()
{
	m_pFactory = nullptr;
	m_pDevice = nullptr;

	m_pCommandQueue = nullptr;
	m_pCommandAllocator = nullptr;
	m_pCommandList = nullptr;

	m_pRTVHeap = nullptr;
	m_pDSVHeap = nullptr;
	m_nRTVDescriptorSize = 0;
	m_nDSVDescriptorSize = 0;
	m_nSRVDescriptorSize = 0;

	m_pDSVBuffer = nullptr;
	
	m_pSwapChian = nullptr;
	for (int i = 0; i < m_nSwapChainBufferCount; ++i)
	{
		m_pSwapChainBuffers[i] = nullptr;
	}

	m_BackBufferFromat = DXGI_FORMAT_B8G8R8A8_UNORM;
	m_DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	m_nClientWindowWidth = 1280;
	m_nClientWindowHeight = 720;

	m_CurrentFence = 0;
	m_pFence = nullptr;

	m_bFullScreen = false;
}
WinApp::~WinApp()
{
	
}

void WinApp::InitConsoleWindow()
{
	AllocConsole();
	freopen("CONOUT$", "w+t", stdout);
	printf("hello world\n");
}

void WinApp::ParseCommandLineArguments()
{
	int argc = 0;
	wchar_t** argv = CommandLineToArgvW(GetCommandLineW(), &argc);
	for (int i = 0; i < argc; ++i)
	{
		std::string argvName;
		WStringToString(std::wstring(argv[i]), argvName);
		std::cout << argvName << std::endl;

		if (wcscmp(argv[i], L"-W") == 0)
		{
			int screenWidth = wcstol(argv[++i], nullptr, 10);
		}
	}
	LocalFree(argv);
}

bool WinApp::CheckTearingSupported()
{
	bool bAllowing = false;

	IDXGIFactory5* pFactory = nullptr;
	CreateDXGIFactory1(IID_PPV_ARGS(&pFactory));
	if (pFactory)
	{
		if (FAILED(pFactory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &bAllowing, sizeof(bAllowing))))
		{
			bAllowing = false;
		}
		else
		{
			bAllowing = true;
		}
		pFactory->Release();
	}
	return bAllowing;
}

void WinApp::SetFullScreen(bool bFullScreen)
{
	if (bFullScreen != m_bFullScreen)
	{
		m_bFullScreen = bFullScreen;
		if (m_bFullScreen)
		{
			GetWindowRect(m_hWnd,&m_WindowRect);

			UINT windowStyle = WS_OVERLAPPEDWINDOW & ~(WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
			SetWindowLongW(m_hWnd, GWL_STYLE, windowStyle);

			HMONITOR hMonitor = MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST);
			MONITORINFOEX monitorInfo = {};
			monitorInfo.cbSize = sizeof(monitorInfo);
			GetMonitorInfo(hMonitor, &monitorInfo);

			SetWindowPos(m_hWnd, HWND_TOPMOST,
				monitorInfo.rcMonitor.left,
				monitorInfo.rcMonitor.top,
				monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
				monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
				SWP_FRAMECHANGED | SWP_NOACTIVATE);

			ShowWindow(m_hWnd, SW_MAXIMIZE);
		}
		else
		{
			SetWindowLongW(m_hWnd, GWL_STYLE, WS_OVERLAPPEDWINDOW);

			SetWindowPos(m_hWnd, HWND_TOPMOST,
				m_WindowRect.left,
				m_WindowRect.top,
				m_WindowRect.right - m_WindowRect.left,
				m_WindowRect.bottom - m_WindowRect.top,
				SWP_FRAMECHANGED | SWP_NOACTIVATE);

			ShowWindow(m_hWnd, SW_NORMAL);
		}
	}
}

void WinApp::InitD3D12()
{
	// 一定要在任何D3Dx12 接口使用之间开启DebugLayer，
	EnableDebugLayer();

	CreateDXGIFactory1(IID_PPV_ARGS(&m_pFactory));
	
	LogAdapters(m_pFactory);

	D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&m_pDevice));

	QueryFeatureData(m_pDevice);
	
	m_pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_pFence));
	
	// 检查DescripterHeap的大小，获取每个DescriptorHandle的字节大小，方便在DescriptorHeap中进行遍历
	m_nRTVDescriptorSize = m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	m_nDSVDescriptorSize = m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	m_nSRVDescriptorSize = m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// check  MultiSample
	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels;
	msQualityLevels.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	msQualityLevels.SampleCount = 4;
	msQualityLevels.NumQualityLevels = 0;
	msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;

	m_pDevice->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &msQualityLevels, sizeof(msQualityLevels));
	std::cout << "MultiSample: SampleCount- " << msQualityLevels.SampleCount << ", QulityLevels- " << msQualityLevels.NumQualityLevels << std::endl;

	// Command
	D3D12_COMMAND_QUEUE_DESC QueueDesc = {}; // 注意此处初始化
	QueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	QueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

	m_pDevice->CreateCommandQueue(&QueueDesc, IID_PPV_ARGS(&m_pCommandQueue));
	m_pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_pCommandAllocator));
	m_pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_pCommandAllocator, nullptr, IID_PPV_ARGS(&m_pCommandList));

	m_pCommandList->Close();

	// Create Swap chain
	DXGI_SWAP_CHAIN_DESC scDesc;
	scDesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	scDesc.BufferDesc.Height = m_nClientWindowHeight;
	scDesc.BufferDesc.Width = m_nClientWindowWidth;
	scDesc.BufferDesc.RefreshRate.Numerator = 60;
	scDesc.BufferDesc.RefreshRate.Denominator = 1;
	scDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	scDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_STRETCHED;
	scDesc.SampleDesc.Count = 1;
	scDesc.SampleDesc.Quality = 0;
	scDesc.BufferCount = m_nSwapChainBufferCount;
	scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	scDesc.OutputWindow = m_hWnd;
	scDesc.Windowed = true;
	scDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	scDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	ThrowIfFailed(m_pFactory->CreateSwapChain(m_pCommandQueue, &scDesc, &m_pSwapChian));

	m_pFactory->MakeWindowAssociation(m_hWnd, DXGI_MWA_NO_ALT_ENTER);

	// create descriptors heap
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.NumDescriptors = m_nSwapChainBufferCount;
	rtvHeapDesc.NodeMask = 0;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	m_pDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_pRTVHeap));

	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.NodeMask = 0;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	m_pDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_pDSVHeap));

	OnResize();
}

void WinApp::OnResize()
{
	if (m_pDevice == nullptr || m_pSwapChian == nullptr || m_pCommandAllocator == nullptr)
		return;

	// 在修改其他资源之前刷新命令队列
	FlushCommandQueue();
	m_pCommandList->Reset(m_pCommandAllocator, nullptr);

	// reset swap chian
	for (int i = 0; i < m_nSwapChainBufferCount; ++i)
	{
		if (m_pSwapChainBuffers[i])
		{
			m_pSwapChainBuffers[i]->Release(); m_pSwapChainBuffers[i] = nullptr;
		}
	}

	if (m_pDSVBuffer)
	{
		m_pDSVBuffer->Release(); m_pDSVBuffer = nullptr;
	}

	m_pSwapChian->ResizeBuffers(m_nSwapChainBufferCount,
		m_nClientWindowWidth, m_nClientWindowHeight,
		m_BackBufferFromat,
		DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);

	m_nCurrentSwapChainIndex = 0;

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtHeapHandle(m_pRTVHeap->GetCPUDescriptorHandleForHeapStart());
	for (int i = 0; i < m_nSwapChainBufferCount; ++i)
	{
		m_pSwapChian->GetBuffer(i, IID_PPV_ARGS(&m_pSwapChainBuffers[i]));
		m_pDevice->CreateRenderTargetView(m_pSwapChainBuffers[i], nullptr,rtHeapHandle);
		rtHeapHandle.Offset(1, m_nRTVDescriptorSize);
	}

	// create dsv
	D3D12_RESOURCE_DESC dsvDesc = {};
	dsvDesc.Format = m_DSVFormat;
	dsvDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	dsvDesc.Alignment = 0;
	dsvDesc.Width = m_nClientWindowWidth;
	dsvDesc.Height = m_nClientWindowHeight;
	dsvDesc.DepthOrArraySize = 1;
	dsvDesc.MipLevels = 1;
	dsvDesc.SampleDesc.Count = 1;
	dsvDesc.SampleDesc.Quality = 0;
	dsvDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	dsvDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE dsvClearValue = {};
	dsvClearValue.Format = m_DSVFormat;
	dsvClearValue.DepthStencil.Depth = 1.0f;
	dsvClearValue.DepthStencil.Stencil = 0;

	m_pDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&dsvDesc,
		D3D12_RESOURCE_STATE_COMMON,
		&dsvClearValue,
		IID_PPV_ARGS(&m_pDSVBuffer));

	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHeapHandle(m_pDSVHeap->GetCPUDescriptorHandleForHeapStart());
	m_pDevice->CreateDepthStencilView(m_pDSVBuffer, nullptr, dsvHeapHandle);

	m_pCommandList->ResourceBarrier(
		1, &CD3DX12_RESOURCE_BARRIER::Transition(
			m_pDSVBuffer,
			D3D12_RESOURCE_STATE_COMMON,
			D3D12_RESOURCE_STATE_DEPTH_WRITE));
	
	// Update Viewport 
	// viewport 同一个RenderTarget上不能有多个VP
	m_ScreenViewPort.TopLeftX = 0.0f;
	m_ScreenViewPort.TopLeftY = 0.0f;
	m_ScreenViewPort.Width = static_cast<float>(m_nClientWindowWidth);
	m_ScreenViewPort.Height = static_cast<float>(m_nClientWindowHeight);
	m_ScreenViewPort.MinDepth = 0.0f;
	m_ScreenViewPort.MaxDepth = 1.0f;

	m_ScissorRect = {0, 0, static_cast<LONG>(m_nClientWindowWidth), static_cast<LONG>(m_nClientWindowHeight) };

	// Execute resize command
	m_pCommandList->Close();
	ID3D12CommandList* cmdLists[1] = { m_pCommandList };
	m_pCommandQueue->ExecuteCommandLists(1, cmdLists);

	FlushCommandQueue();
}

void WinApp::Destroy()
{

}

void WinApp::LogAdapters(IDXGIFactory * pFactory)
{
	std::wstring text;

	int i = 0;
	IDXGIAdapter1* pAdapter = nullptr;
	while (m_pFactory->EnumAdapters1(i, &pAdapter) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_ADAPTER_DESC1 desc;
		pAdapter->GetDesc1(&desc);

		text += L"***Adapter: ";
		text += std::wstring(desc.Description);
		text += L"\nDidiced Video Memory: " + IntToWString(desc.DedicatedVideoMemory);
		text += L"\n";

		pAdapter->Release();

		i++;
	}

	std::string newText;
	WStringToString(text, newText);

	std::cout << newText << std::endl;
}

void WinApp::QueryFeatureData(ID3D12Device* pDevice)
{
	D3D12_FEATURE_DATA_D3D12_OPTIONS d3d12Options;

	pDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &d3d12Options, sizeof(d3d12Options));

	switch (d3d12Options.ResourceBindingTier)
	{
	case D3D12_RESOURCE_BINDING_TIER_3:
		std::cout << "GPU support: D3D12_RESOURCE_BINDING_TIER_3 " << std::endl;
		break;
	case D3D12_RESOURCE_BINDING_TIER_2:
		std::cout << "GPU support: D3D12_RESOURCE_BINDING_TIER_2 " << std::endl;
		break;
	case D3D12_RESOURCE_BINDING_TIER_1:
		std::cout << "GPU support: D3D12_RESOURCE_BINDING_TIER_1 " << std::endl;
		break;
	default:
		break;
	}
}

ID3D12Resource * WinApp::GetCurrentBackBuffer()
{
	return m_pSwapChainBuffers[m_nCurrentSwapChainIndex];
}

D3D12_CPU_DESCRIPTOR_HANDLE WinApp::GetCurrentBackBufferView()
{
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(
		m_pRTVHeap->GetCPUDescriptorHandleForHeapStart(), 
		m_nCurrentSwapChainIndex, 
		m_nRTVDescriptorSize);
}

D3D12_CPU_DESCRIPTOR_HANDLE WinApp::GetDepthStencilView()
{
	return m_pDSVHeap->GetCPUDescriptorHandleForHeapStart();
}

void WinApp::Init()
{
	m_bTearingSupported = CheckTearingSupported();

	InitD3D12();

	GetWindowRect(m_hWnd, &m_WindowRect);
}

void WinApp::Update(double dt)
{

}

void WinApp::Draw()
{
	m_pCommandAllocator->Reset();
	m_pCommandList->Reset(m_pCommandAllocator, nullptr);
	
	m_pCommandList->ResourceBarrier(
		1, &CD3DX12_RESOURCE_BARRIER::Transition(
			GetCurrentBackBuffer(),
			D3D12_RESOURCE_STATE_PRESENT,
			D3D12_RESOURCE_STATE_RENDER_TARGET));
	
	m_pCommandList->RSSetViewports(1, &m_ScreenViewPort);
	
	m_pCommandList->RSSetScissorRects(1, &m_ScissorRect);
	
	float clearColors[4] = {0.5f, 0.5f, 0.5f, 1.0f};
	m_pCommandList->ClearRenderTargetView(
		GetCurrentBackBufferView(),
		clearColors, 0, nullptr);
	
	m_pCommandList->ClearDepthStencilView(
		GetDepthStencilView(),
		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
		1.0, 0, 0, nullptr);

	m_pCommandList->OMSetRenderTargets(1, &GetCurrentBackBufferView(), true, &GetDepthStencilView());

	m_pCommandList->ResourceBarrier(
		1, &CD3DX12_RESOURCE_BARRIER::Transition(
			GetCurrentBackBuffer(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PRESENT));

	m_pCommandList->Close();

	ID3D12CommandList* cmdLists[1] = {m_pCommandList};

	m_pCommandQueue->ExecuteCommandLists(1, cmdLists);

	m_pSwapChian->Present(0, 0);

	m_nCurrentSwapChainIndex = (m_nCurrentSwapChainIndex + 1) % m_nSwapChainBufferCount;

	FlushCommandQueue();
}

LRESULT WinApp::WndMsgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);
		// TODO: 在此处添加使用 hdc 的任何绘图代码...
		EndPaint(hWnd, &ps);
	}
	break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_ACTIVATE:
		if (LOWORD(wParam) == WA_INACTIVE)
		{
			std::cout << "InActive" << std::endl;
		}
		else
		{
			std::cout << "Active" << std::endl;
		}
		break;
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
		break;
	}
	case WM_ENTERSIZEMOVE:
		// 用户开始拖动resize bar
		break;
	case WM_EXITSIZEMOVE:
		OnResize();
		// 用户停止拖动
		break;
	case WM_SYSKEYDOWN:
	case WM_KEYDOWN:
	{
		bool alt = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
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
		}
		break;
	}
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

void WinApp::EnableDebugLayer()
{
#if defined(DEBUG) || defined(_DEBUG)
	// 启动debug
	ID3D12Debug* pDebugController = nullptr;
	D3D12GetDebugInterface(IID_PPV_ARGS(&pDebugController));
	pDebugController->EnableDebugLayer();
#endif
}

void WinApp::FlushCommandQueue()
{
	m_CurrentFence++;

	m_pCommandQueue->Signal(m_pFence, m_CurrentFence);

	if (m_pFence->GetCompletedValue() < m_CurrentFence)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);

		m_pFence->SetEventOnCompletion(m_CurrentFence, eventHandle);

		WaitForSingleObject(eventHandle, INFINITE);

		CloseHandle(eventHandle);
	}
}
