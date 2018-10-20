#include "stdafx.h"
#include "Utility.h"
#include <iostream>
#include <sstream>


BOOL WStringToString(const std::wstring &wstr, std::string &str)
{
	int nLen = (int)wstr.length();
	str.resize(nLen, ' ');

	int nResult = WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)wstr.c_str(), nLen, (LPSTR)str.c_str(), nLen, NULL, NULL);

	if (nResult == 0)
	{
		return FALSE;
	}

	return TRUE;
}

BOOL StringToWString(const std::string & str, std::wstring & wstr)
{
	int nLen = (int)str.length();
	wstr.resize(nLen, L' ');

	int nResult = MultiByteToWideChar(CP_ACP, 0, (LPCSTR)str.c_str(), nLen, (LPWSTR)wstr.c_str(), nLen);

	if (nResult == 0)
	{
		return FALSE;
	}

	return TRUE;
}

std::string WStringToString(const std::wstring & wstr)
{
	std::string str;
	if (WStringToString(wstr, str))
		return str;
	else
		return std::string();
}

std::wstring StringToWString(const std::string & str)
{
	std::wstring wStr;
	if (StringToWString(str, wStr))
		return wStr;
	else
		return std::wstring();
}

std::wstring IntToWString(SIZE_T v)
{
	std::wostringstream ss;
	ss << v;
	return ss.str();
}
