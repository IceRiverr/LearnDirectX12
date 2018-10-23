
// https://github.com/KhronosGroup/glTF-WebGL-PBR
// 参考这个实现来做的

#define M_PI		3.141592653529893
#define M_1DIVPI	0.31830988618379

struct LightInfo
{
    float3 LightColor;
    float Intensity;
    float4 LightDirection;
    float4 LightPosition;

    float RefDist;
    float MaxRadius;
    float CosMinAngle;
    float CosMaxAngle;
};

struct PBRMaterial
{
    float4 BaseColorFactor;
    float4 EmissiveColorFactor;
    
    float MetallicFactor;
    float RoughnessFactor;
    float NormalScale;
    float OcclusionStrength;
};

cbuffer cbPerPass : register(b0)
{
    float4x4 g_mView;
    float4x4 g_mInvView;
    float4x4 g_mProj;
    float4x4 g_mInvProj;
    float4x4 g_mViewProj;
    float4x4 g_mInvViewProj;
    float3 g_vEyePosition;
    float PAD_0;
    float2 g_RenderTargetSize;
    float2 g_InvRenderTargetSize;
    float g_fNearZ;
    float g_fFarZ;
    float g_fTotalTime;
    float g_fDeltaTime;

    int4 g_LightNumbers;
    LightInfo g_Lights[16];
};

cbuffer cbPerObject : register(b1)
{
    float4x4 g_mWorldMat;
    float4x4 g_mInvWorldMat;
};

cbuffer cbPerMaterial : register(b2)
{
    PBRMaterial g_PBRMat;
}

cbuffer cbPerLight : register(b3)
{
    float4 g_LightDirection;
    float4 g_LightColor;
};

//#define HAS_NORMALS
//#define HAS_TANGENTS
//#define HAS_UV

struct VertexIn
{
    float3 PosL : POSITION;
#ifdef HAS_NORMALS
	float3 Normal : NORMAL;
#endif
#ifdef HAS_TANGENTS
    float4 Tangent : TANGENT;
#endif
#ifdef HAS_UV
    float2 UV : TEXCOORD;
#endif
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 PosW : POSITION1;
#ifdef HAS_NORMALS
#ifdef HAS_TANGENTS
    float3x3 TBN : TBNMAT;
#else
    float3 NormalW : NORMAL;
#endif
#endif
	 float2 UV : TEXCOORD;
};

VertexOut VSMain(VertexIn vin)
{
    VertexOut vout;
    
    float4 pos = mul(float4(vin.PosL, 1.0f), g_mWorldMat);
    vout.PosW = pos.xyz / pos.w;
    vout.PosH = mul(pos, g_mViewProj);
	
#ifdef HAS_NORMALS
#ifdef HAS_TANGENTS
    float3 N = normalize(mul(vin.Normal, (float3x3) g_mWorldMat));
    float3 T = normalize(mul(vin.Tangent.xyz, (float3x3) g_mWorldMat));
    float3 B = cross(T, N) * vin.Tangent.w;
    vout.TBN = float3x3(T, B, N);
#else
   vout.NormalW = normalize(mul(vin.Normal, (float3x3) g_mWorldMat));
#endif
#endif
	
#ifdef HAS_UV
    vout.UV = vin.UV;
#else
	vout.UV = float2(0.0f, 0.0f);
#endif
	return vout;
}

//
// This fragment shader defines a reference implementation for Physically Based Shading of
// a microfacet surface material defined by a glTF model.
//
// References:
// [1] Real Shading in Unreal Engine 4
//     http://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf
// [2] Physically Based Shading at Disney
//     http://blog.selfshadow.com/publications/s2012-shading-course/burley/s2012_pbs_disney_brdf_notes_v3.pdf
// [3] README.md - Environment Maps
//     https://github.com/KhronosGroup/glTF-WebGL-PBR/#environment-maps
// [4] "An Inexpensive BRDF Model for Physically based Rendering" by Christophe Schlick
//     https://www.cs.virginia.edu/~jdl/bib/appearance/analytic%20models/schlick94b.pdf


//#define USE_IBL 0
//#define HAS_BASE_COLOR_MAP 1
//#define HAS_NORMAL_MAP 1
//#define HAS_EMISSIVE_MAP 0
//#define HAS_METAL_ROUGHNESS_MAP 1
//#define HAS_OCCLUSION_MAP 1

