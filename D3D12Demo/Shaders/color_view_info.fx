

cbuffer cbPerObject : register(b0)
{
	float4x4 g_mWorldMat;
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
	float PAD_1;
	float2 g_RenderTargetSize;
	float2 g_InvRenderTargetSize;
	float g_fNearZ;
	float g_fFarZ;
	float g_fTotalTime;
	float g_fDeltaTime;
};

struct VertexIn
{
	float3 PosL : POSITION;
	float4 Color : COLOR;
};

struct VertexOut
{
	float4 PosH : SV_POSITION;
	float4 Color : COLOR;
};

VertexOut VSMain(VertexIn vin)
{
	VertexOut vout;

	float4 PosW = mul(float4(vin.PosL, 1.0f), g_mWorldMat);
	vout.PosH = mul(PosW, g_mViewProj);

	vout.Color = vin.Color;
	//vout.Color.r = g_fTotalTime;

	return vout;
}

float4 PSMain(VertexOut pin) : SV_Target
{
	return pin.Color;
}
