#pragma once

#include "Renderer.h"
#include "Resources.h"

namespace LiteEngine::Rendering {
	
	std::vector<std::tuple<bool, Rendering::RenderingScene::CameraInfo, float, float>>
		getSuggestedDepthCamera(
			const Rendering::RenderingScene::CameraInfo& mainCamera,
			const Rendering::LightDesc& light,
			const std::vector<float>& zLists  // 需要包含 near 和 far 平面
		);
}