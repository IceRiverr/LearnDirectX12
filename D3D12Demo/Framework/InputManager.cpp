#include "InputManager.h"

KeyInfo GetKeyInfo(WPARAM wParam, LPARAM lParam)
{
	KeyInfo info;
	// wParam : virtual - key code of the nonsystem key
	if (wParam >= 'A' && wParam <= 'Z')
	{
		info.bIsLeter = true;
		info.key = (char)wParam;
	}

	// lParam
	// 0-15: repeat count for the current message
	info.repeatCount = lParam & 0xFFFF;

	// 16-23: scan code
	info.scanCode = (lParam >> 16) & 0xFF;

	// 30 : The previous key state
	info.bLastDown = (lParam >> 30) & 0x01;

	return info;
}

bool CInputManager::IsKeyJustDown(char vk)
{
	if (m_KeyInfos.count(vk))
	{
		if (m_KeyInfos[vk].bIsLeter && m_KeyInfos[vk].state == KeyState::Down && !m_KeyInfos[vk].bLastDown)
		{
			return true;
		}
		return false;
	}
	return false;
}

bool CInputManager::IsKeyHold(char vk)
{
	if (m_KeyInfos.count(vk))
	{
		if (m_KeyInfos[vk].bIsLeter && m_KeyInfos[vk].state == KeyState::Down && m_KeyInfos[vk].bLastDown)
		{
			return true;
		}
		return false;
	}
	return false;
}

bool CInputManager::IsKeyHoldOrDown(char vk)
{
	if (m_KeyInfos.count(vk))
	{
		if (m_KeyInfos[vk].bIsLeter && m_KeyInfos[vk].state == KeyState::Down)
		{
			return true;
		}
		return false;
	}
	return false;
}

bool CInputManager::IsKeyUp(char vk)
{
	if (m_KeyInfos.count(vk))
	{
		if (m_KeyInfos[vk].bIsLeter && m_KeyInfos[vk].state == KeyState::Up)
		{
			return true;
		}
		return false;
	}
	return false;
}

bool CInputManager::GetMouseDelta(XMINT2 & delta) const
{
	if (m_nDelteMouseX != 0 || m_nDeltaMouseY != 0)
	{
		delta.x = m_nDelteMouseX;
		delta.y = m_nDeltaMouseY;
		return true;
	}
	return false;
}

XMINT2 CInputManager::GetMousePos() const
{
	return XMINT2(m_nMouseX, m_nMouseY);
}

void CInputManager::ResetInputInfos()
{
	for (auto it = m_KeyInfos.begin(); it != m_KeyInfos.end(); ++it)
	{
		ClearKeyInfo(it->second);
	}

	m_nDelteMouseX = 0;
	m_nDeltaMouseY = 0;
	m_nMouseZDelta = 0;
}

void ClearKeyInfo(KeyInfo & keyInfo)
{
	keyInfo.state = KeyState::Free;
	keyInfo.bIsLeter = false;
	keyInfo.repeatCount = 0;
	keyInfo.key = 'a';
	keyInfo.repeatCount = 0;
	keyInfo.scanCode = 0;
	keyInfo.bLastDown = false;
}
