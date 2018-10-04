#include "Camera.h"
#include "InputManager.h"
#include "Utility.h"

CBaseCamera::CBaseCamera()
{
	m_vEyePositon = XMFLOAT3(0.0f, 10.0f, -10.0f);
	m_vTarget = XMFLOAT3(0.0f, 0.0f, 0.0f);

	m_fFovAngleY = 90.0f;
	m_fAspectRatio = 16.0f/9.0f;
	m_fNearZ = 1.0f;
	m_fFarZ = 1000.0f;

	m_bDirty = true;
}

void CBaseCamera::Init(float fovAngle, float aspectRatio, float nearZ, float farZ)
{
	m_fFovAngleY = fovAngle;
	m_fAspectRatio = aspectRatio;
	m_fNearZ = nearZ;
	m_fFarZ = farZ;

	m_bDirty = true;
}

void CBaseCamera::OnUpdate(double dt, CInputManager& InputMgr)
{
	
}

void CBaseCamera::Update(double dt, CInputManager& InputMgr)
{
	OnUpdate(dt, InputMgr);

	if (m_bDirty)
	{
		m_bDirty = false;
		
		m_mInvViewMatrix = XMMatrixInverse(&XMMatrixDeterminant(m_mViewMatrix), m_mViewMatrix);
		m_mInvProjMatrix = XMMatrixInverse(&XMMatrixDeterminant(m_mProjMatrix), m_mProjMatrix);
		m_mInvViewProjMatrix = XMMatrixInverse(&XMMatrixDeterminant(m_mViewProjMatrix), m_mViewProjMatrix);
		
		UpdateCameraInfo();
	}
}

void CBaseCamera::SetAspectRatio(float ratio)
{
	m_fAspectRatio = ratio;
}

void CBaseCamera::UpdateCameraInfo()
{
	m_CameraInfo.vEyePositon = this->m_vEyePositon;
	m_CameraInfo.fNearZ = m_fNearZ;
	m_CameraInfo.fFarZ = m_fFarZ;
	XMStoreFloat4x4(&m_CameraInfo.mViewMatrix, XMMatrixTranspose(m_mViewMatrix));
	XMStoreFloat4x4(&m_CameraInfo.mProjMatrix, XMMatrixTranspose(m_mProjMatrix));
	XMStoreFloat4x4(&m_CameraInfo.mViewProjMatrix, XMMatrixTranspose(m_mViewProjMatrix));
	XMStoreFloat4x4(&m_CameraInfo.mInvViewMatrix, XMMatrixTranspose(m_mInvViewMatrix));
	XMStoreFloat4x4(&m_CameraInfo.mInvProjMatrix, XMMatrixTranspose(m_mInvProjMatrix));
	XMStoreFloat4x4(&m_CameraInfo.mInvViewProjMatrix, XMMatrixTranspose(m_mInvViewProjMatrix));
}

void CAutoRotateCamera::OnUpdate(double dt, CInputManager & InputMgr)
{
	static double dTotalTime = 0.0f;
	dTotalTime += dt;

	m_vEyePositon.x = 16.0f * (float)std::cos(dTotalTime * XM_PI * 0.8f);
	m_vEyePositon.y = 5.0f;
	m_vEyePositon.z = 16.0f * (float)std::sin(dTotalTime * XM_PI * 0.8f);

	m_bDirty = true;
}

CRotateCamera::CRotateCamera()
{
	m_fRotateRadius = 20.0f;
	m_fTheta = 0.0f;
	m_fPhi = 0.0f;
	m_fMouseIntensity = 0.5f;
	m_fSmooth = 0.01f;
}

