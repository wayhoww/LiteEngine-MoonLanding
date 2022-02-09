// XMMatrix 中给出的矩阵都是右乘用的矩阵 （如 Scale 等函数给出的矩阵）
// XMMatrix 存储矩阵的方式是行主序
// HLSL 默认采用列主序
// HLSL 中把传过来的矩阵直接当作左乘矩阵刚刚好~

#ifndef LITE_ENGINE_SHAREDR_VS_FIXED_CONSTANTS_HLSLI
#define LITE_ENGINE_SHAREDR_VS_FIXED_CONSTANTS_HLSLI

#include "Definitions.hlsli"

cbuffer FixedLongtermVSConstants : register(b3) {
	float widthInPixel;
	float heightInPixel;
	float desiredFPS;
};

cbuffer FixedPerframeVSConstants : register(b4) {
	matrix trans_W2V;
	matrix trans_V2C;
	matrix trans_W2C;
	float timeInSecond;
	float currentFPS;
};

cbuffer FixedPerobjectVSConstants : register(b5) {
	matrix trans_L2W;
	matrix trans_W2L;
};

#endif