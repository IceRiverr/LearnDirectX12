#pragma once
#include "DemoApp.h"
#include <DirectXMath.h>

class DrawBoxApp :
	public DemoApp
{
public:
	struct Vertex
	{
		XMFLOAT3 position;
		XMFLOAT4 color;
	};

	struct ObjectConstants
	{
		XMFLOAT4X4 WorldViewProj;

	};

	DrawBoxApp();
	~DrawBoxApp();

	virtual void Init();
	virtual void Update(double deltaTime);
	virtual void Draw();

private:
	ID3D12DescriptorHeap * m_pCBVHeap;
	ID3D12Resource* m_pUploadeConstBuffer;
	ID3D12RootSignature* m_pRootSignature;
};

