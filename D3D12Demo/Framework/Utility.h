#pragma once
#include <string>
#include <sstream>

// https://www.cnblogs.com/02xiaoma/archive/2012/07/18/2597576.html
BOOL WStringToString(const std::wstring &wstr, std::string &str);
BOOL StringToWString(const std::string &str, std::wstring &wstr);

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

#define PI		3.14159265f
#define PI_2	6.28318531f

template<typename T>
T StringToNumber(std::string t)
{
	std::istringstream ss(t);
	T v;
	ss >> v;
	return v;
}

// https://stackoverflow.com/questions/8541301/best-alternative-to-a-typedef-for-a-function-template
//using String2Float = decltype(StringToNumber<float>(0));
