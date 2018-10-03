#include "Light.h"
#include "Utility.h"

LightInfoShaderStruct CDirectionalLight::CreateShaderBlock() const
{
	LightInfoShaderStruct lightInfo = {};
	XMStoreFloat4(&lightInfo.LightDirection, m_vDirection);
	lightInfo.LightColor = m_Color;
	lightInfo.Intensity = m_fIntensity;

	return lightInfo;
}

LightInfoShaderStruct CPointLight::CreateShaderBlock() const
{
	LightInfoShaderStruct lightInfo = {};
	XMStoreFloat4(&lightInfo.LightPosition, m_vPosition);
	lightInfo.RefDist = m_fRefDist;;
	lightInfo.MaxRadius = m_fMaxRadius;
	lightInfo.LightColor = m_Color;
	lightInfo.Intensity = m_fIntensity;

	return lightInfo;
}

LightInfoShaderStruct CSpotLight::CreateShaderBlock() const
{
	LightInfoShaderStruct lightInfo = {};
	XMStoreFloat4(&lightInfo.LightDirection, m_vDirection);
	XMStoreFloat4(&lightInfo.LightPosition, m_vPosition);
	lightInfo.LightColor = m_Color;
	lightInfo.Intensity = m_fIntensity;

	lightInfo.RefDist = m_fRefDist;
	lightInfo.MaxRadius = m_fMaxRadius;
	lightInfo.CosMinAngle = std::cosf(MathUtility::ToRadian(m_fMinAngle));
	lightInfo.CosMaxAngle = std::cosf(MathUtility::ToRadian(m_fMaxAngle));

	return lightInfo;
}
