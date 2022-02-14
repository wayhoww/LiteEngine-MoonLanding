#pragma once

#include "Renderer.h"

namespace LiteEngine::Rendering {
	
	std::vector<std::tuple<bool, Rendering::RenderingScene::CameraInfo, float, float>>
		getSuggestedPointSpotLightDepthCamera(
			const Rendering::RenderingScene::CameraInfo& mainCamera,
			const Rendering::LightDesc& light,
			const std::vector<float>& zLists  // 需要包含 near 和 far 平面
		);
}