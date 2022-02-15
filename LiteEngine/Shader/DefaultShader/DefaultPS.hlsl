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
	float4 c_emissionColor;		// do not support now...

	float c_metallic;
	float c_roughness;
	float c_anisotropy;			// do not support now..
	float c_occlusionStrength;

	float c_normalScale;
	uint uvBaseColor;
	uint uvEmissionColor;
	uint uvMetallic;

	uint uvRoughness;
	uint uvAO;
	uint uvNormal;
	uint channelRoughness;

	uint channelMetallic;
	uint channelAO;
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
sampler sampAO: register(s4);

Texture2D texNormal: register(t5);
sampler sampNormal: register(s5);

Texture2DArray CSMDepthTextures: register(REGISTER_PS_CSM_TEXTURE);
sampler CSMDepthSampler: register(REGISTER_PS_CSM_SAMPLER);

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
	// 基本按照 glTF 2.0 Specification 中对材质的规定写的
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

	float3 cameraPos_W = trans_V2W._m03_m13_m23 / trans_V2W._m33;
	float3 cameraDir_W = normalize(cameraPos_W - pdata.position_W);	// lightID.e. V
	float3 output = float3(0, 0, 0); // baseColor.xyz* float3(0.1, 0.1, 0.1);

	float2 texCoords[2] = { pdata.texCoord0, pdata.texCoord1 };


	float4 sampledBaseColor = uvBaseColor < N_TEXCOORDS ?
		texBaseColor.Sample(sampBaseColor, texCoords[uvBaseColor])
		: float4(1, 1, 1, 1);

	sampledBaseColor.xyz = pow(max(float3(0, 0, 0), sampledBaseColor.xyz), 2.2);

	float4 baseColor = c_baseColor * pdata.color * sampledBaseColor;


	float3 emissionColor = c_emissionColor.xyz * pdata.color.xyz *
		(uvEmissionColor < N_TEXCOORDS ?
			pow(max(float3(0, 0, 0), texEmissionColor.Sample(sampEmissionColor, texCoords[uvEmissionColor]).xyz), 2.2)
			: float3(1, 1, 1));

	float metallic =
		(uvMetallic < N_TEXCOORDS ?
			texMetallic.Sample(sampMetallic, texCoords[uvMetallic])[channelMetallic] : 1)
		* c_metallic;

	float roughness =
		(uvRoughness < N_TEXCOORDS ?
			texRoughness.Sample(sampRoughness, texCoords[uvRoughness])[channelRoughness] : 1)
		* c_roughness;

	float ao =
		(uvAO < N_TEXCOORDS ?
			texAO.Sample(sampAO, texCoords[uvAO])[channelAO] : 1);
	ao = max(0, 1 + c_occlusionStrength * (ao - 1));

	float3 normal_W = uvNormal < N_TEXCOORDS ?
		getNormal(
			normalize(pdata.normal_W),
			normalize(pdata.tangent_W),
			texNormal.Sample(sampNormal, texCoords[uvNormal]).xyz,
			c_normalScale
		) : normalize(pdata.normal_W);

	float depth_V = pdata.position_V.z / pdata.position_V.w;
		
	for (uint lightID = 0; lightID < numberOfLights; lightID++) {
		int coneID = -1;

		for (int j = 0; j < NUMBER_SHADOW_MAP_PER_LIGHT; j++) {
			if (CSMZList[j] <= depth_V && depth_V <= CSMZList[j + 1]) {
				coneID = j;
			} 
		}

		float shadowed = 0;
		if (coneID != -1 && CSMValid[lightID][coneID]) {
			// visibility
			float4 depthMapCoord = mul(trans_W2CSM[lightID][coneID], float4(pdata.position_W, 1));
			depthMapCoord.xyz /= depthMapCoord.w;
			
			[unroll]
			for (float deltaX = -0.001; deltaX < 0.0011; deltaX += 0.001) {
				[unroll]
				for (float deltaY = -0.001; deltaY < 0.0011; deltaY += 0.001) {
					float depthSampled = CSMDepthTextures.Sample(CSMDepthSampler, float3(depthMapCoord.xy + float2(deltaX, deltaY), lightID * NUMBER_SHADOW_MAP_PER_LIGHT + coneID)).x;
					if (depthSampled < depthMapCoord.z) {
						shadowed += 1;
					}
				}
			}
		}

		float visibility = 1 - shadowed / 9;

		Light light = lights[lightID];

		if (light.type == LIGHT_TYPE_POINT || light.type == LIGHT_TYPE_SPOT) {
			float3 positionVecDist = light.position_W - pdata.position_W;
			float distance2 = dot(positionVecDist, positionVecDist);
			float distance = sqrt(distance2);
			float3 lightDir_W = positionVecDist / distance;	// lightID.e. L
			float3 brdf = defaultMaterialBRDF(baseColor.xyz, metallic, roughness, 1.45, cameraDir_W, lightDir_W, normal_W);
			float cosLightNormal = dot(normal_W, lightDir_W);
			output += visibility * brdf * light.intensity * cosLightNormal / distance2;
		} else {
			// Directional
			float3 lightDir_W = -light.direction_W;
			float3 brdf = defaultMaterialBRDF(baseColor.xyz, metallic, roughness, 1.45, cameraDir_W, lightDir_W, normal_W);
			float cosLightNormal = dot(normal_W, lightDir_W);
			output += visibility * brdf * light.intensity * cosLightNormal;
		}
	}

	// ambient 
	output += 0.1 * baseColor.xyz * ao;

	// emission
	output += emissionColor;
	

	return float4(pow(max(float3(0, 0, 0), output), 1 / 2.2), 1.0);
}