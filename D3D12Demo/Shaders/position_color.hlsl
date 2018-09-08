
#include "BufferDefine.hlsl"

cbuffer cbPerObject : register(b0)
{
	float4x4 g_mWorldMat;
};

struct VertexIn
{
	float3 PosL : POSITION;
};

struct VertexOut
{
	float4 PosH : SV_POSITION;
};

VertexOut VSMain(VertexIn vin)
{
	VertexOut vout;

	float4 PosW = mul(float4(vin.PosL, 1.0f), g_mWorldMat);
	vout.PosH = mul(PosW, g_mViewProj);
	return vout;
}

float4 PSMain(VertexOut pin) : SV_Target
{
    return float4(1.0f, 0.0f, 0.0f, 1.0f);
}
