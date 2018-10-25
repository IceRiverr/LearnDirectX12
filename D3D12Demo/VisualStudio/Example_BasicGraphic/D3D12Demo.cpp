//D3D12Demo.cpp: ����Ӧ�ó������ڵ㡣
//



#include <iostream>
#include <stdlib.h>
#include <io.h>
#include <fcntl.h>

#include "D3D12Demo.h"
#include "ImportObj.h"
#include <map>


#include "WinApp.h"
#include "DrawBoxApp.h"
#include "DrawBoxArrayApp.h"
#include "TestInputLayout.h"
#include "LightSourceApp.h"
#include "Material_BRDF.h"
#include "SkyBoxApp.h"

#define MAX_LOADSTRING 100

// ȫ�ֱ���: 
WCHAR szTitle[MAX_LOADSTRING] = L"BRDF Demo";                  // �������ı�
WCHAR szWindowClass[MAX_LOADSTRING] = L"DirextX12 Demo";            // ����������

//DrawBoxApp app;
//DrawBoxArrayApp app;
//TestInputLayoutApp app;
//CLightSourceApp app;
CMaterialBRDFApp app;
//CSkyBoxApp app;

// �˴���ģ���а����ĺ�����ǰ������: 
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

    // TODO: �ڴ˷��ô��롣
	app.InitConsoleWindow();
	app.ParseCommandLineArguments();
	
    // ��ʼ��ȫ���ַ���
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
  //  LoadStringW(hInstance, IDC_D3D12DEMO, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // ִ��Ӧ�ó����ʼ��: 
    if (!InitInstance (hInstance, nCmdShow, app))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_D3D12DEMO));
	
	app.Init();

	__int64 _countsPerSecend = 0;
	__int64 _oldCounts = 0;
	__int64 _currentCounts = 0;
	QueryPerformanceFrequency((LARGE_INTEGER*)&_countsPerSecend);
	QueryPerformanceCounter((LARGE_INTEGER*)&_currentCounts);
	double secendPerCount = 1.0 / _countsPerSecend;
	
	//--------TEST START--------
	




	//--------TEST END---------
	
	MSG msg = {};
    // ����Ϣѭ��: 
    while (msg.message != WM_QUIT)
    {
        if(PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
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
	app.Destroy();
    return 0;
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
	App.m_hInstance = hInstance;// ��ʵ������洢��ȫ�ֱ�����

	// ����ʾ��������
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
