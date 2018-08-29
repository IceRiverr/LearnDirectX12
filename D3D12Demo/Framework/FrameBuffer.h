
#pragma once
#include "stdafx.h"

class CFrameBuffer
{
public:
	CFrameBuffer();
	~CFrameBuffer();

	void CreateBuffer(ID3D12Device* pDevice);

public:
	struct _Buffer
	{
		XMFLOAT4X4 g_mView;
		XMFLOAT4X4 g_mInvView;
		XMFLOAT4X4 g_mProj;
		XMFLOAT4X4 g_mInvProj;
		XMFLOAT4X4 g_mViewProj;
		XMFLOAT4X4 g_mInvViewProj;
		XMFLOAT3 g_vEyePosition;
		float PAD_1;
		XMFLOAT2 g_RenderTargetSize;
		XMFLOAT2 g_InvRenderTargetSize;
		float g_fNearZ;
		float g_fFarZ;
		float g_fTotalTime;
		float g_fDeltaTime;
	};
	
	_Buffer m_FrameBufferData;

	UINT m_nConstantBufferSizeAligned;
	ID3D12Resource* m_pUploadeConstBuffer;
	UINT8* m_pCbvDataBegin;

	UINT m_nDescriptorIndex;
};
