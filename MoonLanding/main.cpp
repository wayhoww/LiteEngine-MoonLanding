// You just need to implement this function 
// and write the game logic.
// Simply leave the rest work to the game framework!

#include <Windows.h>
#include "LiteEngine/LiteEngine.h"

namespace le = LiteEngine;
namespace rd = LiteEngine::Rendering;
namespace io = LiteEngine::IO;
namespace sm = LiteEngine::SceneManagement;

class MoonLandingGame {

	//static constexpr float EarthRadius = 6371e3f;
	//static constexpr float MoonRadius = 1737e3f;
	//static constexpr float SumRadius = 696340e3f;
	//static constexpr float SpaceshipHeight = 10;
	//static constexpr float EarthMoonDistance = 384400e3f;
	//static constexpr float SumEarthDistance = 149597871e3f;
	//static constexpr float OrbitHeight = 1000e3f;

	static constexpr float EarthRadius = 40;
	static constexpr float MoonRadius = 17;
	static constexpr float SumRadius = 100;
	static constexpr float SpaceshipHeight = 1;
	static constexpr float EarthMoonDistance = 200;
	static constexpr float SumEarthDistance = 500;
	static constexpr float OrbitHeightEarth = 5;
	static constexpr float OrbitHeightMoon = 5;
	static constexpr float EarthRevolutionPeriod = 240; // 地球公转周期
	static constexpr float EarthRotationPeriod = 40; // 地球自转周期
	static constexpr float MoonRevolutionPeriod = 100; // 月球公转周期
	static constexpr float EarthMoonTransferTime = 20; // 地月转移周期（两倍时间）
	static constexpr float ShipRevolutionPeriodAroundEarth = 30;	// 飞船绕地球公转周期
	static constexpr float ShipRevolutionPeriodAroundMoon = 20;	// 飞船绕月球公转周期
	static constexpr float EarthAxialTilt = (float) ((23 + 26 / 60.0) / 180 * le::PI); // 黄赤交角
	static constexpr float MoonRevolutionAngle = (float)(5.0 / 180 * le::PI); // 月亮公转轨道和地球的夹角

	rd::Renderer* renderer;
	io::RenderingWindow window;
	// framerate controller 提供额外的帧率控制，也提供帧率信息
	io::FramerateController framerateController;

	std::shared_ptr<sm::Scene> scene;

	// textures
	rd::PtrShaderResourceView texSkymap;

	// mesh-related
	std::shared_ptr<sm::Object> objSum;
	std::shared_ptr<sm::Object> objMoon;
	std::shared_ptr<sm::Object> objMoonOrbit;
	std::shared_ptr<sm::Object> objMoonOrbitAngleLayer; // 处理和黄道的夹角
	std::shared_ptr<sm::Object> objEarth;
	std::shared_ptr<sm::Object> objEarthRotationAngleLayer;	// 处理黄赤交角
	std::shared_ptr<sm::Object> objShip;
	std::shared_ptr<sm::Object> objShipCameraSys;
	std::shared_ptr<sm::Object> objShipCameraSysCoordLayer;			// Earth -> Moon 转移轨道，处理具体位置
	std::shared_ptr<sm::Object> objShipCameraSysOrientationLayer;	
	std::shared_ptr<sm::Object> objShipCameraSysEarthOrbitLayer;	// 环绕地球轨道，本身会转动，所以在这个轨道里不需要动
	std::shared_ptr<sm::Object> objShipCameraSysMoonOrbitLayer;		// 环绕月球轨道，本身会转动，所以在这个轨道里不需要动
	std::shared_ptr<sm::Object> objShipCameraSysEMTransferLayer;	// Earth -> Moon 转移轨道，处理椭圆轨道的朝向和位置
	std::shared_ptr<sm::Object> objEarthMoonSys;
	std::shared_ptr<sm::Object> objEarthMoonSysOrbitLayer;
	std::shared_ptr<sm::Object> objRoot;

