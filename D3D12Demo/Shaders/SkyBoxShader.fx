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

TextureCube g_EnvironmentMap : register(t10);

VertexOut VSMain(VertexIn vin)
{
    VertexOut vout;

    vout.PosL = normalize(vin.PosL);
	
    float4 posW = mul(float4(vin.PosL, 1.0f), g_mWorldMat);
    posW.xyz += g_vEyePosition.xyz;
	
	// z/w 一直等于1，在 远裁剪面处
    vout.PosH = mul(posW, g_mViewProj).xyzw;
    return vout;
}

float4 PSMain(VertexOut pin) : SV_Target
{
    float3 uvw = normalize(pin.PosL);
    float3 color = g_EnvironmentMap.Sample(g_LinearWrapSampler, uvw).xyz;
    color = pow(abs(color), 1.0f / 2.2f);
	
    //float3 color = pin.PosL * 0.5f + 0.5f;

    return float4(color, 1.0f);
}
