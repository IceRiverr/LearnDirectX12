
#include "Common_Define.hlsl"

struct VertexIn
{
    float3 PosL : POSITION;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 PosL : POSITION;
};

TextureCube g_CubeMap : register(t0);

VertexOut VSMain(VertexIn vin)
{
    VertexOut vout;

    vout.PosL = vin.PosL;

    float4 posW = mul(float4(vin.PosL, 1.0f), g_mWorldMat);
    posW.xyz += g_vEyePosition.xyz;

	// z/w 一直等于1，在 远裁剪面处
    vout.PosH = mul(posW, g_mViewProj).xyww;

    return vout;
}

float4 PSMain(VertexOut pin) : SV_Target
{
    return g_CubeMap.Sample(g_LinearWrapSampler, pin.PosL);
}
