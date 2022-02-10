#ifndef LITE_ENGINE_SHAREDR_SKYBOX_SHADER_SKYBOX_DEFS_HLSLI
#define LITE_ENGINE_SHAREDR_SKYBOX_SHADER_SKYBOX_DEFS_HLSLI

#include "../Definitions.hlsli"

struct Skybox_VS_OUTPUT {
	// vertex shader Ӧ����һ�� clip space �ģ�
	// ���� pixel shader ���յ����� screen space 
	// (x in [0, W), y in [0, H), z in [0, 1], w = view space depth)

	float4 position_SV      : SV_POSITION;
	float3 position_W       : POSITION;
};


#endif