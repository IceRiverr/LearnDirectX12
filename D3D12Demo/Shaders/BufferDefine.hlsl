
cbuffer cbPerPass : register(b1)
{
    float4x4 g_mView;
    float4x4 g_mInvView;
    float4x4 g_mProj;
    float4x4 g_mInvProj;
    float4x4 g_mViewProj;
    float4x4 g_mInvViewProj;
    float3 g_vEyePosition;
    float PAD_1;
    float2 g_RenderTargetSize;
    float2 g_InvRenderTargetSize;
    float g_fNearZ;
    float g_fFarZ;
    float g_fTotalTime;
    float g_fDeltaTime;
};
