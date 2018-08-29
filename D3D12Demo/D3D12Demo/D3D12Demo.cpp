//D3D12Demo.cpp: 定义应用程序的入口点。
//

#include "stdafx.h"

#include <iostream>
#include <stdlib.h>
#include <io.h>
#include <fcntl.h>

#include "D3D12Demo.h"
#include "ultility.h"

#include "DemoApp.h"
#include "DrawBoxApp.h"
#include "DrawBoxArrayApp.h"

#define MAX_LOADSTRING 100

// 全局变量: 
HINSTANCE hInst;                                // 当前实例
WCHAR szTitle[MAX_LOADSTRING];                  // 标题栏文本
WCHAR szWindowClass[MAX_LOADSTRING];            // 主窗口类名

//DemoApp app;
//DrawBoxApp app;
DrawBoxArrayApp app;

// 此代码模块中包含的函数的前向声明: 
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
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
	
    // 初始化全局字符串
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_D3D12DEMO, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // 执行应用程序初始化: 
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_D3D12DEMO));

	app.InitD3D12();
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
        if(PeekMessage(&msg, app.m_hWindow, 0, 0, PM_REMOVE))
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
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_D3D12DEMO));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= nullptr;//MAKEINTRESOURCEW(IDC_D3D12DEMO);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}


BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // 将实例句柄存储在全局变量中

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, 1280, 720, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   app.m_hWindow = hWnd;

   return TRUE;
}


//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	return app.WndMsgProc(hWnd, message, wParam, lParam);
}
