// XMMatrix �и����ľ������ҳ��õľ��� ���� Scale �Ⱥ��������ľ���
// XMMatrix �洢����ķ�ʽ��������
// HLSL Ĭ�ϲ���������
// HLSL �аѴ������ľ���ֱ�ӵ�����˾���ոպ�~

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