#ifdef HAS_BASE_COLOR_MAP
Texture2D g_BaseColorMap : register(t0);
#endif
#ifdef HAS_NORMAL_MAP
Texture2D g_NormalMap : register(t1);
#endif
#ifdef HAS_EMISSIVE_MAP
Texture2D g_EmissiveMap : register(t2);
#endif
#ifdef HAS_METAL_ROUGHNESS_MAP
Texture2D g_MetallicRoughnessMap : register(t3);
#endif
#ifdef HAS_OCCLUSION_MAP
Texture2D g_OcclusionMap : register(t4);
#endif

#ifdef USE_IBL
TextureCube g_DiffuseEnvMap : register(t10);
TextureCube g_SpecularEnvMap : register(t11);
Texture2D g_BrdfLUT : register(t12);
#endif


float c_MinRoughness = 0.04f;

SamplerState g_PointWrapSampler : register(s0);
SamplerState g_PointClampSampler : register(s1);
SamplerState g_LinearWrapSampler : register(s2);
SamplerState g_LinearClampSampler : register(s3);
SamplerState g_AnisotropicWrapSampler : register(s4);
SamplerState g_AnisotropicClampSampler : register(s5);

#define SRGB_FAST_APPROXIMATION
float4 SRGBToLinear(float4 srgbIn)
{
#ifdef SRGB_FAST_APPROXIMATION
    float3 linearOut = pow(srgbIn.rgb, 2.2f);
    return float4(linearOut.rgb, srgbIn.a);
#endif
    return srgbIn;
}

float3 Fd_LambertDiffuse(float3 diffuseColor)
{
    return diffuseColor * M_1DIVPI;
}

float3 F_Schick(in float3 f0, in float3 f90, in float u)
{
    return f0 + (f90 - f0) * pow(clamp(1.0f - u, 0.0f, 1.0f), 5.0f);
}

// 使用Frosbite的方法来做，Specular的 G /( 4.0f * NodtL * NdotV) 一起来作为整体计算
// 这样近似，可以使用一个MultiplyAdd，可能是这个原因，具体不知道
float G_SmithGGXCorrelated(float NdotL, float NdotV, float alphaG)
{
    float alphaG2 = alphaG * alphaG;
    float Lambda_GGXV = NdotL * sqrt((-NdotV * alphaG2 + NdotV) * NdotV + alphaG2);
    float Lambda_GGXL = NdotV * sqrt((-NdotL * alphaG2 + NdotL) * NdotL + alphaG2);

    return 0.5f / (Lambda_GGXV + Lambda_GGXL);
}

// GGX/Towbridge-Reitz
float D_GGX(float NdotH, float m)
{
	// Divide by PI 可以放到最后来做 可以和fd的除PI合并，减少一次乘法运算
    float m2 = m * m;
    float f = (NdotH * m2 - NdotH) * NdotH + 1.0f;
    return m2 / (f * f * M_PI);
}

