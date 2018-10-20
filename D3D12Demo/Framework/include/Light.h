#pragma once

#include "stdafx.h"

struct LightInfoShaderStruct
{
	XMFLOAT3 LightColor;
	float Intensity;
	XMFLOAT4 LightDirection;
	XMFLOAT4 LightPosition;

	float RefDist;
	float MaxRadius;
	float CosMinAngle;
	float CosMaxAngle;
};

class CDirectionalLight
{
public:
	LightInfoShaderStruct CreateShaderBlock() const;

public:
	XMFLOAT3 m_Color;
	float m_fIntensity;
	XMVECTOR m_vDirection;
};

class CPointLight
{
public:
	LightInfoShaderStruct CreateShaderBlock() const;

public:
	XMFLOAT3 m_Color;
	float m_fIntensity;
	XMVECTOR m_vPosition;
	float m_fRefDist;
	float m_fMaxRadius;
};

class CSpotLight
{
public:
	LightInfoShaderStruct CreateShaderBlock() const;

public:
	XMFLOAT3 m_Color;
	float m_fIntensity;
	XMVECTOR m_vPosition;
	XMVECTOR m_vDirection;
	float m_fRefDist;
	float m_fMaxRadius;
	float m_fMinAngle; // 最小范围
	float m_fMaxAngle; // 最大范围
};
