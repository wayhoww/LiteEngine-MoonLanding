#include "DefaultDefs.hlsli"
#include "../Definitions.hlsli"
#include "../FixedVSConstants.hlsli"


struct Default_VS_INPUT {
	float3 position_L: POSITION;
	float3 normal_L: NORMAL;
	float3 tangent_L: TANGENT;
	float4 color: COLOR;	// apply to base color only
	float2 texCoord0: TEXCOORD0;
	float2 texCoord1: TEXCOORD1;
};


// todo scale 之后 normal 和 tangent 有问题的，要校正
Default_VS_OUTPUT main(Default_VS_INPUT vdata) {
	Default_VS_OUTPUT outval;

	float4 position_W4 = mul(trans_L2W, float4(vdata.position_L, 1));

	outval.position_SV = mul(trans_W2C, position_W4);
	outval.position_W = position_W4.xyz / position_W4.w;
	outval.texCoord0 = vdata.texCoord0;
	outval.texCoord1 = vdata.texCoord1;
	outval.color = vdata.color;

	// 需要做方向插值嘛
	//outval.normal_W = mul(transpose(trans_W2L), float4(vdata.normal_L, 0)).xyz;
	outval.normal_W = mul(float4(vdata.normal_L, 0), trans_W2L).xyz;
	outval.tangent_W = mul(trans_L2W, float4(vdata.tangent_L, 0)).xyz;

	return outval;
}