	// camera-related
	static constexpr float CMShipFreeFOV = le::PI / 2;
	std::shared_ptr<sm::Camera> cmShipFree;		// 相对飞船，自由移动
	std::shared_ptr<sm::Object> cmShipFreePitchLayer;
	std::shared_ptr<sm::Object> cmShipFreeYawLayer;
	std::shared_ptr<sm::Object> cmShipFreePresetLayer;
	io::HoldRotateController cmShipFreeRotate;
	io::MoveController cmShipFreeMoveX{'D', 'A'};
	io::MoveController cmShipFreeMoveY{'E', 'C'};
	io::MoveController cmShipFreeMoveZ{'W', 'S'};

	std::shared_ptr<sm::Camera> cmNorthFixed;		// 固定在太阳上方的正交相机

	// light-related
	std::shared_ptr<sm::Light> ltSum; // 位于 Root，直接控制方向

	// states
	enum class CameraSetting { ShipFree, FixedNorth };
	CameraSetting activeCamera = CameraSetting::ShipFree;

	enum class OrbitState { 
		OnTheEarth, LaunchEarth, AroundEarth, EarthToMoon, LandEarth,
		OnTheMoon,  LauhcnMoon,  AroundMoon,  MoonToEarth, LandMoon
	};
	OrbitState orbitState = OrbitState::AroundEarth;	// todo

	// commandInQueue 表示目前有待处理的命令在队列中等待执行（如开始转移）
	bool commandInQueue = false;
	OrbitState nextState;
	bool forbitCommand = false;	// 禁止任何命令

	// floating state
	double shipOnEarthRotationAngle = 0;			// 飞船绕地球旋转的角度
	double moonRevolutionOrbitRotationAngle = 0;	// 月球绕地球公转的角度
	double earthToMoonTransferStartTime = 0;		// 地球向月球转移的开始时间
	double earthToMoonRotationAngle = 0;			// 地球向月球转移的椭圆轨道角度（0-PI）
	double moonToEarthTransferStartTime = 0;		// 月球向地球转移的开始时间
	double moonToEarthRotationAngle = 0;			// 月球向地球转移的椭圆轨道角度（0-PI)
	double aroundEarthStartTime = 0;				// 飞船环绕地球转动的开始时间
	double aroundMoonStartTime = 0;					// 飞船环绕月球转动的开始时间
	double shipOnMoonRotationAngle = 0;				// 飞船绕月球旋转的角度
	double shipOnEarthRotationAngleOffset = 0;			// 地球开始转动时候的角度


	MoonLandingGame(): window(
		L"Rendering Window", 
		WS_OVERLAPPEDWINDOW ^ WS_SIZEBOX ^ WS_MAXIMIZEBOX,
		0, 0, 1500, 1500), 
		framerateController(55, io::FramerateControlling::WaitableObject)
	{
		rd::Renderer::setHandle(window.getHwnd());
		renderer = &rd::Renderer::getInstance();

		scene = std::make_shared<sm::Scene>();

		window.renderCallback = [this](auto& x, auto& y) { renderCallback(x, y); };
	}

	void loadResources() {
		objSum = io::loadDefaultResourceGLTF("Sun_10m.glb");
		objSum->multiplyScale({ SumRadius / 10, SumRadius / 10, SumRadius / 10 });

		auto objEarthLD = io::loadDefaultResourceGLTF("Earth_10m.glb");
		
		objEarthLD->multiplyScale({ EarthRadius / 10, EarthRadius / 10, EarthRadius / 10 });
		objEarthLD->rotateParentCoord(DirectX::XMQuaternionRotationAxis({1, 0, 0}, le::PI/2));
		objEarth = std::make_shared<sm::Object>("Earth");
		objEarth->children = { objEarthLD };

		auto objMoonLD = io::loadDefaultResourceGLTF("Moon_10m.glb");
		objMoonLD->multiplyScale({ -MoonRadius / 10, MoonRadius / 10, MoonRadius / 10 });
		objMoon = std::make_shared<sm::Object>("Moon");
		objMoon->children = { objMoonLD };

		objShip = io::loadDefaultResourceGLTF("Spaceship_5cm.glb");
		objShip->multiplyScale({ SpaceshipHeight / 0.05f, SpaceshipHeight / 0.05f, SpaceshipHeight / 0.05f });

		texSkymap = renderer->createCubeMapFromDDS(L"skymap.dds");
	}

