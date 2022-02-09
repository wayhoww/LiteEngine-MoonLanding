#ifndef LITE_ENGINE_SHAREDR_DEFAULT_SHADER_DEFAULT_DEFS_HLSLI
#define LITE_ENGINE_SHAREDR_DEFAULT_SHADER_DEFAULT_DEFS_HLSLI

#include "../Definitions.hlsli"

struct Default_VS_OUTPUT {
	// vertex shader 应当传一个 clip space 的，
	// 但是 pixel shader 接收到的是 screen space 
	// (x in [0, W), y in [0, H), z in [0, 1], w = view space depth)
	float4 position_SV      : SV_POSITION;

	float3 position_W       : POSITION;
	float3 normal_W 		: NORMAL;		// TODO 这个矫正咋做。。还没弄
	float3 tangent_W	    : TANGENT;
	float2 texCoord0		: TEXCOORD0;
	float2 texCoord1		: TEXCOORD1;
	float4 color			: COLOR;
};


#endif