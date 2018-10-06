
#include "Common_Define.hlsl"

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

float DistanceFallOff(float refDist, float d)
{
    float distDelta = refDist / max(d, 0.01f);
    float distFalloff = distDelta * distDelta;
    return distFalloff;
}

float DistanceWindowFunction(float maxRadius, float d)
{
    float winDelta = clamp(1.0f - pow(d / maxRadius, 4.0f), 0.0f, 1.0f);
    return winDelta * winDelta;
}

float PointLightFallOff(LightInfo light, float toLightLen)
{
    float distFalloff = DistanceFallOff(light.RefDist, toLightLen);
    float winFallOff = DistanceWindowFunction(light.MaxRadius, toLightLen);
    
    return distFalloff * winFallOff;
}

float SpotLightFallOff(LightInfo light, float toLightLen, float3 toLight)
{
    float distFalloff = DistanceFallOff(light.RefDist, toLightLen);
    float winFallOff = DistanceWindowFunction(light.MaxRadius, toLightLen);
  
    float dirDelta = (dot(toLight, -light.LightDirection.xyz) - light.CosMaxAngle) / (light.CosMinAngle - light.CosMaxAngle);
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
    
    int pointLightEnd = g_LightNumbers.x + g_LightNumbers.y;
    int spotLightEnd = pointLightEnd + g_LightNumbers.z;

    for (int a = 0; a < g_LightNumbers.x; ++a)
    {
        LightInfo light = g_Lights[a];
        resultColor += light.LightColor.rgb;
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
