
#include "Common_Define.hlsl"

struct VertexIn
{
    float3 PosL : POSITION;
    float2 UV : TEXCOORD;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 PosL : POSITION;
    float2 UV : TEXCOORD;
};

//TextureCube g_CubeMap : register(t0);
Texture2D g_LightMap : register(t10);

VertexOut VSMain(VertexIn vin)
{
    VertexOut vout;

    vout.PosL = vin.PosL;
    vout.UV = vin.UV;

    float4 posW = mul(float4(vin.PosL, 1.0f), g_mWorldMat);
	posW.xyz += g_vEyePosition.xyz;
	
	// z/w 一直等于1，在 远裁剪面处
    vout.PosH = mul(posW, g_mViewProj).xyzw;
    return vout;
}

float4 PSMain(VertexOut pin) : SV_Target
{
	//return g_CubeMap.Sample(g_LinearWrapSampler, pin.PosL);

    float3 color = g_LightMap.Sample(g_LinearWrapSampler, pin.UV).xyz;
    color = pow(color, 1.0f/2.2f);
    return float4(color, 1.0f);
}
