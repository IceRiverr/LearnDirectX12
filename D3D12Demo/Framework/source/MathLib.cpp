#include "MathLib.h"

namespace MathLib
{

XMFLOAT3 XMFloat3Subtract(const XMFLOAT3& v1, const XMFLOAT3& v2)
{
	XMVECTOR XV1 = XMLoadFloat3(&v1);
	XMVECTOR XV2 = XMLoadFloat3(&v2);

	XMVECTOR sub = XMVectorSubtract(XV1, XV2);

	XMFLOAT3 result;
	XMStoreFloat3(&result, sub);
	return result;
}

XMFLOAT2 XMFloat2Subtract(const XMFLOAT2 & v1, const XMFLOAT2 & v2)
{
	XMVECTOR XV1 = XMLoadFloat2(&v1);
	XMVECTOR XV2 = XMLoadFloat2(&v2);

	XMVECTOR sub = XMVectorSubtract(XV1, XV2);

	XMFLOAT2 result;
	XMStoreFloat2(&result, sub);
	return result;
}

XMFLOAT3 XMFloat3Add(const XMFLOAT3 & v1, const XMFLOAT3 & v2)
{
	XMVECTOR XV1 = XMLoadFloat3(&v1);
	XMVECTOR XV2 = XMLoadFloat3(&v2);

	XMVECTOR sub = XMVectorAdd(XV1, XV2);

	XMFLOAT3 result;
	XMStoreFloat3(&result, sub);
	return result;
}

}


