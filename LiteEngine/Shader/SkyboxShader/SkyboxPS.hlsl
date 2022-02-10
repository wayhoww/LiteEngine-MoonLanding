#include "SkyboxDefs.hlsli"
#include "../Definitions.hlsli"
#include "../FixedPSConstants.hlsli"

cbuffer FixedPerobjectPSConstants : register(REGISTER_PS_MATERIAL) {
	// 用于做左右手系交换、+Z up 转 +Y up 等
	// 比如，blender 建模软件使用的是 +Z up 右手系坐标
	// 而 DirectX （的 TextureCube 的采样函数）用的是 +Y up 的左手系坐标
	// 使用 [x', y', z'] 矩阵可以进行坐标变换 （x' 是建模软件 x轴在 DirectX 坐标中的表示）

	// 旋转的星空之类的效果也可以在这里做

	// 这个矩阵只应该做旋转和缩放

	matrix trans_CubeMap;
};

TextureCube texSkybox: register(t0);
sampler sampSkybox: register(s0);

float4 main(Skybox_VS_OUTPUT pdata) : SV_TARGET{
	// float3 val = pdata.position_W;   
	// float3(val.y, val.z, -val.x)
	return texSkybox.Sample(sampSkybox, mul(trans_CubeMap, float4(pdata.position_W, 0)).xyz);
}