	void createCameras() {
		cmShipFree = std::make_shared<sm::Camera>();
		cmShipFree->data.projectionType = rd::RenderingScene::CameraInfo::ProjectionType::PERSPECTIVE;
		cmShipFree->data.nearZ = 0.1f;
		cmShipFree->data.farZ = 1000;
		cmShipFree->data.aspectRatio = 1; // TODO
		cmShipFree->data.fieldOfViewYRadian = CMShipFreeFOV;

		cmNorthFixed = std::make_shared<sm::Camera>();
		cmNorthFixed->transT = { 0, -SumRadius * 5, 0 };
		cmNorthFixed->transS = { 1, 1, -1 };
		cmNorthFixed->transR = DirectX::XMQuaternionRotationAxis({ 1, 0, 0 }, le::PI / 2);
		cmNorthFixed->data.projectionType = rd::RenderingScene::CameraInfo::ProjectionType::PERSPECTIVE;
		cmNorthFixed->data.nearZ = SumRadius;
		cmNorthFixed->data.farZ = SumRadius * 6;
		cmNorthFixed->data.aspectRatio = 1; // TODO
		cmNorthFixed->data.fieldOfViewYRadian = CMShipFreeFOV;
		//cmNorthFixed->data.viewHeight = SumEarthDistance + SumRadius * 2 + EarthMoonDistance + MoonRadius * 3;
		//cmNorthFixed->data.viewWidth = SumEarthDistance + SumRadius * 2 + EarthMoonDistance + MoonRadius * 3;
	}

	void createLights() {
		ltSum = std::make_shared<sm::Light>(/*"SumLight"*/);
		ltSum->type = rd::LightType::LIGHT_TYPE_DIRECTIONAL;
		ltSum->intensity = { 2, 2, 2 };
	}

	DirectX::XMFLOAT3 substract(const DirectX::XMFLOAT3& lhs, const DirectX::XMFLOAT3& rhs) {
		return { lhs.x - rhs.x , lhs.y - rhs.x , lhs.z - rhs.z };
	}

	void updateLights() {
		// sum is at the origin
		auto dirShip3 = substract(objShip->getWorldPosition(), objRoot->getWorldPosition());
		auto dirShip3v = DirectX::XMVector3Normalize({ dirShip3.x, dirShip3.y, dirShip3.z });
		DirectX::XMStoreFloat3(&ltSum->direction_L, dirShip3v);
	}

