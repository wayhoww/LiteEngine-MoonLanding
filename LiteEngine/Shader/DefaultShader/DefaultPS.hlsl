#include "DefaultDefs.hlsli"
#include "../Definitions.hlsli"
#include "../FixedPSConstants.hlsli"

//#define SLOT_BASE_COLOR			0
//#define SLOT_EMISSION_COLOR		1
//#define SLOT_METALLIC				2
//#define SLOT_ROUGHNESS			3
//#define SLOT_AMBIENT_OCCLUSION	4
//#define SLOT_NORMAL				5

const static uint N_TEXCOORDS = 2;
const static float PI = 3.14159265f;

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

float3 fresnelMix(float ior, float3 base, float3 layer, float VdotHPow5) {
	float f0 = pow((1 - ior) / (1 + ior), 2);
	// float f0 = 0.033735943;
	float fr = f0 + (1 - f0) * VdotHPow5;
	return fr * layer + (1 - fr) * base;
}

// V: view
// H: half
// L: light
// N: normal
// normalized vectors
float specularBRDF(float3 V, float3 H, float3 L, float3 N, float alpha) {
	float HdotL = dot(H, L);
	float HdotV = dot(H, V);
	float NdotH = dot(N, H);

	if (NdotH <= 0 || HdotL <= 0 || HdotV <= 0) {
		return 0;
	} else {

		float absNdotL = abs(dot(N, L));
		float absNdotV = abs(dot(N, V));
		float a2 = alpha * alpha;

		float V_val = 1;
		V_val /= (absNdotL * sqrt(max(0, (a2 + (1 - a2) * absNdotL * absNdotL))));
		V_val /= (absNdotV * sqrt(max(0, (a2 + (1 - a2) * absNdotV * absNdotV))));

		float D_val = a2 / (PI * pow(NdotH * NdotH * (a2 - 1) + 1, 2));

		return D_val;
	}
}

float3 defaultMaterialBRDF(
	// material
	float3 baseColor,
	float metallic, 
	float roughness, 
	float ior,

	// normalized view direction
	float3 V,

	// normalized light direction
	float3 L,

	// normalized surface normal
	float3 N
) {
	// 因为有 normal map 的关系，这里不能加 dot(V, H) < N
	if (dot(L, N) < 0) {
		return float3(0, 0, 0);
	} else {
		float3 H = normalize(L + V);
		float VdotH = dot(V, H);
		float VdotHPow5 = pow(max(0, 1 - abs(VdotH)), 5);
		float alpha = roughness * roughness;
		float3 base = (1 / PI) * baseColor;
		float3 layer = specularBRDF(V, H, L, N, alpha);
		float3 dielectric = fresnelMix(ior, base, layer, VdotHPow5);
		float3 metal = layer * (baseColor + (1 - baseColor) * VdotHPow5);
		return (1.0 - metallic) * dielectric + metallic * metal;
	}
}

float3 getNormal(float3 normal, float3 tangent, float3 normalMapValue, float scale) {
	tangent = normalize(tangent - dot(tangent, normal) * normal / length(normal));
	float3 bitangent = normalize(cross(normal, tangent));
	float3x3 TBN = float3x3(tangent, bitangent, normal);
	float3 scaledNormal = normalize(normalMapValue * 2.0 - 1.0) * float3(scale, scale, 1.0);
	return normalize(mul(TBN, scaledNormal));
}


float4 main(Default_VS_OUTPUT pdata) : SV_TARGET{

	float2 texCoords[2] = { pdata.texCoord0, pdata.texCoord1 };
	float4 ZERO4 = { 0, 0, 0, 0 };

	float4 baseColor = pdata.color * 
		( uvBaseColor < N_TEXCOORDS ? 
		  pow(max(ZERO4, texBaseColor.Sample(sampBaseColor, texCoords[uvBaseColor])), 2.2)
	    : c_baseColor );

	float metallic = uvMetallic < N_TEXCOORDS ? texMetallic.Sample(sampMetallic, texCoords[uvMetallic]).x : c_metallic;
	float roughness = uvRoughness < N_TEXCOORDS ? texRoughness.Sample(sampRoughness, texCoords[uvRoughness]).x : c_roughness;
	// todo normal scale
	float3 normal_W = uvNormal < N_TEXCOORDS ? 
		getNormal(
			normalize(pdata.normal_W), 
			normalize(pdata.tangent_W), 
			texNormal.Sample(sampNormal, texCoords[uvNormal]).xyz,
			c_normalScale
		) : normalize(pdata.normal_W);

	// ambient
	float3 output = float3(0, 0, 0); // baseColor.xyz* float3(0.1, 0.1, 0.1);

	float3 cameraPos_W = trans_V2W._m03_m13_m23 / trans_V2W._m33;
	float3 cameraDir_W = normalize(cameraPos_W - pdata.position_W);	// i.e. V

	for (uint i = 0; i < numberOfLights; i++) {

		float3 positionVecDist = lights[i].position_W - pdata.position_W;
		float distance2 = dot(positionVecDist, positionVecDist);
		float distance = sqrt(distance2);
		float3 lightDir_W = positionVecDist / distance;	// i.e. L

		
		// todo ior
		float3 brdf = defaultMaterialBRDF(baseColor.xyz, metallic, roughness, 1.45, cameraDir_W, lightDir_W, normal_W);

		float cosLightNormal = dot(normal_W, lightDir_W);

		output += brdf * lights[i].intensity * cosLightNormal / distance2;

	}

	return float4(pow(max(float3(0, 0, 0), output), 1 / 2.2), 1.0);
}