void CRotateCamera::OnUpdate(double dt, CInputManager & InputMgr)
{
	XMINT2 moveDelta;
	if (InputMgr.m_nMouseZDelta != 0)
	{
		m_fRotateRadius += -m_fSmooth * InputMgr.m_nMouseZDelta;
		m_bDirty = true;
	}

	if (InputMgr.GetMouseDelta(moveDelta))
	{
		m_fTheta -= moveDelta.x * m_fMouseIntensity;
		m_fPhi += moveDelta.y * m_fMouseIntensity;
		
		m_fPhi = MathUtility::Clamp<float>(m_fPhi, 5.0f, 175.0f);
		m_bDirty = true;
	}

	if (m_bDirty)
	{
		float thetaRadian = m_fTheta / 180.0f * XM_PI;
		float phiRadian = m_fPhi / 180.0f * XM_PI;

		m_vEyePositon.x = m_fRotateRadius * std::sinf(phiRadian) * std::cosf(thetaRadian);
		m_vEyePositon.y = m_fRotateRadius * std::cosf(phiRadian);
		m_vEyePositon.z = m_fRotateRadius * std::sinf(phiRadian) * std::sinf(thetaRadian);

		XMVECTOR eyePos = XMVectorSet(m_vEyePositon.x, m_vEyePositon.y, m_vEyePositon.z, 1.0f);
		XMVECTOR target = XMVectorZero();
		XMVECTOR upDir = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

		m_mViewMatrix = XMMatrixLookAtLH(eyePos, target, upDir);
		m_mProjMatrix = XMMatrixPerspectiveFovLH(m_fFovAngleY / 180.0f * XM_PI, m_fAspectRatio, m_fNearZ, m_fFarZ);
		m_mViewProjMatrix = XMMatrixMultiply(m_mViewMatrix, m_mProjMatrix);
	}
}

CFPSCamera::CFPSCamera()
{
	m_fRotateRadius = 5.0f;
	m_fTheta = 0.0f;
	m_fPhi = 0.0f;
	m_fMouseIntensity = 0.5f;
	m_fSmooth = 0.01f;
}

void CFPSCamera::OnUpdate(double dt, CInputManager & InputMgr)
{
	XMINT2 moveDelta;
	if (InputMgr.m_nMouseZDelta != 0)
	{
		m_fRotateRadius += -m_fSmooth * InputMgr.m_nMouseZDelta;
		m_fRotateRadius = m_fRotateRadius < 0.1f ? 0.1f : m_fRotateRadius;

		m_bDirty = true;
	}

	if (InputMgr.GetMouseDelta(moveDelta))
	{
		m_fTheta -= moveDelta.x * m_fMouseIntensity;
		m_fPhi += moveDelta.y * m_fMouseIntensity;

		m_fPhi = MathUtility::Clamp<float>(m_fPhi, 5.0f, 175.0f);
		m_bDirty = true;
	}

	float fMoveDelta = 0.0f;
	if (InputMgr.IsKeyHoldOrDown('W'))
	{
		fMoveDelta += 0.1f;
		m_bDirty = true;
	}
	if (InputMgr.IsKeyHoldOrDown('S'))
	{
		fMoveDelta += -0.1f;
		m_bDirty = true;
	}

	float fRightMoveDelta = 0.0f;
	if (InputMgr.IsKeyHoldOrDown('D'))
	{
		fRightMoveDelta += 0.1f;
		m_bDirty = true;
	}
	if (InputMgr.IsKeyHoldOrDown('A'))
	{
		fRightMoveDelta += -0.1f;
		m_bDirty = true;
	}

	if (m_bDirty)
	{
		float thetaRadian = m_fTheta / 180.0f * XM_PI;
		float phiRadian = m_fPhi / 180.0f * XM_PI;

		XMFLOAT3 RotatePosition;
		RotatePosition.x = m_fRotateRadius * std::sinf(phiRadian) * std::cosf(thetaRadian);
		RotatePosition.y = m_fRotateRadius * std::cosf(phiRadian);
		RotatePosition.z = m_fRotateRadius * std::sinf(phiRadian) * std::sinf(thetaRadian);

		XMVECTOR EyeRotate = XMLoadFloat3(&RotatePosition);
		XMVECTOR target = XMLoadFloat3(&m_vTarget);
		XMVECTOR upDir = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

		XMVECTOR eyePos = XMVectorAdd(EyeRotate, target);

		XMVECTOR dir = XMVectorSubtract(target, eyePos);	dir = XMVector3Normalize(dir);
		XMVECTOR Right = XMVector3Cross(upDir, dir);		Right = XMVector3Normalize(Right);

		target = XMVectorAdd(target, dir * fMoveDelta);
		target = XMVectorAdd(target, Right * fRightMoveDelta);
		
		XMVECTOR NewEyePos = XMVectorAdd(EyeRotate, target);

		XMStoreFloat3(&m_vTarget, target);
		XMStoreFloat3(&m_vEyePositon, NewEyePos);

		m_mViewMatrix = XMMatrixLookAtLH(NewEyePos, target, upDir);
		m_mProjMatrix = XMMatrixPerspectiveFovLH(m_fFovAngleY / 180.0f * XM_PI, m_fAspectRatio, m_fNearZ, m_fFarZ);
		m_mViewProjMatrix = XMMatrixMultiply(m_mViewMatrix, m_mProjMatrix);
	}
}
