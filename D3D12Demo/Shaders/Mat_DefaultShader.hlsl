
#include "BufferDefine.hlsl"
#include "BRDF_Lib.hlsl"
#include "Light_Lib.hlsl"
#include "Common_Function.hlsl"

struct VertexIn
{
    float3 PosL : POSITION;
    float3 Normal : NORMAL0;
    float3 Tangent : NORMAL1;
    float2 UV : TEXCOORD;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float4 PosW : POSITION1;
    float3x3 TBN : TBNMAT;
    float2 UV : TEXCOORD;
};

Texture2D g_AldeboMap : register(t0);
Texture2D g_NormalMap : register(t1);
Texture2D g_RoughnessMap : register(t2);
Texture2D g_MetalMaskMap : register(t3);

VertexOut VSMain(VertexIn vin)
{
    VertexOut vout;
    
    vout.PosW = mul(float4(vin.PosL, 1.0f), g_mWorldMat);
    vout.PosH = mul(vout.PosW, g_mViewProj);
	
	float3 N = normalize(mul(vin.Normal, (float3x3) g_mWorldMat));
    float3 T = normalize(mul(vin.Tangent, (float3x3) g_mWorldMat));
    T = normalize(T - dot(T, N) * N);
    float3 B = cross(T, N);
    vout.TBN = float3x3(T, B, N);

    vout.UV = vin.UV;
    return vout;
}

float4 PSMain(VertexOut pin) : SV_Target
{
    float3 resultColor;

    float3 V = normalize(g_vEyePosition - pin.PosW.xyz);
  
    float3 BaseColor = g_AldeboMap.Sample(g_LinearWrapSampler, pin.UV).rgb;
    BaseColor = pow(BaseColor, 2.2f);

    float3 Mat_N = g_NormalMap.Sample(g_LinearWrapSampler, pin.UV).rgb;
    Mat_N = Mat_N * 2.0f - 1.0f;
    float3 N = mul(Mat_N, pin.TBN);

    float Roughness = g_RoughnessMap.Sample(g_LinearWrapSampler, pin.UV).r;
    float MetalMask = g_MetalMaskMap.Sample(g_LinearWrapSampler, pin.UV).r;

    int pointLightEnd = g_LightNumbers.x + g_LightNumbers.y;
    int spotLightEnd = pointLightEnd + g_LightNumbers.z;

    for (int a = 0; a < g_LightNumbers.x; ++a)
    {
        LightInfo light = g_Lights[a];

        float3 L = -light.LightDirection.xyz;
        L = normalize(L);

        float3 brdf = Default_ShadeModel(N, V, L, BaseColor, Roughness, MetalMask, g_Material.F0);
        float NdotL = saturate(dot(N, L));

        resultColor += brdf * light.LightColor.rgb * light.Intensity * NdotL;
    }
    
    for (int i = g_LightNumbers.x; i < pointLightEnd; ++i)
    {
        LightInfo light = g_Lights[i];

        float3 L = light.LightPosition.xyz - pin.PosW.xyz;
        float toLight = length(L);
        L = L / toLight;

        float3 brdf = Default_ShadeModel(N, V, L, BaseColor, Roughness, MetalMask, g_Material.F0);
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

        float3 brdf = Default_ShadeModel(N, V, L, BaseColor, Roughness, MetalMask, g_Material.F0);
        float fallOff = SpotLightFallOff(light, toLight, L);
        float NdotL = saturate(dot(N, L));

        resultColor += brdf * light.LightColor.rgb * light.Intensity * fallOff * NdotL;
    }
    
    resultColor = linearColorToSRGB(resultColor);
    return float4(resultColor, 1.0f);
}
