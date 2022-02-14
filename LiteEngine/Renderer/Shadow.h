#pragma once

#include "Renderer.h"

namespace LiteEngine::Rendering {
	
	std::vector<std::tuple<bool, Rendering::RenderingScene::CameraInfo, float, float>>
		getSuggestedPointSpotLightDepthCamera(
			const Rendering::RenderingScene::CameraInfo& mainCamera,
			const Rendering::LightDesc& light,
			const std::vector<float>& zLists  // ��Ҫ���� near �� far ƽ��
		);
}