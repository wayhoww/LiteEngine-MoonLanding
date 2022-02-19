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
	std::pair<bool, Rendering::RenderingScene::CameraInfo> getPointLightDepthMapMatrix(
		DirectX::XMVECTOR lightCoord3,	// w == 1
		DirectX::XMVECTOR up,			// w == 0
		std::array<DirectX::XMVECTOR, 8> rangeWorld	// 需要照亮的范围, vector3, w == 1
	) {
		// 求出目标范围“中心点”
		DirectX::XMVECTOR rangeCoordSum = {};
		for (auto& pos : rangeWorld) {
			rangeCoordSum = DirectX::XMVectorAdd(pos, rangeCoordSum);
		}
		DirectX::XMVECTOR worldCenter = DirectX::XMVectorScale(rangeCoordSum, 1.f / 8);

		// 使得深度相机朝向中心点，并且指定 top
		// corner case: transZ == 0
		// corner case：top 和 transZ 平行，那么略微偏移 top
		auto transZ = DirectX::XMVector3Normalize(DirectX::XMVectorSubtract(worldCenter, lightCoord3));
		DirectX::XMFLOAT4 transZ4;
		DirectX::XMStoreFloat4(&transZ4, transZ);
		transZ4.w = 0;
		transZ = DirectX::XMLoadFloat4(&transZ4);
		auto transX = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(transZ, up));
		auto transY = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(transX, transZ));
		auto lightRotate = DirectX::XMMATRIX(transX, transY, transZ, DirectX::XMVECTOR{ 0, 0, 0, 1 });

		auto lightTransform = DirectX::XMMatrixMultiply(
			// in: 3d vector offset
			lightRotate, //DirectX::XMMatrixLookAtLH(lightCoord3, worldCenter, up),
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
		constexpr float FOV_THRESHOLD = (float)(170.f / 180.f * PI);
		if (!std::isfinite(info.fieldOfViewYRadian) ||
			info.fieldOfViewYRadian > FOV_THRESHOLD ||
			info.fieldOfViewYRadian <= 0
			) {
			return { false, info };
		}

		return { true, info };
	}


	Rendering::RenderingScene::CameraInfo getDirectionalLightDepthMapMatrix(
		DirectX::XMVECTOR lightDirection,	// w == 1
		DirectX::XMVECTOR up,			// w == 0
		std::array<DirectX::XMVECTOR, 8> rangeWorld	// 需要照亮的范围, vector3, w == 1
	) {
		//// 求出目标范围“中心点”
		//DirectX::XMVECTOR rangeCoordSum = {};
		//for (auto& pos : rangeWorld) {
		//	rangeCoordSum = DirectX::XMVectorAdd(pos, rangeCoordSum);
		//}
		//DirectX::XMVECTOR worldCenter = DirectX::XMVectorScale(rangeCoordSum, 1.f / 8);

		// 相机朝向和光线朝向一致，并保持 up
		auto transZ = DirectX::XMVector3Normalize(lightDirection);
		DirectX::XMFLOAT4 transZ4;
		DirectX::XMStoreFloat4(&transZ4, transZ);
		transZ4.w = 0;
		transZ = DirectX::XMLoadFloat4(&transZ4);
		auto transX = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(transZ, up));
		auto transY = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(transX, transZ));
		auto lightRotate = DirectX::XMMATRIX(transX, transY, transZ, DirectX::XMVECTOR{ 0, 0, 0, 1 });

		auto detLightRotate = DirectX::XMMatrixDeterminant(lightRotate);
		auto lightRotateInv = DirectX::XMMatrixInverse(&detLightRotate, lightRotate);

		// 将点投影到光源的 view space，并求出 AABB
		std::array<DirectX::XMFLOAT3, 8> range;
		std::transform(rangeWorld.begin(), rangeWorld.end(), range.begin(), [&](auto val) {
			auto coord = DirectX::XMVector3TransformCoord(val, lightRotateInv);
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

		// 移动，使得范围合适
		float offset = minimums.z - (maximums.z - minimums.z) * 0.01f;

		auto lightTransform = DirectX::XMMatrixMultiply(
			DirectX::XMMatrixTranslation(0, 0, offset),
			lightRotate
		);

		auto transformInv = DirectX::XMMatrixMultiply(
			lightRotateInv,
			DirectX::XMMatrixTranslation(0, 0, -offset)
		);

		info.trans_W2V = transformInv;
		info.projectionType = Rendering::RenderingScene::CameraInfo::ProjectionType::ORTHOGRAPHICS; 
		info.nearZ = minimums.z - offset;
		info.farZ = maximums.z - offset;
		info.viewWidth = info.aspectRatio = 2 * std::max(abs(maximums.x), abs(minimums.x));
		info.viewHeight = info.aspectRatio = 2 * std::max(abs(maximums.y), abs(minimums.y));
		

		return info;
	}



	std::vector<std::tuple<bool, Rendering::RenderingScene::CameraInfo, float, float>> 
	getSuggestedDepthCamera(
		const Rendering::RenderingScene::CameraInfo& mainCamera,
		const Rendering::LightDesc& light,
		const std::vector<float>& zLists  // 需要包含 near 和 far 平面
	) {
		auto trans_W2V_det = DirectX::XMMatrixDeterminant(mainCamera.trans_W2V);
		auto trans_V2W = DirectX::XMMatrixInverse(&trans_W2V_det, mainCamera.trans_W2V);

		auto corners = getConeCutCorners(
			trans_V2W, 
			mainCamera.aspectRatio, 
			mainCamera.fieldOfViewYRadian, 
			zLists
		);

		if (light.type == Rendering::LightType::LIGHT_TYPE_DIRECTIONAL) {
			DirectX::XMVECTOR upDir = DirectX::XMVector3TransformNormal({0, 1, 0, 0}, trans_V2W);
			std::vector<std::tuple<bool, Rendering::RenderingScene::CameraInfo, float, float>> out;
			for (size_t i = 1; i < corners.size(); i++) {
				std::array<DirectX::XMVECTOR, 8> arr;
				std::copy(corners[i - 1].begin(), corners[i - 1].end(), arr.begin());
				std::copy(corners[i].begin(), corners[i].end(), arr.begin() + 4);

				DirectX::XMVECTOR lightDirection = DirectX::XMLoadFloat3(&light.direction_W);

				auto camera = getDirectionalLightDepthMapMatrix(lightDirection, upDir, arr);
				out.push_back({true, camera, zLists[i - 1], zLists[i] });
			}

			return out;

		} else {
			DirectX::XMVECTOR upDir = DirectX::XMVector3TransformNormal({0, 1, 0, 0}, trans_V2W);
			std::vector<std::tuple<bool, Rendering::RenderingScene::CameraInfo, float, float>> out;
			for (size_t i = 1; i < corners.size(); i++) {
				std::array<DirectX::XMVECTOR, 8> arr;
				std::copy(corners[i - 1].begin(), corners[i - 1].end(), arr.begin());
				std::copy(corners[i].begin(), corners[i].end(), arr.begin() + 4);

				DirectX::XMVECTOR lightCoords = DirectX::XMLoadFloat3(&light.position_W);

				auto [feasible, camera] = getPointLightDepthMapMatrix(lightCoords, upDir, arr);
				out.push_back({feasible, camera, zLists[i - 1], zLists[i] });
			}

			return out;
		}
	}
}