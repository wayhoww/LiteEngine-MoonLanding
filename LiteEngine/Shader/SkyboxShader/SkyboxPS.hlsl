#include "SkyboxDefs.hlsli"
#include "../Definitions.hlsli"
#include "../FixedPSConstants.hlsli"

cbuffer FixedPerobjectPSConstants : register(REGISTER_PS_MATERIAL) {
	// ������������ϵ������+Z up ת +Y up ��
	// ���磬blender ��ģ���ʹ�õ��� +Z up ����ϵ����
	// �� DirectX ���� TextureCube �Ĳ����������õ��� +Y up ������ϵ����
	// ʹ�� [x', y', z'] ������Խ�������任 ��x' �ǽ�ģ��� x���� DirectX �����еı�ʾ��

	// ��ת���ǿ�֮���Ч��Ҳ������������

	// �������ֻӦ������ת������

	matrix trans_CubeMap;
};

TextureCube texSkybox: register(t0);
sampler sampSkybox: register(s0);

float4 main(Skybox_VS_OUTPUT pdata) : SV_TARGET{
	// float3 val = pdata.position_W;   
	// float3(val.y, val.z, -val.x)
	return texSkybox.Sample(sampSkybox, mul(trans_CubeMap, float4(pdata.position_W, 0)).xyz);
}