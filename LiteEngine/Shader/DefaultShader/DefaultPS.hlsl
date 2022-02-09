#include "DefaultDefs.hlsli"
#include "../Definitions.hlsli"
#include "../FixedPSConstants.hlsli"

//#define SLOT_BASE_COLOR			0
//#define SLOT_EMISSION_COLOR		1
//#define SLOT_METALLIC				2
//#define SLOT_ROUGHNESS			3
//#define SLOT_AMBIENT_OCCLUSION	4
//#define SLOT_NORMAL				5

cbuffer DefaultPSConstant : register(REGISTER_PS_MATERIAL) {
	float4 c_baseColor;
	float4 c_emissionColor;

	float c_metallic;
	float c_roughness;
	float c_anisotropy;
	float c_occlusionStrength;

	float c_normalScale;
	uint uvBaseColor;
	uint uvEmissionColor;
	uint uvMetallic;

	uint uvRoughness;
	uint uvAO;
	uint uvNormal;
};


Texture2D texBaseColor: register(t0);
sampler sampBaseColor: register(s0);

Texture2D texEmissionColor: register(t1);
sampler sampEmissionColor: register(s1);

Texture2D texMetallic: register(t2);
sampler sampMetallic: register(s2);

Texture2D texRoughness: register(t3);
sampler sampRoughness: register(s3);

Texture2D texAO: register(t4);
sampler sampAO: register(s5);

Texture2D texNormal: register(t5);
sampler sampNormal: register(s5);


float4 main(Default_VS_OUTPUT pdata) : SV_TARGET{
	float4 baseColor;

	if (uvBaseColor == 0) {
		baseColor = texBaseColor.Sample(sampBaseColor, pdata.texCoord0);
	} else if (uvBaseColor == 1) {
		baseColor = texBaseColor.Sample(sampBaseColor, pdata.texCoord1);
	} else {
		baseColor = c_baseColor;
	}

	baseColor *= pdata.color;

	// ambient
	float3 output = float3(0, 0, 0); // baseColor.xyz* float3(0.1, 0.1, 0.1);

	float3 cameraPos_W = trans_V2W._m03_m13_m23 / trans_V2W._m33;
	float3 cameraDir_W = normalize(cameraPos_W - pdata.position_W);

	for (uint i = 0; i < numberOfLights; i++) {

		float3 positionVecDist = lights[i].position_W - pdata.position_W;
		float distance2 = dot(positionVecDist, positionVecDist);
		float distance = sqrt(distance2);
		float3 lightDir_W = positionVecDist / distance;

		float3 normal_W_normalized = normalize(pdata.normal_W);
		float cosLightNormal = dot(normal_W_normalized, lightDir_W);

		float3 halfVector = normalize(lightDir_W + cameraDir_W);
		float cosHalfNormal = dot(normal_W_normalized, halfVector);

		// diffuse
		output += (baseColor.xyz * lights[i].intensity) * max(cosLightNormal, 0) / distance2;
	
	}

	return float4(pow(max(float3(0, 0, 0), output), 1 / 2.2), 1.0);
}