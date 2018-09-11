
#include "BufferDefine.hlsl"

cbuffer cbPerObject : register(b0)
{
    float4x4 g_mWorldMat;
};

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

float DistanceFallOff(float minDist, float maxDist, float d)
{
    float distDelta = maxDist / max(d, minDist);
    float distFalloff = distDelta * distDelta;
    return distFalloff;
}

float PointLightFallOff(LightInfo light, float toLightLen)
{
    float distFalloff = DistanceFallOff(light.LightRange.x, light.LightRange.y, toLightLen);
    
    float winDelta = clamp(1.0f - pow(toLightLen / light.LightRange.y, 4.0f), 0.0f, 1.0f);
    float winFalloff = winDelta * winDelta;

    return distFalloff * winFalloff;
}

float SpotLightFallOff(LightInfo light, float toLightLen, float3 toLight)
{
    float distFalloff = DistanceFallOff(light.LightRange.x, light.LightRange.y, toLightLen);

    float fMinxCosA = light.LightRange.z;
    float fMaxCosA = light.LightRange.w;
    float dirDelta = (dot(toLight, -light.LightDirection.xyz) - fMaxCosA) / (fMinxCosA - fMaxCosA);
    dirDelta = clamp(dirDelta, 0.0f, 1.0f);
    float dirFalloff = pow(dirDelta, 2.0f);

    return distFalloff * dirFalloff;
}

float SpotLightFallOffV2(LightInfo light, float toLightLen, float3 toLight)
{
    float distFalloff = 1.0f;

    float fMinxCosA = light.LightRange.z;
    float fMaxCosA = light.LightRange.w;
    float dirDelta = (dot(toLight, -light.LightDirection.xyz) - fMaxCosA) / (fMinxCosA - fMaxCosA);
    dirDelta = clamp(dirDelta, 0.0f, 1.0f);
    float dirFalloff = pow(dirDelta, 2.0f);

    return distFalloff * dirFalloff;
}

float GammaCorrection(float a)
{
    if (a > 0.0031308f)
    {
        return 1.055f * pow(a, 1.0f / 2.4f) - 0.055f;
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
    //for (int i = 0; i < 2; ++i)
    //{
    //    LightInfo light = g_Lights[i];
    //    float3 toLight = light.LightPosition.xyz - pin.PosW.xyz;
    //    float toLightLength = length(toLight);
            
    //    float LdotN = dot(normalize(toLight), pin.NormalW);

    //    resultColor += light.LightColor.rgb * PointLightFallOff(light, toLightLength);
    //}
    
    for (int i = 2; i < 4; ++i)
    {
        LightInfo light = g_Lights[i];
        float3 toLight = light.LightPosition.xyz - pin.PosW.xyz;
        float toLightLength = length(toLight);
        toLight = toLight / toLightLength;
        resultColor += light.LightColor.rgb * SpotLightFallOffV2(light, toLightLength, toLight);
    }

    resultColor = linearColorToSRGB(resultColor);
    return float4(resultColor, 1.0f);
}
