#include "Shadow.h"
#include "../Utilities/Utilities.h"

#include <array>
#include <algorithm>



namespace LiteEngine::Rendering {

	// vec3. w 必定是 1
	std::vector<std::array<DirectX::XMVECTOR, 4>> getConeCutCorners(
		DirectX::XMMATRIX transform, // trans_V2W
		float aspectRatio,
		float fieldOfViewY,
		const std::vector<float> zList
	) {
		std::vector<std::array<DirectX::XMVECTOR, 4>> out;

		auto hUnit = tanf(fieldOfViewY / 2);
		auto wUnit = hUnit * aspectRatio;

		for (auto z : zList) {
			std::array<DirectX::XMVECTOR, 4> localCorners = {
				DirectX::XMVECTOR{wUnit * z * -1, wUnit * z * +1, z, 1},
				DirectX::XMVECTOR{wUnit * z * -1, wUnit * z * -1, z, 1},
				DirectX::XMVECTOR{wUnit * z * +1, wUnit * z * +1, z, 1},
				DirectX::XMVECTOR{wUnit * z * +1, wUnit * z * -1, z, 1}
			};

			std::array<DirectX::XMVECTOR, 4> worldCorners;

			std::transform(localCorners.begin(), localCorners.end(), worldCorners.begin(), [&](auto val) {
				auto world = DirectX::XMVector3TransformCoord(val, transform);
				return world;
			});

			out.push_back(worldCorners);
		}
		return out;
	}

	// spot light 也一样
	// suitable, W2V, V2W(transform)
	std::pair<bool, Rendering::RenderingScene::CameraInfo> pointLightDepthMapMatrix(
		DirectX::XMVECTOR lightCoord3,	// w == 1
		DirectX::XMVECTOR up,			// w == 0
		std::array<DirectX::XMVECTOR, 8> rangeWorld	// 需要照亮的范围, vector3, w == 1
	) {
		// 求出目标范围“中心点”
		DirectX::XMVECTOR rangeCoordSum = {};
		for (auto& pos : rangeWorld) {
			DirectX::XMVectorAdd(pos, rangeCoordSum);
		}
		DirectX::XMVECTOR worldCenter = DirectX::XMVectorScale(rangeCoordSum, 1.f / 8);
		
		// 使得深度相机朝向中心点，并且指定 top
		// corner case：top 和 transZ 平行，那么略微偏移 top
		/*auto transZ = DirectX::XMVector3Normalize(DirectX::XMVectorSubtract(worldCenter, lightCoord3));
		auto transX = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(transZ, up));
		auto transY = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(transX, transZ));
		auto lightRotate = DirectX::XMMATRIX(transX, transY, transZ, DirectX::XMVECTOR{ 0, 0, 0, 1 });
		*/
		auto lightTransform = DirectX::XMMatrixMultiply(
			// in: 3d vector offset
			DirectX::XMMatrixLookAtLH(lightCoord3, worldCenter, up),
			DirectX::XMMatrixTranslationFromVector(lightCoord3)
		);
		auto det = DirectX::XMMatrixDeterminant(lightTransform);
		auto transformInv = DirectX::XMMatrixInverse(&det, lightTransform);

		// 将点投影到光源的 view space，并求出 AABB
		std::array<DirectX::XMFLOAT3, 8> range;
		std::transform(rangeWorld.begin(), rangeWorld.end(), range.begin(), [&](auto val) {
			auto coord = DirectX::XMVector3TransformCoord(val, transformInv);
			DirectX::XMFLOAT3 out;
			DirectX::XMStoreFloat3(&out, coord);
			return out;
		});

		DirectX::XMFLOAT3 maximums = range[0];
		DirectX::XMFLOAT3 minimums = range[0];

		for (auto it = range.begin() + 1; it != range.end(); it++) {
			auto& val = *it;
			
			if (val.x > maximums.x) maximums.x = val.x;
			if (val.y > maximums.y) maximums.y = val.y;
			if (val.z > maximums.z) maximums.z = val.z;

			if (val.x < minimums.x) minimums.x = val.x;
			if (val.y < minimums.y) minimums.y = val.y;
			if (val.z < minimums.z) minimums.z = val.z;
		}

		Rendering::RenderingScene::CameraInfo info;
		// 不合适
		if (minimums.z <= 0) {
			return { false, info };
		}

		info.trans_W2V = transformInv;
		info.projectionType = Rendering::RenderingScene::CameraInfo::ProjectionType::PERSPECTIVE; 
		info.nearZ = minimums.z;
		info.farZ = maximums.z;
		info.aspectRatio = 
			std::max(abs(maximums.x), abs(minimums.x)) / 
			std::max(abs(maximums.y), abs(minimums.y));
		info.fieldOfViewYRadian = 2 * atanf(std::max(abs(maximums.y), abs(minimums.y)) / minimums.z);
		
		// 不合适
		constexpr float FOV_THRESHOLD = 140.f / 180.f * PI;
		if (!std::isfinite(info.fieldOfViewYRadian) ||
			info.fieldOfViewYRadian > FOV_THRESHOLD ||
			info.fieldOfViewYRadian <= 0
		) {
			return { false, info };
		}
		
		return { true, info };
	}

	std::vector<std::tuple<bool, Rendering::RenderingScene::CameraInfo, float, float>> 
	getSuggestedPointSpotLightDepthCamera(
		const Rendering::RenderingScene::CameraInfo& mainCamera,
		const Rendering::LightDesc& light,
		const std::vector<float>& zLists  // 需要包含 near 和 far 平面
	) {
		if (light.type != Rendering::LightType::LIGHT_TYPE_POINT 
			&& light.type != Rendering::LightType::LIGHT_TYPE_SPOT) {
			throw std::exception("unexpected light type");
		}

		auto trans_W2V_det = DirectX::XMMatrixDeterminant(mainCamera.trans_W2V);
		auto trans_V2W = DirectX::XMMatrixInverse(&trans_W2V_det, mainCamera.trans_W2V);

		auto corners = getConeCutCorners(
			trans_V2W, 
			mainCamera.aspectRatio, 
			mainCamera.fieldOfViewYRadian, 
			zLists
		);

		DirectX::XMVECTOR upDir = DirectX::XMVector3TransformNormal({0, 1, 0, 0}, trans_V2W);
		std::vector<std::tuple<bool, Rendering::RenderingScene::CameraInfo, float, float>> out;
		for (size_t i = 1; i < corners.size(); i++) {
			std::array<DirectX::XMVECTOR, 8> arr;
			std::copy(corners[i - 1].begin(), corners[i - 1].end(), arr.begin());
			std::copy(corners[i].begin(), corners[i].end(), arr.begin() + 4);

			DirectX::XMVECTOR lightCoords = DirectX::XMLoadFloat3(&light.position_W);
		
			auto [feasible, camera] = pointLightDepthMapMatrix(lightCoords, upDir, arr);
			out.push_back({feasible, camera, zLists[i - 1], zLists[i] });
		}

		return out;
	}
}