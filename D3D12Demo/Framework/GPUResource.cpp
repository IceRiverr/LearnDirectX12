#include "GPUResource.h"

void CCommonBuffer::Map()
{
	m_pCommonBuffer->Map(0, nullptr, (void**)(&m_pData));
}

void CCommonBuffer::UnMap()
{
	m_pCommonBuffer->Unmap(0, nullptr);
}
