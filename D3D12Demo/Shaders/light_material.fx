
#include "Common_Define.hlsl"
#include "BRDF_Lib.hlsl"
#include "Light_Lib.hlsl"
#include "Common_Function.hlsl"

#define USE_HDRI_LIGHTING 1

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

#if USE_HDRI_LIGHTING
Texture2D g_EnvironmentEnvMap : register(t4);
Texture2D g_EnvironmentRefMap : register(t5);
Texture2D g_EnvironmentBGMap : register(t6);
#endif

VertexOut VSMain(VertexIn vin)
{
    VertexOut vout;
  
    vout.PosW = mul(float4(vin.PosL, 1.0f), g_mWorldMat);
    vout.PosH = mul(vout.PosW, g_mViewProj);
    vout.NormalW = mul(vin.Normal, (float3x3) g_mWorldMat);
    vout.NormalW = normalize(vout.NormalW);
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
    float3 resultColor = float3(0.0f, 0.0f, 0.0f);

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
        const float EnvIntensity = 5.0f;
        const float RefIntensity = 2.0f;

        float3 R = 2.0f * dot(V, N) * N - V;
        float3 L = R;

        float3 H = normalize(L + V);

        float NdotV = abs(dot(N, V)) + 1e-5f;
        float LdotH = saturate(dot(L, H));
        float NdotL = saturate(dot(N, L));
        float NdotH = saturate(dot(N, H));
		
        float Fd = Fr_DisneyDiffuse(NdotV, NdotL, LdotH, g_Material.Roughness);
		
        float3 f0 = lerp(float3(g_Material.F0, g_Material.F0, g_Material.F0), g_Material.BaseColor.xyz, g_Material.MetalMask);
        float3 f90 = float3(1.0f, 1.0f, 1.0f);
        float3 F = F_Schick(f0, f90, LdotH);
        float3 Fr = Fr_Specular(NdotV, NdotL, NdotH, g_Material.Roughness, F);
		
        float3 diffuseWeight = float3(1.0f, 1.0f, 1.0f) - F;
        diffuseWeight *= 1.0f - g_Material.MetalMask;
		
        float2 ambientUV = EnvironmentDirectionToEquirectangular(R);
        float3 ambientColor = g_EnvironmentEnvMap.Sample(g_LinearWrapSampler, ambientUV).xyz;
        float2 refUV = EnvironmentDirectionToEquirectangular(R);
        float3 refColor = g_EnvironmentRefMap.Sample(g_LinearWrapSampler, refUV).xyz;

        resultColor += ambientColor * g_Material.BaseColor.xyz * diffuseWeight * Fd * XM_1DIVPI * EnvIntensity;
        resultColor += refColor * Fr * XM_1DIVPI * RefIntensity;
    }
	#endif
    
    resultColor = linearColorToSRGB(resultColor);
    return float4(resultColor, 1.0f);
}
