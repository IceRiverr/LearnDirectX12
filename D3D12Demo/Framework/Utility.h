#pragma once
#include <string>
#include <sstream>
#include "DirectXMath.h"

using namespace DirectX;

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

template<typename T>
T StringToNumber(const std::string& t)
{
	std::istringstream ss(t);
	T v;
	ss >> v;
	return v;
}

// https://stackoverflow.com/questions/8541301/best-alternative-to-a-typedef-for-a-function-template
//using String2Float = decltype(StringToNumber<float>(0));

namespace MathUtility
{
	template<typename T>
	T Clamp(T v, T min, T max)
	{
		T result = v;
		if (v < min)
			result = min;
		else if (v > max)
			result = max;
		return result;
	}

	inline float ToRadian(float degree)
	{
		static float delta = XM_PI / 180.0f;
		return degree * delta;
	}
}
