#ifndef _COMMON_DEFINE_H_
#define _COMMON_DEFINE_H_

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

struct BRDFMaterial
{
    float4 BaseColor;

    float Roughness;
    float MetalMask;
    float F0;

    float Unused;
};

cbuffer cbPerObject : register(b0)
{
    float4x4 g_mWorldMat;
    float4x4 g_mInvWorldMat;
};

cbuffer cbPerPass : register(b1)
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

cbuffer cbPerMaterial : register(b2)
{
    BRDFMaterial g_Material;
}

SamplerState g_PointWrapSampler			: register(s0);
SamplerState g_PointClampSampler		: register(s1);
SamplerState g_LinearWrapSampler		: register(s2);
SamplerState g_LinearClampSampler		: register(s3);
SamplerState g_AnisotropicWrapSampler	: register(s4);
SamplerState g_AnisotropicClampSampler	: register(s5);

#endif
