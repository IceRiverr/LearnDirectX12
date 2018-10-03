#ifndef _BRDF_LIB_H_
#define _BRDF_LIB_H_

#define XM_PI 3.141592654f
#define XM_1DIVPI 0.318309886f

// Fresnel 近似
float3 F_Schick(in float3 f0, in float3 f90, in float u)
{
    return f0 + (f90 - f0) * pow(1.0f - u, 5.0f);
}

// From Frosbite
float D_GGX_TR(float NdotH, float m)
{
    // NDF_GGX_TR(n, h, a) = a^2 / (PI * ((n*h)^2*(a^2-1)+1)^2)

    // Divide by PI is apply later
    float m2 = m * m;
    // float f = NdotH * NdotH * (m2 - 1.0f) + 1.0f;
    float f = (NdotH * m2 - NdotH) * NdotH + 1.0f;
    return m2 / (f * f);
}

// From Frosbite
float V_SimthGGXCorrelated(float NdotL, float NdotV, float alphaG)
{
    // Original formulation of G_SmithGGX Correlated
    // lambda_v = (-1 + sqrt(alphaG2 * (1 - NdotV2) / NodtV2) + 1) * 0.5f;
    // lambda_l = (-1 + sqrt(alphaG2 * (1 - NdotL2) / NdotL2) + 1) * 0.5f;
    // G_SmithGGXCorrelated = 1.0f / (1.0f + lambda_v + lambda_l);
    // V_SmithGGXCorrelated = G_SmithGGXCorrelated / (4.0f * NdotL * NdotV);

    // This is the optimize version
    float alphaG2 = alphaG * alphaG;
    float lambda_GGXV = NdotL * sqrt((-NdotV * alphaG2 + NdotV) * NdotV + alphaG2);
    float lambda_GGXL = NdotV * sqrt((-NdotL * alphaG2 + NdotL) * NdotL + alphaG2);

    return 0.5f / (lambda_GGXV + lambda_GGXL);
}

// 有些过亮
float DisneyDiffuse(float NdotV, float NdotL, float LdotH, float linearRoughness)
{
    float3 f0 = float3(1.0f, 1.0f, 1.0f);
    float3 f90 = 0.5f + 2.0f * linearRoughness * LdotH * LdotH;

    float lightScatter = F_Schick(f0, f90, NdotL).r;
    float viewScatter = F_Schick(f0, f90, NdotV).r;

    return lightScatter * viewScatter;
}

float Fr_DisneyDiffuse(float NdotV, float NdotL, float LdotH, float linearRoughness)
{
    float energyBias = lerp(0.0f, 0.5f, linearRoughness);
    float energyFactor = lerp(1.0f, 1.0f / 1.5f, linearRoughness);

    float3 f0 = float3(1.0f, 1.0f, 1.0f);
    float3 f90 = energyBias + 2.0f * linearRoughness * LdotH * LdotH;

    float lightScatter = F_Schick(f0, f90, NdotL).r;
    float viewScatter = F_Schick(f0, f90, NdotV).r;

    return lightScatter * viewScatter * energyFactor;
}

float3 Fr_Specular(float NdotV, float NdotL, float NdotH, float linearRoughness, float3 F)
{
    float Vis = V_SimthGGXCorrelated(NdotL, NdotV, linearRoughness);
    float D = D_GGX_TR(NdotH, linearRoughness);
    // 除PI 放到最后来做  F.x * Vis * D * XM_1DIVPI;
    return F * Vis * D;
}

float3 Default_ShadeModel(float3 N, float3 V, float3 L, float3 BaseColor, float Roughness, float MetalMask, float F0)
{
    float3 H = normalize(L + V);

    float NdotV = abs(dot(N, V)) + 1e-5f;
    float LdotH = saturate(dot(L, H));
    float NdotL = saturate(dot(N, L));
    float NdotH = saturate(dot(N, H));
    
    // Diffuse BRDF
    float Fd = Fr_DisneyDiffuse(NdotV, NdotL, LdotH, Roughness);

    // Specular BRDF
    float3 f0 = lerp(float3(F0, F0, F0), BaseColor, MetalMask);
    float3 f90 = float3(1.0f, 1.0f, 1.0f);
    float3 F = F_Schick(f0, f90, LdotH);
    float3 Fr = Fr_Specular(NdotV, NdotL, NdotH, Roughness, F);

    // Diffuse Weight
    float3 diffuseWeight = float3(1.0f, 1.0f, 1.0f) - F;
    diffuseWeight *= 1.0f - MetalMask;

    return (BaseColor.rgb * diffuseWeight * Fd + Fr) * XM_1DIVPI;
}

#endif
