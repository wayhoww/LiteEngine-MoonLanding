#ifndef LITE_ENGINE_SHAREDR_PS_FIXED_CONSTANTS_HLSLI
#define LITE_ENGINE_SHAREDR_PS_FIXED_CONSTANTS_HLSLI

#include "Definitions.hlsli"

struct Light {
	uint type;						// all
	uint shadow;						// all
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

	float exposure;
	uint numberOfLights; 
	// space

	Light lights[MAX_NUMBER_OF_LIGHTS];

	float CSMZList[NUMBER_SHADOW_MAP_PER_LIGHT + 1];	// 4 floats

	// order: light0: map0 map1 map2; light1: map0 map1 map2, ...
	uint CSMValid[MAX_NUMBER_OF_LIGHTS] [NUMBER_SHADOW_MAP_PER_LIGHT] ;	// 12 uint

	// 4 3
	matrix trans_W2CSM[MAX_NUMBER_OF_LIGHTS] [NUMBER_SHADOW_MAP_PER_LIGHT] ;
};

cbuffer FixedPerobjectPSConstants : register(b5) {
	matrix trans_L2W;
	matrix trans_W2L;
};

#endif