float4 PSMain(VertexOut pin) : SV_Target
{
	// DO TEST
    float3 LightDir = float3(0.0f, 1.0f, 0.0f);
    float3 LightColor = 3.14f;

    float perceptualRoughness = g_PBRMat.RoughnessFactor;
    float metallic = g_PBRMat.MetallicFactor;

#ifdef HAS_METAL_ROUGHNESS_MAP
    float4 mrSample = g_MetallicRoughnessMap.Sample(g_LinearWrapSampler, pin.UV);
    perceptualRoughness *= mrSample.g;
    metallic *= mrSample.b;
#endif
    perceptualRoughness = clamp(perceptualRoughness, 0.0f, 1.0f);
    metallic = clamp(metallic, 0.0f, 1.0f);

    float alphaRoughness = perceptualRoughness * perceptualRoughness;

#ifdef HAS_BASE_COLOR_MAP
    float4 baseColor = SRGBToLinear(g_BaseColorMap.Sample(g_LinearWrapSampler, pin.UV)) * g_PBRMat.BaseColorFactor;
#else
	float4 baseColor = g_PBRMat.BaseColorFactor;
#endif

	// 将绝缘体的反射率固定为 0.04f，后期可以加在材质中 Reflectance
    float3 f0 = 0.04f;
    float3 diffuseColor = baseColor.rgb * (1.0f - f0);
    diffuseColor *= 1.0f - metallic;
    float3 specularColor = lerp(f0, diffuseColor, metallic);

	// For typical incident reflectance range (between 4% to 100%) set the grazing reflectance to 100% for typical fresnel effect.
    // For very low reflectance range on highly diffuse objects (below 4%), incrementally reduce grazing reflecance to 0%.
    float3 specularR0 = specularColor;
    float3 specularR90 = 1.0f;
    //float reflectance = max(max(specularColor.r, specularColor.g), specularColor.b);
    //float3 specularR90 = clamp(reflectance * 25.0f, 0.0f, 1.0f);

	// Normal
    float3 N = float3(0.0f, 1.0f, 0.0f);
#ifdef HAS_NORMALS
#ifdef HAS_TANGENTS
#ifdef HAS_NORMAL_MAP
    float3 BumpNormal = g_NormalMap.Sample(g_LinearWrapSampler, pin.UV).rgb;
    BumpNormal = BumpNormal * 2.0f - 1.0f;
    N = mul(BumpNormal, pin.TBN) * float3(g_PBRMat.NormalScale, g_PBRMat.NormalScale, 1.0f);
    N = normalize(N);
#endif
#else
    N = pin.NormalW;
#endif
#endif

    float3 V = normalize(g_vEyePosition.xyz - pin.PosW);
   // float3 L = normalize(g_LightDirection.xyz);
    float3 L = normalize(LightDir.xyz);
    float3 H = normalize(V + L);
    float3 R = 2.0f * dot(V, N) * N - V;

    float NdotL = clamp(dot(N, L), 0.001f, 1.0f);
    float NdotV = clamp(abs(dot(N, V)), 0.001f, 1.0f);
    float NdotH = clamp(dot(N, H), 0.0f, 1.0f);
    float LdotH = clamp(dot(L, H), 0.0f, 1.0f);
    float VdotH = clamp(dot(V, H), 0.0f, 1.0f);

	// Direct Light
    float3 F = F_Schick(specularR0, specularR90, VdotH);
    float G = G_SmithGGXCorrelated(NdotL, NdotV, alphaRoughness);
    float D = D_GGX(NdotH, alphaRoughness);

    float3 diffuseContribute = (1.0f - F) * diffuseColor;
    float3 specularContribute = F * G * D;

    //float3 color = g_LightColor.rgb * (diffuseContribute + specularContribute) * NdotL;
    float3 color = LightColor.rgb * (diffuseContribute + specularContribute) * NdotL;

	// IBL Light
#ifdef USE_IBL
	float mipCount = 9.0f; // 512x512
    float lod = perceptualRoughness * mipCount;
	
	float3 brdf = SRGBToLinear(g_BrdfLUT.Sample(g_LinearClampSampler, float2(NdotV, 1.0f - perceptualRoughness)));
	float3 diffuseEnvLight = SRGBToLinear(g_DiffuseEnvMap.Sample(g_LinearWrapSampler, N)).rgb;
	float3 specularEnvLight = SRGBToLinear(g_SpecularEnvMap.Sample(g_LinearWrapSampler, R)).rgb;
	
    float3 diffuseEnv = diffuseEnvLight * diffuseColor;
    float3 specularEnv = specularEnvLight * (specularColor * brdf.x + brdf.y);
	
    color += diffuseEnv + specularEnv;
#endif

#ifdef HAS_OCCLUSION_MAP
    float ao = g_OcclusionMap.Sample(g_LinearClampSampler, pin.UV).r;
    color = lerp(color, color * ao, g_PBRMat.OcclusionStrength);
#endif

#ifdef HAS_EMISSIVE_MAP
    float3 emissive = SRGBToLinear(g_EmissiveMap.Sample(g_LinearWrapSampler, pin.UV)).rgb * g_PBRMat.EmissiveColorFactor.rgb;
#else
    float3 emissive = g_PBRMat.EmissiveColorFactor.rgb;
#endif
    color += emissive;

	return float4(pow(color, 1.0f/2.2f), baseColor.a);
}
