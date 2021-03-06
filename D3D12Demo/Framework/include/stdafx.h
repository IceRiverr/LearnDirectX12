// stdafx.h : 标准系统包含文件的包含文件，
// 或是经常使用但不常更改的
// 特定于项目的包含文件
//

#pragma once

//#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // 从 Windows 头中排除极少使用的资料
// Windows 头文件: 
#include <windows.h>

// C 运行时头文件
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

// Directx12
#include <d3d12.h>
#include "d3dx12.h"
#include <dxgi1_6.h>
#include <DirectXMath.h>
#include <d3dcompiler.h>
#include <iostream>
#include <string>
using namespace DirectX;


#include <vector>
#include <algorithm>

// 只是用algorithm中的min max方法
#if defined(min)
#undef min
#endif

#if defined(max)
#undef max
#endif

// TODO: 在此处引用程序需要的其他头文件

