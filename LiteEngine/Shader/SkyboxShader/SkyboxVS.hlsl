#include "SkyboxDefs.hlsli"
#include "../Definitions.hlsli"
#include "../FixedVSConstants.hlsli"

Skybox_VS_OUTPUT main( float3 pos : POSITION ) {
	float3 cameraPos_W = trans_V2W._m03_m13_m23 / trans_V2W._m33;

	Skybox_VS_OUTPUT oval;
	oval.position_W = pos;
	oval.position_SV = mul(trans_W2C, float4(pos + cameraPos_W, 1));

	// perspective corrected interpolation 是根据 w 来做的，所以改 z
	oval.position_SV.z = oval.position_SV.w;
	return oval;
}