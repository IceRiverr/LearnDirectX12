
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
    float3 resultColor;

    float3 V = normalize(g_vEyePosition - pin.PosW.xyz);
    float3 N = pin.NormalW;
    
    int pointLightEnd = g_LightNumbers.x + g_LightNumbers.y;
    int spotLightEnd = pointLightEnd + g_LightNumbers.z;

    for (int a = 0; a < g_LightNumbers.x; ++a)
    {
        LightInfo light = g_Lights[a];

        float3 L = -light.LightDirection.xyz;
        L = normalize(L);

        float3 brdf = Default_ShadeModel(N, V, L, g_Material.BaseColor.xyz, g_Material.Roughness, g_Material.MetalMask, g_Material.F0);
        float NdotL = saturate(dot(N, L));

        resultColor += brdf * light.LightColor.rgb * light.Intensity * NdotL;
    }
    
    for (int i = g_LightNumbers.x; i < pointLightEnd; ++i)
    {
        LightInfo light = g_Lights[i];

        float3 L = light.LightPosition.xyz - pin.PosW.xyz;
        float toLight = length(L);
        L = L / toLight;

        float3 brdf = Default_ShadeModel(N, V, L, g_Material.BaseColor.xyz, g_Material.Roughness, g_Material.MetalMask, g_Material.F0);
        float fallOff = PointLightFallOff(light, toLight);
        float NdotL = saturate(dot(N, L));

        resultColor += brdf * light.LightColor.rgb * light.Intensity * fallOff * NdotL;
    }

    for (int j = pointLightEnd; j < spotLightEnd; ++j)
    {
        LightInfo light = g_Lights[i];

        float3 L = light.LightPosition.xyz - pin.PosW.xyz;
        float toLight = length(L);
        L = L / toLight;

        float3 brdf = Default_ShadeModel(N, V, L, g_Material.BaseColor.xyz, g_Material.Roughness, g_Material.MetalMask, g_Material.F0);
        float fallOff = SpotLightFallOff(light, toLight, L);
        float NdotL = saturate(dot(N, L));

        resultColor += brdf * light.LightColor.rgb * light.Intensity * fallOff * NdotL;
    }
    
    resultColor = linearColorToSRGB(resultColor);
    return float4(resultColor, 1.0f);
}
