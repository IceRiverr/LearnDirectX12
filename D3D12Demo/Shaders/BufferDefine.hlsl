
struct LightInfo
{
    float4 LightColor;
    float4 LightDirection;
    float4 LightPosition;
    float4 LightRange; // radius min
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

    LightInfo g_Lights[16];
    int g_nLightCount;
    float PAD_1[3];
};
