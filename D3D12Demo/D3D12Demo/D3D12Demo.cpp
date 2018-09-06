//D3D12Demo.cpp: 定义应用程序的入口点。
//

#include <iostream>
#include <stdlib.h>
#include <io.h>
#include <fcntl.h>

#include "D3D12Demo.h"

#include "WinApp.h"
#include "DrawBoxApp.h"
#include "DrawBoxArrayApp.h"
#include "TestInputLayout.h"

#define MAX_LOADSTRING 100

// 全局变量: 
WCHAR szTitle[MAX_LOADSTRING];                  // 标题栏文本
WCHAR szWindowClass[MAX_LOADSTRING];            // 主窗口类名

//DemoApp app;
//DrawBoxApp app;
DrawBoxArrayApp app;
//TestInputLayoutApp app;

// 此代码模块中包含的函数的前向声明: 
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int, WinApp& App);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: 在此放置代码。
	app.InitConsoleWindow();
	app.ParseCommandLineArguments();
	
    // 初始化全局字符串
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_D3D12DEMO, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // 执行应用程序初始化: 
    if (!InitInstance (hInstance, nCmdShow, app))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_D3D12DEMO));
	
	app.Init();

	__int64 _countsPerSecend;
	__int64 _oldCounts;
	__int64 _currentCounts;
	QueryPerformanceFrequency((LARGE_INTEGER*)&_countsPerSecend);
	QueryPerformanceCounter((LARGE_INTEGER*)&_currentCounts);
	double secendPerCount = 1.0 / _countsPerSecend;

	MSG msg = {};

    // 主消息循环: 
    while (msg.message != WM_QUIT)
    {
        if(PeekMessage(&msg, app.m_hWnd, 0, 0, PM_REMOVE))
		{
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
		else
		{
			_oldCounts = _currentCounts;
			QueryPerformanceCounter((LARGE_INTEGER*)&_currentCounts);
			double deltaTime = (_currentCounts - _oldCounts) * secendPerCount;
			double fps = _countsPerSecend * 1.0f / (_currentCounts - _oldCounts);
			//std::cout << "Delta Time: " <<  deltaTime << "Fps: " << fps << std::endl;

			app.Update(deltaTime);
			app.Draw();
		}
    }

    return (int) msg.wParam;
}


ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, nullptr);
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= nullptr;
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, nullptr);

    return RegisterClassExW(&wcex);
}


BOOL InitInstance(HINSTANCE hInstance, int nCmdShow, WinApp& App)
{
	App.m_hInstance = hInstance;// 将实例句柄存储在全局变量中

	// 主显示器的像素
	int screenWidth = GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = GetSystemMetrics(SM_CYSCREEN);

	RECT windowRect = { 0, 0, (LONG)App.m_nClientWindowWidth, (LONG)App.m_nClientWindowHeight };
	AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, false);

	int windowWidth = windowRect.right - windowRect.left;
	int windowHeight = windowRect.bottom - windowRect.top;

	int windowX = std::max<int>(0, (screenWidth - windowWidth) / 2);
	int windowY = std::max<int>(0, (screenHeight - windowHeight) / 2);

	HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
		windowX, windowY, windowWidth, windowHeight, nullptr, nullptr, hInstance, nullptr);

	if (!hWnd)
	{
		return FALSE;
	}

	App.m_hWnd = hWnd;

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return TRUE;
}


//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	return app.WndMsgProc(hWnd, message, wParam, lParam);
}
