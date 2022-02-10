#ifndef LITE_ENGINE_SHAREDR_DEFINITIONS_HLSLI
#define LITE_ENGINE_SHAREDR_DEFINITIONS_HLSLI

static const uint LIGHT_TYPE_POINT = 0x0;
static const uint LIGHT_TYPE_DIRECTIONAL = 0x1;
static const uint LIGHT_TYPE_SPOT = 0x2;

static const uint LIGHT_SHADOW_EMPTY = 0x1;
static const uint LIGHT_SHADOW_HARD = 0x2;
static const uint LIGHT_SHADOW_SOFT = 0x3;	// TODO: unimplemented

#define MAX_NUMBER_OF_LIGHTS 4

// 这些都是 custom 的 constants

#define REGISTER_PS_LONGTERM b0
#define REGISTER_PS_PERFRAME b1
#define REGISTER_PS_PEROBJECT b2
#define REGISTER_PS_MATERIAL b6

#define REGISTER_VS_LONGTERM b0
#define REGISTER_VS_PERFRAME b1
#define REGISTER_VS_PEROBJECT b2

#endif