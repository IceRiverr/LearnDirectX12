
#include "Common_Define.hlsl"

// 使用Latitude-Longitude Mapping，或者说 Equirectangular Projection(ERP) 投影

struct VertexIn
{
    float3 PosL : POSITION;
   
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 PosL : POSITION;
};

Texture2D g_EnvironmentMap : register(t10);

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

float2 EnvironmentDirectionToEquirectangular(float3 dir)
{
    float u = atan2(dir.z, dir.x) * XM_1DIVPI;
    u = (u + 1.0f) * 0.5f;
    float v = acos(dir.y) * XM_1DIVPI;
    
    return float2(u, v);
}

float4 PSMain(VertexOut pin) : SV_Target
{
    float2 uv = EnvironmentDirectionToEquirectangular(pin.PosL);
    float3 color = g_EnvironmentMap.Sample(g_LinearWrapSampler, uv).xyz;
    color = pow(abs(color), 1.0f / 2.2f);
    return float4(color, 1.0f);
}
