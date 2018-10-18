
#include "Common_Define.hlsl"
#include "BRDF_Lib.hlsl"
#include "Light_Lib.hlsl"
#include "Common_Function.hlsl"

#define USE_HDRI_LIGHTING 1

struct VertexIn
{
    float3 PosL : POSITION;
    float3 Normal : NORMAL;
    float2 UV : TEXCOORD;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float4 PosW : POSITION1;
    float3 NormalW : NORMAL;
    float2 UV : TEXCOORD;
};

#if USE_HDRI_LIGHTING
Texture2D g_EnvironmentEnvMap : register(t4);
Texture2D g_EnvironmentRefMap : register(t5);
#endif

VertexOut VSMain(VertexIn vin)
{
    VertexOut vout;
  
    vout.PosW = mul(float4(vin.PosL, 1.0f), g_mWorldMat);
    vout.PosH = mul(vout.PosW, g_mViewProj);
    vout.NormalW = mul(vin.Normal, (float3x3) g_mWorldMat);
    vout.NormalW = normalize(vout.NormalW);
    vout.UV = vin.UV;
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

	#if USE_HDRI_LIGHTING
	{
        float2 ambientUV = EnvironmentDirectionToEquirectangular(N);
        float3 ambientColor = g_EnvironmentEnvMap.Sample(g_LinearWrapSampler, ambientUV).xyz;
        float3 brdf = Default_ShadeModel(N, V, N, g_Material.BaseColor.xyz, g_Material.Roughness, g_Material.MetalMask, g_Material.F0);
        resultColor += brdf * ambientColor;
    }

	{
        float3 R = dot(V, N) * N - V;
        float2 refUV = EnvironmentDirectionToEquirectangular(R);
        float3 refColor = g_EnvironmentRefMap.Sample(g_LinearWrapSampler, refUV).xyz;
        float3 brdf = Default_ShadeModel(N, V, R, g_Material.BaseColor.xyz, g_Material.Roughness, g_Material.MetalMask, g_Material.F0);
        resultColor += brdf * refColor * dot(R, N);
    }
	#endif
    
    resultColor = linearColorToSRGB(resultColor);
    return float4(resultColor, 1.0f);
}
