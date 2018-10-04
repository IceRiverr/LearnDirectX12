#pragma once
#include "stdafx.h"

struct CameraInfo
{
	XMFLOAT3 vEyePositon;

	XMFLOAT4X4 mViewMatrix;
	XMFLOAT4X4 mProjMatrix;
	XMFLOAT4X4 mViewProjMatrix;
	XMFLOAT4X4 mInvViewMatrix;
	XMFLOAT4X4 mInvProjMatrix;
	XMFLOAT4X4 mInvViewProjMatrix;

	float fNearZ;
	float fFarZ;
};

class CInputManager;
class CBaseCamera
{
public:
	CBaseCamera();
	virtual ~CBaseCamera() {};
	
	virtual void Init(float fovAngle, float aspectRatio, float nearZ, float farZ);
	virtual void OnUpdate(double dt, const CInputManager& InputMgr);
	virtual void Update(double dt, const CInputManager& InputMgr);

	void SetAspectRatio(float ratio);

private:
	void UpdateCameraInfo();

public:
	CameraInfo m_CameraInfo;

protected:
	bool m_bDirty;

	float m_fFovAngleY;
	float m_fAspectRatio;
	float m_fNearZ;
	float m_fFarZ;

	XMFLOAT3 m_vEyePositon;
	XMFLOAT3 m_vTarget;

	XMMATRIX m_mViewMatrix;
	XMMATRIX m_mProjMatrix;
	XMMATRIX m_mViewProjMatrix;
	XMMATRIX m_mInvViewMatrix;
	XMMATRIX m_mInvProjMatrix;
	XMMATRIX m_mInvViewProjMatrix;
};

class CAutoRotateCamera :public CBaseCamera
{
public:
	virtual void OnUpdate(double dt, const CInputManager& InputMgr);
};

class CRotateCamera :public CBaseCamera
{
public:
	CRotateCamera();
	virtual void OnUpdate(double dt, const CInputManager& InputMgr);

public:
	float m_fRotateRadius;
	float m_fMouseIntensity;

	float m_fTheta;
	float m_fPhi;
	float m_fSmooth;
};
