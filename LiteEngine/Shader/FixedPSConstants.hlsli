#ifndef LITE_ENGINE_SHAREDR_PS_FIXED_CONSTANTS_HLSLI
#define LITE_ENGINE_SHAREDR_PS_FIXED_CONSTANTS_HLSLI

#include "Definitions.hlsli"

struct Light {
	int type;						// all
	int shadow;						// all
	float innerConeAngle;			    // spot
	float outerConeAngle;				// spot
	
	float3 position_W;			// spot & point
	float maximumDistance;			// spot & point

	float3 direction_W;			// spot & directional
	float3 intensity;			// all
};


cbuffer FixedLongtermConstantBufferData : register(b3) {
	float screen_widthInPixel;
	float screen_heightInPixel;
	float desiredFPS;
};


cbuffer FixedPerframePSConstantBufferData : register(b4) {
	matrix trans_W2V;
	matrix trans_V2W;

	float timeInSecond;
	float currentFPS;
	float exposure;
	uint numberOfLights; // space of 4bytes.

	Light lights[MAX_NUMBER_OF_LIGHTS];
};

cbuffer FixedPerobjectPSConstants : register(b5) {
	matrix trans_L2W;
	matrix trans_W2L;
};

#endif