	void setupScene() {

		objShipCameraSys = std::make_shared<sm::Object>("ShipCameraSystem");
		objShipCameraSysCoordLayer = std::make_shared<sm::Object>("ShipCameraSystem Coord");
		objShipCameraSysOrientationLayer = std::make_shared<sm::Object>("ShipCameraSystem Moving Dir");
		objShipCameraSysEarthOrbitLayer = std::make_shared<sm::Object>("ShipCameraSystem Earth Orbit");
		objShipCameraSysEMTransferLayer = std::make_shared<sm::Object>("Earth -> Moon Transfer Orbit");
		objEarthMoonSys = std::make_shared<sm::Object>("EarthMoonSystem");
		objEarthMoonSysOrbitLayer = std::make_shared<sm::Object>("EarthMoonSystem Orbit Layer");
		objEarthRotationAngleLayer = std::make_shared<sm::Object>("Earth Ro Angle Layer");
		objRoot = std::make_shared<sm::Object>("SceneRoot");
		objMoonOrbit = std::make_shared<sm::Object>("Moon Orbit");
		objMoonOrbitAngleLayer = std::make_shared<sm::Object>("Moon Re Angle Layer");
		objShipCameraSysMoonOrbitLayer = std::make_shared<sm::Object>("ShipCameraSystem Moon Orbit");

		scene->rootObject = objRoot;

		objRoot->children = { objEarthMoonSysOrbitLayer, ltSum, objSum, cmNorthFixed };
		{
			objEarthMoonSysOrbitLayer->children = { objEarthMoonSys };
			// 地月系
			objEarthMoonSys->children = { objEarthRotationAngleLayer, objMoonOrbitAngleLayer };
			objEarthMoonSys->moveParentCoord({ SumEarthDistance, 0, 0 });
			{
				// 地球
				objEarthRotationAngleLayer->children = { objEarth };
				objEarthRotationAngleLayer->rotateParentCoord(DirectX::XMQuaternionRotationAxis({ 1, 0, 0 }, (float)EarthAxialTilt));
			}
			{
				// 月球
				objMoonOrbitAngleLayer->transR = DirectX::XMQuaternionRotationAxis({0, 0, 1}, MoonRevolutionAngle);
				objMoonOrbitAngleLayer->children = { 
					objMoonOrbit, 
					objShipCameraSysEarthOrbitLayer,
					objShipCameraSysEMTransferLayer
				};

				{
					objMoonOrbit->children = { objMoon };
					{
						objMoon->moveParentCoord({ EarthMoonDistance, 0, 0 });
						objMoon->children.push_back(objShipCameraSysMoonOrbitLayer);
					}
				}
				{
					objShipCameraSysCoordLayer->children = { objShipCameraSysOrientationLayer }; 
					objShipCameraSysEarthOrbitLayer->children = { objShipCameraSysCoordLayer };
					objShipCameraSysOrientationLayer->children = { objShipCameraSys };
					objShipCameraSys->children = { objShip, cmShipFree };

					objShipCameraSysCoordLayer->moveParentCoord({ EarthRadius + OrbitHeightEarth, 0, 0 });
					objShipCameraSysCoordLayer->rotateParentCoord(
						DirectX::XMQuaternionMultiply(
							DirectX::XMQuaternionRotationAxis({ -1, 0, 0 }, le::PI / 2),
							DirectX::XMQuaternionRotationAxis({ 0, 0, 1 }, le::PI / 2)
						)
					);
				}
			}
		}

		
		scene->link();

		// 必须在 link 之后才能做
		cmShipFreePitchLayer = cmShipFree->insertParent();
		cmShipFreeYawLayer = cmShipFreePitchLayer->insertParent();
		cmShipFreePresetLayer = cmShipFreePitchLayer->insertParent();
		cmShipFreePresetLayer->moveParentCoord({ 0, -SpaceshipHeight, -SpaceshipHeight });
		cmShipFreePresetLayer->rotateParentCoord(DirectX::XMQuaternionRotationAxis({ -1, 0, 0 }, le::PI / 2 * 0.8));
		

		updateLights();
	}

