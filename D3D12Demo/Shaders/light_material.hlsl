
#include "BufferDefine.hlsl"
#include "BRDF_Lib.hlsl"
#include "Light_Lib.hlsl"

struct VertexIn
{
    float3 PosL : POSITION;
    float3 Normal : NORMAL;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float4 PosW : POSITION1;
    float3 NormalW : NORMAL;
};

VertexOut VSMain(VertexIn vin)
{
    VertexOut vout;
  
    vout.PosW = mul(float4(vin.PosL, 1.0f), g_mWorldMat);
    vout.PosH = mul(vout.PosW, g_mViewProj);
    vout.NormalW = mul(vin.Normal, (float3x3) g_mWorldMat);
    vout.NormalW = normalize(vout.NormalW);
    return vout;
}

float GammaCorrection(float a)
{
    if (a > 0.0031308f)
    {
        return 1.055f * pow(abs(a), 1.0f / 2.4f) - 0.055f;
    }
    else
    {
        return 12.92f * a;
    }
}

float3 linearColorToSRGB(float3 color)
{
    return float3(GammaCorrection(color.r), GammaCorrection(color.g), GammaCorrection(color.b));
}

float4 PSMain(VertexOut pin) : SV_Target
{
    float3 toEye = g_vEyePosition - pin.PosW.xyz;
    float VDotN = dot(normalize(toEye), pin.NormalW);

    float3 resultColor;
    
    int pointLightEnd = g_LightNumbers.x + g_LightNumbers.y;
    int spotLightEnd = pointLightEnd + g_LightNumbers.z;

    for (int a = 0; a < g_LightNumbers.x; ++a)
    {
        LightInfo light = g_Lights[a];

        float3 L = -light.LightDirection.xyz;
        float3 V = normalize(g_vEyePosition - pin.PosW.xyz);
        float3 N = pin.NormalW;
        float3 H = normalize(L + V);

        float NdotV = abs(dot(N, V)) + 1e-5f;
        float LdotH = saturate(dot(L, H));
        float NdotL = saturate(dot(N, L));
        float NdotH = saturate(dot(N, H));
        
        float Fd = Fr_DisneyDiffuse(NdotV, NdotL, LdotH, 0.9f);

        resultColor += light.LightColor.rgb * g_Material.DiffuseColor.rgb * Fd * NdotL * XM_1DIVPI;

    }
    
    for (int i = g_LightNumbers.x; i < pointLightEnd; ++i)
    {
        LightInfo light = g_Lights[i];
        float3 toLight = light.LightPosition.xyz - pin.PosW.xyz;
        float toLightLength = length(toLight);
        toLight = toLight / toLightLength;
        resultColor += light.LightColor.rgb * PointLightFallOff(light, toLightLength);
    }

    for (int j = pointLightEnd; j < spotLightEnd; ++j)
    {
        LightInfo light = g_Lights[j];
        float3 toLight = light.LightPosition.xyz - pin.PosW.xyz;
        float toLightLength = length(toLight);
        toLight = toLight / toLightLength;
        resultColor += light.LightColor.rgb * SpotLightFallOff(light, toLightLength, toLight);
    }

    resultColor = linearColorToSRGB(resultColor);
    return float4(resultColor, 1.0f);
}
