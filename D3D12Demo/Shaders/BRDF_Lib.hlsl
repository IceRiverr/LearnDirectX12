

#define XM_PI 3.141592654f
#define XM_1DIVPI 0.318309886f

// Fresnel 近似
float3 F_Schick(in float3 f0, in float3 f90, in float u)
{
    return f0 + (f90 - f0) * pow(1.0f - u, 5.0f);
}

// 有些过亮
float DisneyDiffuse(float NdotV, float NdotL, float LdotH, float linearRoughness)
{
    float3 f0 = float3(1.0f, 1.0f, 1.0f);
    float3 f90 = 0.5f + 2.0f * linearRoughness * LdotH * LdotH;

    float lightScatter  = F_Schick(f0, f90, NdotL).r;
    float viewScatter   = F_Schick(f0, f90, NdotV).r;

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