	void switchOrbitState(OrbitState to) {
		if (orbitState == OrbitState::AroundEarth && to == OrbitState::EarthToMoon) {
			objShipCameraSysEMTransferLayer->children = { objShipCameraSysCoordLayer };
			objShipCameraSysCoordLayer->parent = objShipCameraSysEMTransferLayer;
			objShipCameraSysEarthOrbitLayer->children.clear();

			objShipCameraSysEMTransferLayer->transR = DirectX::XMQuaternionRotationAxis(
				{ 0, 1, 0 },
				float(moonRevolutionOrbitRotationAngle + 2 * le::PI / MoonRevolutionPeriod * (EarthMoonTransferTime / 2))
			);

			constexpr double R1 = EarthRadius + OrbitHeightEarth;
			constexpr double R2 = EarthMoonDistance - MoonRadius - OrbitHeightMoon;
			constexpr float offset = (float)((R1 + R2) / 2 - R1);

			objShipCameraSysEMTransferLayer->transT = { 0, 0, 0 };
			objShipCameraSysEMTransferLayer->moveLocalCoord({ offset, 0, 0 });

			earthToMoonTransferStartTime = framerateController.getLastFrameEndTime();
		} else if (orbitState == OrbitState::AroundMoon && to == OrbitState::MoonToEarth) {
			objShipCameraSysEMTransferLayer->children = { objShipCameraSysCoordLayer };
			objShipCameraSysCoordLayer->parent = objShipCameraSysEMTransferLayer;
			objShipCameraSysMoonOrbitLayer->children.clear();

			objShipCameraSysEMTransferLayer->transR = DirectX::XMQuaternionRotationAxis({ 0, 1, 0 }, moonRevolutionOrbitRotationAngle);

			constexpr double R1 = EarthRadius + OrbitHeightEarth;
			constexpr double R2 = EarthMoonDistance - MoonRadius - OrbitHeightMoon;
			constexpr float offset = (float)((R1 + R2) / 2 - R1);

			objShipCameraSysEMTransferLayer->transT = { 0, 0, 0 };
			objShipCameraSysEMTransferLayer->moveLocalCoord({ offset, 0, 0 });

			moonToEarthTransferStartTime = framerateController.getLastFrameEndTime();

			// 顺便把返回地球后的参数也设置好
			shipOnEarthRotationAngleOffset = fmod(moonRevolutionOrbitRotationAngle + le::PI, 2 * le::PI);

		} else if (orbitState == OrbitState::EarthToMoon && to == OrbitState::AroundMoon) {
			objShipCameraSysMoonOrbitLayer->children = { objShipCameraSysCoordLayer };
			objShipCameraSysCoordLayer->parent = objShipCameraSysMoonOrbitLayer;
			objShipCameraSysEMTransferLayer->children.clear();

			objShipCameraSysCoordLayer->transT = { MoonRadius + OrbitHeightMoon, 0, 0 };

			objShipCameraSysOrientationLayer->transR = DirectX::XMQuaternionMultiply(
				DirectX::XMQuaternionRotationAxis({ 0, 1, 0 }, le::PI),
				DirectX::XMQuaternionRotationAxis({ -1, 0, 0 }, le::PI)
			);

			aroundMoonStartTime = framerateController.getLastFrameEndTime();
		} else if (orbitState == OrbitState::MoonToEarth && to == OrbitState::AroundEarth) {
			objShipCameraSysEarthOrbitLayer->children = { objShipCameraSysCoordLayer };
			objShipCameraSysCoordLayer->parent = objShipCameraSysEarthOrbitLayer;
			objShipCameraSysEMTransferLayer->children.clear();

			objShipCameraSysCoordLayer->transT = { EarthRadius + OrbitHeightMoon, 0, 0 };
			objShipCameraSysOrientationLayer->transR = DirectX::XMQuaternionIdentity();
			//objShipCameraSysOrientationLayer->transR = DirectX::XMQuaternionMultiply(
			//	DirectX::XMQuaternionRotationAxis({ 0, 1, 0 }, le::PI),
			//	DirectX::XMQuaternionRotationAxis({ -1, 0, 0 }, le::PI)
			//);

			aroundEarthStartTime = framerateController.getLastFrameEndTime();
			
		} else {
			assert(false);
		}


		orbitState = to;
	}
	
	// 要求三个值都在 [0, mod) 内
	static double modulePassThrough(double val, double start, double end, double mod) {
		assert(end < mod && end >= 0 && start < mod && start >= 0 && val < mod && val >= 0);
		if (end < start) {
			end += mod;
		}
		if (val < start) {
			val += mod;
		}
		return start <= val && val <= end;
	}

	// changeState 用的变量
	double lastShipOnEarthRotationAngle = -1;
	double lastShipOnMoonRotationAngle = -1;
	void changeState() {
		char buffer[100];
		if (commandInQueue) {
			if (nextState == OrbitState::EarthToMoon) {
				if (lastShipOnEarthRotationAngle < 0) {
					lastShipOnEarthRotationAngle = shipOnEarthRotationAngle;
					// then do nothing else
				} else {
					double threshold = le::PI + moonRevolutionOrbitRotationAngle + 2 * le::PI / MoonRevolutionPeriod * (EarthMoonTransferTime / 2);
					threshold = fmod(threshold, 2 * le::PI);

					if (modulePassThrough(threshold, lastShipOnEarthRotationAngle, shipOnEarthRotationAngle, le::PI * 2)) {
						lastShipOnEarthRotationAngle = -1;
						// 开始转移，禁止任何新命令进入
						forbitCommand = true;
						commandInQueue = false;
						shipOnEarthRotationAngle = threshold;
						// state 已经改变
						this->switchOrbitState(OrbitState::EarthToMoon);
					} else {
						lastShipOnEarthRotationAngle = shipOnEarthRotationAngle;
					}
				}
			} else if (nextState == OrbitState::MoonToEarth) {
				if (lastShipOnMoonRotationAngle < 0) {
					lastShipOnMoonRotationAngle = shipOnMoonRotationAngle;
					// then do nothing else
				} else {
					double threshold = le::PI;

					if (modulePassThrough(threshold, lastShipOnMoonRotationAngle, shipOnMoonRotationAngle, le::PI * 2)) {
						lastShipOnMoonRotationAngle = -1;
						forbitCommand = true;
						commandInQueue = false;
						this->switchOrbitState(OrbitState::MoonToEarth);
					} else {
						lastShipOnMoonRotationAngle = shipOnMoonRotationAngle;
					}

				}
			}
		} else {
			if (orbitState == OrbitState::EarthToMoon) {
				// 处理转移结束
				if (earthToMoonRotationAngle > le::PI) {
					this->switchOrbitState(OrbitState::AroundMoon);
					forbitCommand = false;
				}
			} else if (orbitState == OrbitState::MoonToEarth) {
				if (moonToEarthRotationAngle > le::PI) {
					this->switchOrbitState(OrbitState::AroundEarth);
					forbitCommand = false;
				}
			}
		}


	}

