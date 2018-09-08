#pragma once
#include "stdafx.h"
#include <unordered_map>

enum KeyState
{
	Free = 0,
	Down,
	Up
};

struct KeyInfo
{
	KeyState state;
	bool bIsLeter;
	char key;
	int repeatCount;
	int scanCode;
	int bLastDown;
};

KeyInfo GetKeyInfo(WPARAM wParam, LPARAM lParam);
void ClearKeyInfo(KeyInfo& keyInfo);

class CInputManager
{
public:
	bool IsKeyJustDown(char virtualKey);
	bool IsKeyHold(char virtualKey);
	bool IsKeyHoldOrDown(char virtualKey);
	bool IsKeyUp(char virtualKey);
	bool GetMouseDelta(XMINT2& delta);
	XMINT2 GetMousePos();
	void ResetInputInfos();

public:
	std::unordered_map<char, KeyInfo> m_KeyInfos;
	int m_nMouseX;
	int m_nMouseY;
	int m_nLastMouseX;
	int m_nLastMouseY;
	int m_nDelteMouseX;
	int m_nDeltaMouseY;
};
