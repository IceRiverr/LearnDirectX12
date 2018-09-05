#pragma once
#include "stdafx.h"

namespace Graphics
{
	ID3D12Resource* CreateDefaultBuffer(ID3D12Device* pDevice, ID3D12GraphicsCommandList* cmdList, const void* initData, UINT64 byteSize, ID3D12Resource** ppUploadBuffer);
}