	void moveCameraShipFree(const std::vector<std::tuple<UINT, WPARAM, LPARAM>>& events) {
		if (this->activeCamera != CameraSetting::ShipFree) return;

		auto lastDuration = (float)framerateController.getLastFrameDuration();

		cmShipFreeRotate.receiveEvent(events, lastDuration);
		cmShipFreeMoveX.receiveEvent(events, lastDuration);
		cmShipFreeMoveY.receiveEvent(events, lastDuration);
		cmShipFreeMoveZ.receiveEvent(events, lastDuration);
		
		auto [rx, ry, rz] = cmShipFreeRotate.getXYZ();
		cmShipFreeYawLayer->moveLocalCoord({ 
			(float)cmShipFreeMoveX.popValue(), 
			(float)cmShipFreeMoveY.popValue(),
			(float)cmShipFreeMoveZ.popValue(),
		});
		cmShipFreeYawLayer->transR = DirectX::XMQuaternionRotationAxis({ 0, -1, 0 }, (float)rx);
		cmShipFreePitchLayer->transR = DirectX::XMQuaternionRotationAxis({ -1, 0, 0 }, (float)ry);
		cmShipFree->data.fieldOfViewYRadian = (float) std::min<double>(le::PI * 0.8, CMShipFreeFOV * (0.5 + (rz + 1) / 2));
	}

	void updateWindowTitle() {
		wchar_t titleBuffer[50];
		swprintf_s(titleBuffer, L"%5.0lf", framerateController.getAverageFPS());
		SetWindowText(window.getHwnd(), titleBuffer);
	}

	static double circleToAngle(double x, double y) {
		auto len = sqrt(x * x + y * y);
		x /= len;
		y /= len;

		double theta = acos(x);	// (theta \in [0, PI])
		if (y < 0) {
			theta = le::PI * 2 - theta;
		}
		return theta;
	}

