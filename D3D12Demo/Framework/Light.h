#pragma once

#include "stdafx.h"

struct LightInfo
{
	XMFLOAT4 LightColor;
	XMFLOAT4 LightDirection;
	XMFLOAT4 LightPosition;

	float RefDist;
	float MaxRadius;
	float MinAngle;
	float MaxAngle;
};

class CDirectionalLight
{
public:
	XMFLOAT4 m_Color;
	XMVECTOR m_vDirection;
};

class CPointLight
{
public:
	XMFLOAT4 m_Color;
	XMVECTOR m_vPosition;
	float m_fRefDist;
	float m_fMaxRadius;
};

class CSpotLight
{
public:
	XMFLOAT4 m_Color;
	XMVECTOR m_vPosition;
	XMVECTOR m_vDirection;
	float m_fRefDist;
	float m_fMaxRadius;
	float m_fMinAngle; // 最小范围
	float m_fMaxAngle; // 最大范围
};
