#pragma once

#include "stdafx.h"

class CGraphicPipeline
{
public:
	std::string m_sName;

	ID3D12PipelineState * m_pPSO;
	ID3D12RootSignature* m_pRootSignature;

	std::vector<D3D12_INPUT_ELEMENT_DESC> m_InputLayout;
};