	void updateSpaceshipAnimation() {
		// 飞行器绕地球的转动
		// 距离对应角度
		
		switch (orbitState) {
		case OrbitState::AroundEarth:
		{


			double time = framerateController.getLastFrameEndTime() - aroundEarthStartTime;
			shipOnEarthRotationAngle = le::PI * 2 * (fmod(time, ShipRevolutionPeriodAroundEarth) / ShipRevolutionPeriodAroundEarth);
			shipOnEarthRotationAngle += shipOnEarthRotationAngleOffset;
			shipOnEarthRotationAngle = fmod(shipOnEarthRotationAngle, le::PI * 2);

		/*	char buffer[100];
			sprintf_s(buffer, "ship = %.2lf  moon = %.2lf\n", shipOnEarthRotationAngle * 180 / le::PI, moonRevolutionOrbitRotationAngle * 180 / le::PI);
			OutputDebugStringA(buffer);
		*/
			objShipCameraSysEarthOrbitLayer->transR = DirectX::XMQuaternionRotationAxis({ 0, 1, 0 }, (float)shipOnEarthRotationAngle);
		}
		break;
		case OrbitState::EarthToMoon:
		{
			double time = framerateController.getLastFrameEndTime() - earthToMoonTransferStartTime;
			earthToMoonRotationAngle = le::PI * 2 * (fmod(time, EarthMoonTransferTime) / EarthMoonTransferTime);

			constexpr double R1 = EarthRadius + OrbitHeightEarth;
			constexpr double R2 = EarthMoonDistance - MoonRadius - OrbitHeightMoon;
			constexpr double c = (R2 - R1) / 2;
			constexpr double a = (R2 + R1) / 2;
			const double b = sqrt(a * a - c * c);

			float x = float(a * cos(earthToMoonRotationAngle));
			float z = float(b * sin(earthToMoonRotationAngle));

			objShipCameraSysCoordLayer->transT = { -x, 0, z };
			objShipCameraSysOrientationLayer->transR = DirectX::XMQuaternionMultiply(
				DirectX::XMQuaternionRotationAxis({ 0, 1, 0 }, earthToMoonRotationAngle),
				DirectX::XMQuaternionRotationAxis({ -1, 0, 0 }, circleToAngle(-x, z))
			);
		}
		break;
		case OrbitState::AroundMoon:
		{
			double time = framerateController.getLastFrameEndTime() - aroundMoonStartTime;
			shipOnMoonRotationAngle = le::PI + le::PI * 2 * (fmod(time, ShipRevolutionPeriodAroundMoon) / ShipRevolutionPeriodAroundMoon);
			shipOnMoonRotationAngle = fmod(shipOnMoonRotationAngle, 2 * le::PI);

			objShipCameraSysMoonOrbitLayer->transR = DirectX::XMQuaternionRotationAxis({ 0, -1, 0 }, shipOnMoonRotationAngle);
		}
		break;
		case OrbitState::MoonToEarth:
		{
			// 走地月转移轨道的 [PI, 2PI]

			double time = framerateController.getLastFrameEndTime() - moonToEarthTransferStartTime;
			moonToEarthRotationAngle = le::PI * 2 * (fmod(time, EarthMoonTransferTime) / EarthMoonTransferTime);

			constexpr double R1 = EarthRadius + OrbitHeightEarth;
			constexpr double R2 = EarthMoonDistance - MoonRadius - OrbitHeightMoon;
			constexpr double c = (R2 - R1) / 2;
			constexpr double a = (R2 + R1) / 2;
			const double b = sqrt(a * a - c * c);

			float x = float(a * cos(moonToEarthRotationAngle + le::PI));
			float z = float(b * sin(moonToEarthRotationAngle + le::PI));

			objShipCameraSysCoordLayer->transT = { -x, 0, z };
			objShipCameraSysOrientationLayer->transR = DirectX::XMQuaternionMultiply(
				DirectX::XMQuaternionRotationAxis({ 0, 1, 0 }, moonToEarthRotationAngle + le::PI),
				DirectX::XMQuaternionRotationAxis({ -1, 0, 0 }, circleToAngle(-x, z))
			);
		}
		break;
		default:
			assert(false);
		}
	}

	void updateEarthRevolutionAnimation() {
		// 地球公转
		double time = framerateController.getLastFrameEndTime();
		double rotationAngle = le::PI * 2 * (fmod(time, EarthRevolutionPeriod) / EarthRevolutionPeriod);
		objEarthMoonSysOrbitLayer->transR = DirectX::XMQuaternionRotationAxis({0, 1, 0}, (float)rotationAngle);
	}

	void updateEarthRotationAnimation() {
		double time = framerateController.getLastFrameEndTime();
		double rotationAngle = le::PI * 2 * (fmod(time, EarthRotationPeriod) / EarthRotationPeriod);
		objEarth->transR = DirectX::XMQuaternionRotationAxis({0, 1, 0}, (float)rotationAngle);
	}
	
	void updateMoonRevolutionAnimation() {
		double time = framerateController.getLastFrameEndTime();
		double rotationAngle = le::PI * 2 * (fmod(time, MoonRevolutionPeriod) / MoonRevolutionPeriod);
		moonRevolutionOrbitRotationAngle = rotationAngle;
		objMoonOrbit->transR = DirectX::XMQuaternionRotationAxis({0, 1, 0}, (float)rotationAngle);
	}

