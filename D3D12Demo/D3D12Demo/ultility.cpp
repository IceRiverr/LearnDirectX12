#include "stdafx.h"
#include "ultility.h"
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

std::wstring IntToWString(SIZE_T v)
{
	std::wostringstream stream;
	stream << v;
	return stream.str();
}
