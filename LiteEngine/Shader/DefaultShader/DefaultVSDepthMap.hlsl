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


float4 main(Default_VS_INPUT vdata): SV_POSITION {
	Default_VS_OUTPUT outval;

	float4 position_W4 = mul(trans_L2W, float4(vdata.position_L, 1));
	return mul(trans_W2C, position_W4);
}