	void updateAnimation() {
		updateSpaceshipAnimation();
	//	updateEarthRevolutionAnimation();
	//	updateEarthRotationAnimation();
		updateMoonRevolutionAnimation();
	}

	void processEvents(const std::vector<std::tuple<UINT, WPARAM, LPARAM>>& events) {
		constexpr float NORTH_FIX_SHIP_SCALE = 20;
		for (auto [msg, wparam, lparam]: events) {
			if (msg == WM_KEYDOWN && wparam == VK_SPACE) {
			//	objRoot->dump();
				this->switchOrbitState(OrbitState::EarthToMoon);
			} else if (msg == WM_KEYDOWN && (wparam == le::getKeyCode<'3'>() || wparam == VK_NUMPAD3)) {
				if (activeCamera == CameraSetting::FixedNorth) continue;
				scene->activeCamera = this->cmNorthFixed;
				activeCamera = CameraSetting::FixedNorth;
				objShip->multiplyScale({ NORTH_FIX_SHIP_SCALE, NORTH_FIX_SHIP_SCALE, NORTH_FIX_SHIP_SCALE });
			} else if (msg == WM_KEYDOWN && (wparam == le::getKeyCode<'4'>() || wparam == VK_NUMPAD4)) {
				if (activeCamera == CameraSetting::ShipFree) continue;
				scene->activeCamera = this->cmShipFree;
				activeCamera = CameraSetting::ShipFree;
				objShip->multiplyScale({ 1/NORTH_FIX_SHIP_SCALE, 1/NORTH_FIX_SHIP_SCALE, 1/NORTH_FIX_SHIP_SCALE });
			} 


			// left -> Earth; right -> Moon
			if (msg == WM_KEYDOWN && (wparam == VK_LEFT || wparam == VK_RIGHT) && !forbitCommand) {
				switch (orbitState) {
				case OrbitState::AroundEarth:
					if (wparam == VK_LEFT) {
						// land on the earth
					} else {
						// earth -> moon
						commandInQueue = true;
						nextState = OrbitState::EarthToMoon;
					}
					break;
				case OrbitState::AroundMoon:
					if (wparam == VK_LEFT) {
						// moon -> earth
						commandInQueue = true;
						nextState = OrbitState::MoonToEarth;
					} else {
						// land on the moon
					}
				}
			}
		}
	}

	void moveCameraNorthFixed() {
		if (this->activeCamera == CameraSetting::FixedNorth) {
			auto pos = this->objEarthMoonSys->getPositionInAncestor(this->objRoot);
			cmNorthFixed->transT.x = pos.x / 2;
			cmNorthFixed->transT.z = pos.z / 2;
		}
	}

	void renderCallback(const io::RenderingWindow&, const std::vector<std::tuple<UINT, WPARAM, LPARAM>>& events) {
		// 额外调用 begin 无害
		framerateController.begin();

		processEvents(events);

		updateWindowTitle();

		changeState();
		updateAnimation();
		moveCameraShipFree(events);
		moveCameraNorthFixed();
		updateLights();


		auto rScene = scene->getRenderingScene();

		renderer->beginRendering();
		renderer->renderScene(rScene, true);

		renderer->renderSkybox(this->texSkymap, rScene->camera, DirectX::XMMatrixIdentity());

		renderer->swap();
		framerateController.wait();
	}

	void start() {
		window.show();
		window.runRenderingLoop();
	}


public:
	static MoonLandingGame& getInstance() {
		static MoonLandingGame game;
		return game;
	}
	MoonLandingGame(const MoonLandingGame&) = delete;
	void operator=(const MoonLandingGame&) = delete;
	
	void lanuch() {
		loadResources();
		createCameras();
		createLights();
		setupScene();
		scene->activeCamera = cmShipFree;
		objRoot->dump();
		this->start();
	}
};

int WINAPI wWinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance, 
	_In_ PWSTR pCmdLine, 
	_In_ int nCmdShow
) {
	SetProcessDPIAware();

	auto& game = MoonLandingGame::getInstance();
	game.lanuch();
}
