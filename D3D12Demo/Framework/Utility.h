#pragma once
#include <string>

BOOL WStringToString(const std::wstring &wstr, std::string &str);

std::wstring IntToWString(SIZE_T v);

// From DXSampleHelper.h 
// Source: https://github.com/Microsoft/DirectX-Graphics-Samples
inline void ThrowIfFailed(HRESULT hr)
{
	if (FAILED(hr))
	{
		throw std::exception();
	}
}
