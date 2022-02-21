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
	static constexpr float EarthRadius = 40;
	static constexpr float MoonRadius = 17;
	static constexpr float SumRadius = 100;
	static constexpr float SpaceshipHeight = 1;
	static constexpr float EarthMoonDistance = 200;
	static constexpr float SumEarthDistance = 500;
	static constexpr float OrbitHeightEarth = 5;
	static constexpr float OrbitHeightMoon = 5;
	static constexpr float EarthRevolutionPeriod = 240; // ����ת����
	static constexpr float EarthRotationPeriod = 40; // ������ת����
	static constexpr float MoonRevolutionPeriod = 100; // ����ת����
	static constexpr float EarthMoonTransferTime = 20; // ����ת�����ڣ�����ʱ�䣩
	static constexpr float ShipRevolutionPeriodAroundEarth = 15;	// �ɴ��Ƶ���ת����
	static constexpr float ShipRevolutionPeriodAroundMoon = 10;	// �ɴ�������ת����
	static constexpr float EarthAxialTilt = (float) ((23 + 26 / 60.0) / 180 * le::PI); // �Ƴཻ��
	static constexpr float MoonRevolutionAngle = (float)(5.0 / 180 * le::PI); // ������ת����͵���ļн�
	static constexpr float LaunchFromEarthTime = 5;
	static constexpr float LandToEarthTime = 5;
	static constexpr float LaunchFromMoonTime = 5;
	static constexpr float LandToMoonTime = 5;
	static constexpr float EarthLaunchOrbitRadian = le::PI * 90 / 180; // �ڵ����Ϸ��䵽���ľ���
	static constexpr float EarthBaseRz = float(-le::PI * 40 / 180);
	static constexpr float EarthBaseRy = float(-le::PI * 85 / 180 + le::PI * 2);
	static constexpr float LandEarthStartRy = float(-le::PI * 85 / 180 + le::PI / 2);

	rd::Renderer* renderer;
	io::RenderingWindow window;
	// framerate controller �ṩ�����֡�ʿ��ƣ�Ҳ�ṩ֡����Ϣ
	io::FramerateController framerateController;

	std::shared_ptr<sm::Scene> scene;
	float aspectRatio = 1;

	// textures
	rd::PtrShaderResourceView texSkymap;

	// mesh-related
	std::shared_ptr<sm::Object> objSum;
	std::shared_ptr<sm::Object> objMoon;
	std::shared_ptr<sm::Object> objMoonOrbit;
	std::shared_ptr<sm::Object> objMoonOrbitAngleLayer; // ����ͻƵ��ļн�
	std::shared_ptr<sm::Object> objEarth;
	std::shared_ptr<sm::Object> objEarthShipPlacement;	// �ڵ����ϵ�����
	std::shared_ptr<sm::Object> objEarthRotationAngleLayer;	// ����Ƴཻ��
	std::shared_ptr<sm::Object> objShip;
	std::shared_ptr<sm::Object> objShipCameraSys;
	std::shared_ptr<sm::Object> objShipCameraSysCoordLayer;			// Earth -> Moon ת�ƹ�����������λ��
	std::shared_ptr<sm::Object> objShipCameraSysOrientationLayer;	
	std::shared_ptr<sm::Object> objShipCameraSysEarthOrbitLayer;	// ���Ƶ������������ת�����������������ﲻ��Ҫ��
	std::shared_ptr<sm::Object> objShipCameraSysMoonOrbitLayer;		// �����������������ת�����������������ﲻ��Ҫ��
	std::shared_ptr<sm::Object> objShipCameraSysEMTransferLayer;	// Earth -> Moon ת�ƹ����������Բ����ĳ����λ��
	std::shared_ptr<sm::Object> objEarthMoonSys;
	std::shared_ptr<sm::Object> objEarthMoonSysOrbitLayer;
	std::shared_ptr<sm::Object> objRoot;
	std::shared_ptr<sm::Object> objShipCenter;

	// camera-related
	static constexpr float CMShipFreeFOV = le::PI / 2;
	std::shared_ptr<sm::Camera> cmShipFree;		// ��һ�˳����
	std::shared_ptr<sm::Object> cmShipFreePitchLayer;
	std::shared_ptr<sm::Object> cmShipFreeYawLayer;
	std::shared_ptr<sm::Object> cmShipFreePresetLayer;
	io::HoldRotateController cmShipFreeRotate;

	std::shared_ptr<sm::Camera> cmShipThirdPerson;		// �����˳����
	std::shared_ptr<sm::Object> cmShipThirdInnerOrientationPerson;		// �����˳����
	std::shared_ptr<sm::Object> cmShipThirdPersonOrientationLayer;
	std::shared_ptr<sm::Object> cmShipThirdPersonCoordLayer;
	io::HoldRotateController cmShipThirdPersonRotate;

	std::shared_ptr<sm::Camera> cmNorthFixed;		// �̶���̫���Ϸ����������

	// light-related
	std::shared_ptr<sm::Light> ltSum; // λ�� Root��ֱ�ӿ��Ʒ���

	// states
	enum class CameraSetting { FirstPerson, ThirdPerson, North };
	CameraSetting activeCamera = CameraSetting::FirstPerson;

	enum class OrbitState { 
		OnTheEarth, LaunchEarth, AroundEarth, EarthToMoon, LandEarth,
		OnTheMoon,  LaunchMoon,  AroundMoon,  MoonToEarth, LandMoon
	};
	OrbitState orbitState = OrbitState::OnTheEarth;	// todo

	// commandInQueue ��ʾĿǰ�д�����������ڶ����еȴ�ִ�У��翪ʼת�ƣ�
	bool commandInQueue = false;
	OrbitState nextState;
	bool forbitCommand = false;	// ��ֹ�κ�����

	// floating state
	double shipOnEarthRotationAngle = 0;			// �ɴ��Ƶ�����ת�ĽǶ�
	double moonRevolutionOrbitRotationAngle = 0;	// �����Ƶ���ת�ĽǶ�
	double earthToMoonTransferStartTime = 0;		// ����������ת�ƵĿ�ʼʱ��
	double earthToMoonRotationAngle = 0;			// ����������ת�Ƶ���Բ����Ƕȣ�0-PI��
	double moonToEarthTransferStartTime = 0;		// ���������ת�ƵĿ�ʼʱ��
	double moonToEarthRotationAngle = 0;			// ���������ת�Ƶ���Բ����Ƕȣ�0-PI)
	double aroundEarthStartTime = 0;				// �ɴ����Ƶ���ת���Ŀ�ʼʱ��
	double aroundMoonStartTime = 0;					// �ɴ���������ת���Ŀ�ʼʱ��
	double shipOnMoonRotationAngle = 0;				// �ɴ���������ת�ĽǶ�
	double shipOnEarthRotationAngleOffset = 0;			// ����ʼת��ʱ��ĽǶ�
	double earthRotationAngle = 0;					// ������ת�Ƕ�

	std::tuple<double, double, double> rCoordLaunchFromEarthFrom;
	std::tuple<double, double, double> rCoordLaunchFromEarthTo;
	double launchFromEarthProgress = 0;
	double launchFromEarthStartTime = 0;

	std::tuple<double, double, double> rCoordLandToEarthTo;
	std::tuple<double, double, double> rCoordLandToEarthFrom;
	double landToEarthProgress = 0;
	double landEarthStartTime = 0;
	double landEarthStartRz = 0;

	double landOnTheMoonStartTime = 0;
	double landOnTheMoonProgress = 0;
	double landOnTheMoonRadianOffset = 0;
	double onTheMoonRadian = 0;

	double launchFromMoonStartTime = 0;
	double launchFromMoonProgress = 0;

	double shipOnMoonRotationAngleOffset = 0;

	// 1 ��ʾ����״̬��0 ��ʾ�ڵ��档
	// �ǶȲ�ͬ
	double cameraFlyingMode = 0;


	// ����ָ�ϣ�
	bool leftKeyValid = false;
	bool rightKeyValid = false;
	OrbitState leftKeyOp;
	OrbitState rightKeyOp;

	MoonLandingGame(): window(
		L"Rendering Window", 
		WS_OVERLAPPEDWINDOW ^ WS_SIZEBOX ^ WS_MAXIMIZEBOX,
		0, 0, 1800, 1000), 
		framerateController(60, io::FramerateControlling::WaitableObject)
	{
		rd::Renderer::setHandle(window.getHwnd());
		renderer = &rd::Renderer::getInstance();

		scene = std::make_shared<sm::Scene>();

		cmShipThirdPersonRotate.accZ = -1.5;
		cmShipThirdPersonRotate.zMin = -15;
		cmShipThirdPersonRotate.zMax = -1.5;

		auto [w, h] = window.getSize();
		this->aspectRatio = 1.f * w / h;


		window.renderCallback = [this](auto& x, auto& y) { renderCallback(x, y); };
	}

	static DirectX::XMVECTOR rotateAxisToQuat(float ry, float rz) {
		return DirectX::XMQuaternionMultiply(
			DirectX::XMQuaternionRotationAxis({ 0, 0, 1 }, rz),
			DirectX::XMQuaternionRotationAxis({ 0, 1, 0 }, -ry)
		);
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
		objMoonLD->multiplyScale({ MoonRadius / 10, MoonRadius / 10, MoonRadius / 10 });
		objMoonLD->rotateParentCoord(DirectX::XMQuaternionRotationAxis({ 0, 1, 0 }, le::PI));
		objMoon = std::make_shared<sm::Object>("Moon");
		objMoon->children = { objMoonLD };

		auto objShipLD = io::loadDefaultResourceGLTF("Spaceship_5cm.glb");
		objShipLD->multiplyScale({ SpaceshipHeight / 0.05f, SpaceshipHeight / 0.05f, SpaceshipHeight / 0.05f });
		objShip = std::make_shared<sm::Object>("Ship");
		objShip->children = { objShipLD };
		
		texSkymap = renderer->createCubeMapFromDDS(L"skymap.dds");
	}

	void createCameras() {
		cmShipFree = std::make_shared<sm::Camera>();
		cmShipFree->data.projectionType = rd::RenderingScene::CameraInfo::ProjectionType::PERSPECTIVE;
		cmShipFree->data.nearZ = 0.1f;
		cmShipFree->data.farZ = 1000;
		cmShipFree->data.aspectRatio = aspectRatio; 
		cmShipFree->data.fieldOfViewYRadian = CMShipFreeFOV;

		cmShipThirdPerson = std::make_shared<sm::Camera>();
		cmShipThirdPerson->data = cmShipFree->data;

		cmNorthFixed = std::make_shared<sm::Camera>();
		cmNorthFixed->transT = { 0, -SumRadius * 5, 0 };
		cmNorthFixed->transS = { 1, 1, -1 };
		cmNorthFixed->transR = DirectX::XMQuaternionRotationAxis({ 1, 0, 0 }, le::PI / 2);
		cmNorthFixed->data.projectionType = rd::RenderingScene::CameraInfo::ProjectionType::PERSPECTIVE;
		cmNorthFixed->data.nearZ = SumRadius;
		cmNorthFixed->data.farZ = SumRadius * 6;
		cmNorthFixed->data.aspectRatio = aspectRatio; 
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
		objEarthShipPlacement = std::make_shared<sm::Object>("Earth Ship Placement");
		objShipCenter = std::make_shared<sm::Object>("Ship Center");
		cmShipThirdInnerOrientationPerson = std::make_shared<sm::Object>("Ship TP Camera Inner Orient");;
		cmShipThirdPersonCoordLayer = std::make_shared<sm::Object>("Ship TP Camera Coord");
		cmShipThirdPersonOrientationLayer = std::make_shared<sm::Object>("Ship TP Camera Orient");
		
		scene->rootObject = objRoot;

		objRoot->children = { objEarthMoonSysOrbitLayer, ltSum, objSum, cmNorthFixed };
		{
			objEarthMoonSysOrbitLayer->children = { objEarthMoonSys };
			// ����ϵ
			objEarthMoonSys->children = { objEarthRotationAngleLayer, objMoonOrbitAngleLayer };
			objEarthMoonSys->moveParentCoord({ SumEarthDistance, 0, 0 });
			{
				// ����
				objEarthRotationAngleLayer->children = { objEarth };
				objEarthRotationAngleLayer->rotateParentCoord(DirectX::XMQuaternionRotationAxis({ 1, 0, 0 }, EarthAxialTilt));
				
				{
					objEarth->children.push_back(objEarthShipPlacement);
					objEarthShipPlacement->children = { objShipCameraSysCoordLayer };
					objEarthShipPlacement->transR = rotateAxisToQuat(EarthBaseRy, EarthBaseRz);

					objShipCameraSysCoordLayer->children = { objShipCameraSysOrientationLayer }; 
					objShipCameraSysOrientationLayer->children = { objShipCameraSys };
					objShipCameraSys->children = { objShip, cmShipFree };

					objShip->children.push_back(objShipCenter);
					objShipCenter->transT = { 0, SpaceshipHeight / 2, 0 };
					objShipCenter->children = { cmShipThirdPersonCoordLayer };
					cmShipThirdPersonCoordLayer->children = { cmShipThirdPersonOrientationLayer };
					cmShipThirdPersonOrientationLayer->children = { cmShipThirdInnerOrientationPerson };
					cmShipThirdInnerOrientationPerson->children = { cmShipThirdPerson };
					cmShipThirdInnerOrientationPerson->transT = { (float)cmShipThirdPersonRotate.accZ, 0, 0 };

					objShipCameraSysCoordLayer->moveParentCoord({ EarthRadius, 0, 0 });
					objShipCameraSysCoordLayer->rotateParentCoord(
						DirectX::XMQuaternionMultiply(
							DirectX::XMQuaternionRotationAxis({ 0, 0, 1 }, -le::PI / 2),
							DirectX::XMQuaternionRotationAxis({ 0, 0, 1 }, 0)
						)
					);
				}
			}
			{
				// ����
				objMoonOrbitAngleLayer->transR = DirectX::XMQuaternionRotationAxis({1, 0, 0}, MoonRevolutionAngle);
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
					objShipCameraSysEarthOrbitLayer->children = { };
				}
			}
		}

		
		scene->link();

		// ������ link ֮�������
		cmShipFreePitchLayer = cmShipFree->insertParent();
		cmShipFreeYawLayer = cmShipFreePitchLayer->insertParent();
		cmShipFreePresetLayer = cmShipFreePitchLayer->insertParent();


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

			objShipCameraSysEMTransferLayer->transR = DirectX::XMQuaternionRotationAxis({ 0, 1, 0 }, (float)moonRevolutionOrbitRotationAngle);

			constexpr double R1 = EarthRadius + OrbitHeightEarth;
			constexpr double R2 = EarthMoonDistance - MoonRadius - OrbitHeightMoon;
			constexpr float offset = (float)((R1 + R2) / 2 - R1);

			objShipCameraSysEMTransferLayer->transT = { 0, 0, 0 };
			objShipCameraSysEMTransferLayer->moveLocalCoord({ offset, 0, 0 });

			moonToEarthTransferStartTime = framerateController.getLastFrameEndTime();

			// ˳��ѷ��ص����Ĳ���Ҳ���ú�
			shipOnEarthRotationAngleOffset = fmod(moonRevolutionOrbitRotationAngle + le::PI, 2 * le::PI);

		} else if (orbitState == OrbitState::EarthToMoon && to == OrbitState::AroundMoon) {
			shipOnMoonRotationAngleOffset = 0;
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
			
		} else if (orbitState == OrbitState::OnTheEarth && to == OrbitState::LaunchEarth) {
			// ���ø���״ֵ̬
			launchFromEarthStartTime = framerateController.getLastFrameEndTime();

			auto shipWorldPos = objShipCameraSys->getWorldPosition();
			auto l2w = objMoonOrbitAngleLayer->getLocalToWorldMatrix();
			auto l2wDet = DirectX::XMMatrixDeterminant(l2w);
			auto w2l = DirectX::XMMatrixInverse(&l2wDet, l2w);
			auto shipLocalPos = DirectX::XMVector3TransformCoord(DirectX::XMLoadFloat3(&shipWorldPos), w2l);
			DirectX::XMFLOAT3 localPos3;
			DirectX::XMStoreFloat3(&localPos3, shipLocalPos);

			auto len = sqrt(localPos3.x * localPos3.x + localPos3.y * localPos3.y + localPos3.z * localPos3.z);
			localPos3.x /= len;
			localPos3.y /= len;
			localPos3.z /= len;

			auto rx = len;
			auto ry = circleToAngle(localPos3.x, localPos3.z);
			auto rz = asin(localPos3.y);

			rCoordLaunchFromEarthFrom = { rx, ry, rz };
 			rCoordLaunchFromEarthTo = { rx + OrbitHeightEarth, ry - EarthLaunchOrbitRadian, 0 };
			launchFromEarthProgress = 0;


			// ׼����νṹ
			objShipCameraSysEarthOrbitLayer->children = { objShipCameraSysCoordLayer };
			objShipCameraSysCoordLayer->parent = objShipCameraSysEarthOrbitLayer;
			objEarthShipPlacement->children.clear();

			// �������֮��ĳ�ʼֵ
			this->shipOnEarthRotationAngleOffset = - ry + EarthLaunchOrbitRadian;
			if (shipOnEarthRotationAngleOffset < 0) shipOnEarthRotationAngleOffset += le::PI * 2;
		} else if (orbitState == OrbitState::LaunchEarth && to == OrbitState::AroundEarth) {
			this->aroundEarthStartTime = framerateController.getLastFrameEndTime();
		} else if (orbitState == OrbitState::AroundEarth && to == OrbitState::LandEarth) {
			objShipCameraSysEarthOrbitLayer->children.clear();
			objEarthShipPlacement->children = { objShipCameraSysCoordLayer };
			objShipCameraSysCoordLayer->parent = objEarthShipPlacement;

			landEarthStartTime = framerateController.getLastFrameEndTime();
			landToEarthProgress = 0;
		} else if (orbitState == OrbitState::LandEarth && to == OrbitState::OnTheEarth) {
			// do nothing extra
		} else if (orbitState == OrbitState::AroundMoon && to == OrbitState::LandMoon) { 
			landOnTheMoonProgress = 0;
			landOnTheMoonStartTime = framerateController.getLastFrameEndTime();
			landOnTheMoonRadianOffset = shipOnMoonRotationAngle;
		} else if (orbitState == OrbitState::LandMoon && to == OrbitState::OnTheMoon) {
			onTheMoonRadian = shipOnMoonRotationAngle;
		} else if (orbitState == OrbitState::OnTheMoon && to == OrbitState::LaunchMoon) {
			launchFromMoonStartTime = framerateController.getLastFrameEndTime();
			launchFromMoonProgress = 0;
		} else if (orbitState == OrbitState::LaunchMoon && to == OrbitState::AroundMoon) {
			aroundMoonStartTime = framerateController.getLastFrameEndTime();
			shipOnMoonRotationAngleOffset = shipOnMoonRotationAngle + le::PI;
		} else {
			assert(false);
		}

		orbitState = to;
	}
	
	// Ҫ������ֵ���� [0, mod) ��
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

	// changeState �õı���
	double lastShipOnEarthRotationAngle = -1;
	double lastShipOnMoonRotationAngle = -1;
	double lastShipOnEarthRy = -1;
	void changeState() {
		if (commandInQueue) {
			if (orbitState == OrbitState::AroundEarth && nextState == OrbitState::EarthToMoon) {
				if (lastShipOnEarthRotationAngle < 0) {
					lastShipOnEarthRotationAngle = shipOnEarthRotationAngle;
					// then do nothing else
				} else {
					double threshold = le::PI + moonRevolutionOrbitRotationAngle + 2 * le::PI / MoonRevolutionPeriod * (EarthMoonTransferTime / 2);
					threshold = fmod(threshold, 2 * le::PI);

					if (modulePassThrough(threshold, lastShipOnEarthRotationAngle, shipOnEarthRotationAngle, le::PI * 2)) {
						lastShipOnEarthRotationAngle = -1;
						// ��ʼת�ƣ���ֹ�κ����������
						forbitCommand = true;
						commandInQueue = false;
						shipOnEarthRotationAngle = threshold;
						// state �Ѿ��ı�
						this->switchOrbitState(OrbitState::EarthToMoon);
					} else {
						lastShipOnEarthRotationAngle = shipOnEarthRotationAngle;
					}
				}
			} else if (orbitState == OrbitState::AroundMoon && nextState == OrbitState::MoonToEarth) {
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
			} else if (orbitState == OrbitState::AroundEarth && nextState == OrbitState::LandEarth) {
				
				auto shipWorldPos = objShipCameraSysCoordLayer->getWorldPosition();
				auto l2w = objEarth->getLocalToWorldMatrix();
				auto l2wDet = DirectX::XMMatrixDeterminant(l2w);
				auto w2l = DirectX::XMMatrixInverse(&l2wDet, l2w);
				auto shipLocalPos = DirectX::XMVector3TransformCoord(DirectX::XMLoadFloat3(&shipWorldPos), w2l);

				DirectX::XMFLOAT3 localPos3;
				DirectX::XMStoreFloat3(&localPos3, shipLocalPos);

				auto len = sqrt(localPos3.x * localPos3.x + localPos3.y * localPos3.y + localPos3.z * localPos3.z);
				localPos3.x /= len;
				localPos3.y /= len;
				localPos3.z /= len;

				auto rx = len;
				auto ry = circleToAngle(localPos3.x, localPos3.z);	// dest ry
				auto rz = asin(localPos3.y);

				if (lastShipOnEarthRy >= 0 && modulePassThrough(LandEarthStartRy, ry, lastShipOnEarthRy, 2 * le::PI)) {
					lastShipOnEarthRy = -1;
					forbitCommand = true;
					commandInQueue = false;
					landEarthStartRz = rz;
					this->switchOrbitState(OrbitState::LandEarth);
				} else {
					lastShipOnEarthRy = ry;
				}
				
			}
		} else {
			if (orbitState == OrbitState::EarthToMoon) {
				// ����ת�ƽ���
				if (earthToMoonRotationAngle > le::PI) {
					this->switchOrbitState(OrbitState::AroundMoon);
					forbitCommand = false;
				}
			} else if (orbitState == OrbitState::MoonToEarth) {
				if (moonToEarthRotationAngle > le::PI) {
					this->switchOrbitState(OrbitState::AroundEarth);
					forbitCommand = false;
				}
			} else if (orbitState == OrbitState::LaunchEarth) {
				if (launchFromEarthProgress > 1) {
					this->switchOrbitState(OrbitState::AroundEarth);
					forbitCommand = false;
				}
			}else if (orbitState == OrbitState::LandEarth) {
				if (landToEarthProgress > 1) {
					this->switchOrbitState(OrbitState::OnTheEarth);
					forbitCommand = false;
				}
			} else if (orbitState == OrbitState::LandMoon) {
				if (landOnTheMoonProgress > 1) {
					this->switchOrbitState(OrbitState::OnTheMoon);
					forbitCommand = false;
				}
			} else if (orbitState == OrbitState::LaunchMoon) {
				if (launchFromMoonProgress > 1) {
					this->switchOrbitState(OrbitState::AroundMoon);
				}
			}
		}


	}

	void moveCameraShipFree(const std::vector<std::tuple<UINT, WPARAM, LPARAM>>& events) {
		if (this->activeCamera != CameraSetting::FirstPerson) return;

		auto lastDuration = (float)framerateController.getLastFrameDuration();

		cmShipFreeRotate.receiveEvent(events, lastDuration);

		auto [rx, ry, rz] = cmShipFreeRotate.getXYZ();

		cmShipFreeYawLayer->transR = DirectX::XMQuaternionRotationAxis({ 0, -1, 0 }, (float)rx);
		cmShipFreePitchLayer->transR = DirectX::XMQuaternionRotationAxis({ -1, 0, 0 }, (float)ry);
		
		cmShipFree->data.fieldOfViewYRadian = (float) std::min<double>(le::PI * 0.8, CMShipFreeFOV * (0.5 + (rz + 1) / 2));
	}

	static const wchar_t* getStateName(OrbitState state) {
		/*
		enum class OrbitState { 
		OnTheEarth, LaunchEarth, AroundEarth, EarthToMoon, LandEarth,
		OnTheMoon,  LaunchMoon,  AroundMoon,  MoonToEarth, LandMoon
		};
		*/
		static const wchar_t* names[] = {
			L"ͣ������",
			L"�ӵ�����",
			L"���Ƶ���",
			L"����ת��",
			L"��½����",
			L"ͣ������",
			L"��������",
			L"��������",
			L"�µ�ת��",
			L"��½����"
		};

		return names[static_cast<int>(state)];
	}

	static const wchar_t* getCameraName(CameraSetting camera) {
		/*
		enum class OrbitState { 
		OnTheEarth, LaunchEarth, AroundEarth, EarthToMoon, LandEarth,
		OnTheMoon,  LaunchMoon,  AroundMoon,  MoonToEarth, LandMoon
		};
		*/
		static const wchar_t* names[] = {
			L"��һ�˳�",
			L"�����˳�",
			L"��������"
		};

		return names[static_cast<int>(camera)];
	}

	void updateWindowTitle() {
		wchar_t operationHint[100];

		swprintf_s(operationHint, L"����: %s��%s%s%s%s%s%s%s%s%s�ո��������꣺���򣻵�ǰ�����%s",
			getStateName(orbitState),
			commandInQueue ? L"����� " : L"",
			commandInQueue ? getStateName(nextState) : L"",
			commandInQueue ? L"��" : L"",
			leftKeyValid ? L"�������" : L"",
			leftKeyValid ? getStateName(leftKeyOp) : L"",
			leftKeyValid ? L"��" : L"",
			rightKeyValid ? L"�ҷ������" : L"",
			rightKeyValid ? getStateName(rightKeyOp) : L"",
			rightKeyValid ? L"��" : L"",
			getCameraName(this->activeCamera)
		);

		SetWindowText(window.getHwnd(), operationHint);
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
		// �������Ƶ����ת��
		// �����Ӧ�Ƕ�
		
		switch (orbitState) {
		case OrbitState::AroundEarth:
		{
			cameraFlyingMode = 1;
			
			double time = framerateController.getLastFrameEndTime() - aroundEarthStartTime;
			shipOnEarthRotationAngle = le::PI * 2 * (fmod(time, ShipRevolutionPeriodAroundEarth) / ShipRevolutionPeriodAroundEarth);
			shipOnEarthRotationAngle += shipOnEarthRotationAngleOffset;
			shipOnEarthRotationAngle = fmod(shipOnEarthRotationAngle, le::PI * 2);

			objShipCameraSysEarthOrbitLayer->transR = DirectX::XMQuaternionRotationAxis({ 0, 1, 0 }, (float)shipOnEarthRotationAngle);

			objShipCameraSysOrientationLayer->transR = DirectX::XMQuaternionMultiply(
				DirectX::XMQuaternionRotationAxis({ 0, 1, 0 }, le::PI),
				DirectX::XMQuaternionRotationAxis({ -1, 0, 0 }, le::PI / 2)
			);
		}
		break;
		case OrbitState::EarthToMoon:
		{
			cameraFlyingMode = 1;
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
				DirectX::XMQuaternionRotationAxis({ 0, 1, 0 }, float(le::PI - earthToMoonRotationAngle)),
				DirectX::XMQuaternionRotationAxis({ -1, 0, 0 }, float(le::PI / 2 - circleToAngle(-x, z)))
			);
		}
		break;
		case OrbitState::AroundMoon:
		{
			cameraFlyingMode = 1;
			double time = framerateController.getLastFrameEndTime() - aroundMoonStartTime;
			shipOnMoonRotationAngle = le::PI + le::PI * 2 * (fmod(time, ShipRevolutionPeriodAroundMoon) / ShipRevolutionPeriodAroundMoon) + shipOnMoonRotationAngleOffset;
			shipOnMoonRotationAngle = fmod(shipOnMoonRotationAngle, 2 * le::PI);

			objShipCameraSysMoonOrbitLayer->transR = DirectX::XMQuaternionRotationAxis({ 0, -1, 0 }, float(shipOnMoonRotationAngle));

			objShipCameraSysOrientationLayer->transR = DirectX::XMQuaternionRotationAxis({ -1, 0, 0 }, -le::PI / 2);
			
		}
		break;
		case OrbitState::MoonToEarth:
		{
			cameraFlyingMode = 1;
			// �ߵ���ת�ƹ���� [0, PI]

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
				DirectX::XMQuaternionRotationAxis({ 0, 1, 0 }, float(moonToEarthRotationAngle)),
				DirectX::XMQuaternionRotationAxis({ -1, 0, 0 }, float(le::PI / 2 - circleToAngle(-x, z)))
			);
		}
		break;
		case OrbitState::OnTheEarth:
		{
			cameraFlyingMode = 0;
			// do nothing
			// todo ��������������һ�³���֮���
		}
		break;
		case OrbitState::LaunchEarth:
		{
			double time = framerateController.getLastFrameEndTime() - launchFromEarthStartTime;
			launchFromEarthProgress = time / LaunchFromEarthTime;


			cameraFlyingMode = launchFromEarthProgress;


			auto [rx1, ry1, rz1] = rCoordLaunchFromEarthFrom;
			auto [rx2, ry2, rz2] = rCoordLaunchFromEarthTo;
			auto quat1 = rotateAxisToQuat((float)ry1, (float)rz1);
			auto quat2 = rotateAxisToQuat((float)ry2, (float)rz2);
			float transformedProgress = float(1 - pow(launchFromEarthProgress - 1, 3));
			this->objShipCameraSysEarthOrbitLayer->transR = DirectX::XMQuaternionSlerp(quat1, quat2, float(launchFromEarthProgress));
			this->objShipCameraSysCoordLayer->transT = { float(rx1 * (1 - launchFromEarthProgress) + rx2 * launchFromEarthProgress), 0, 0 };

			auto orient = DirectX::XMQuaternionMultiply(
				DirectX::XMQuaternionRotationAxis({ 0, 1, 0 }, float(0 * (1 - launchFromEarthProgress) + le::PI * launchFromEarthProgress)),
				DirectX::XMQuaternionRotationAxis({ -1, 0, 0 }, float(le::PI * (1 - transformedProgress) + le::PI / 2 * transformedProgress))
			);

			this->objShipCameraSysOrientationLayer->transR = orient;
		
		}
		break;
		case OrbitState::LandEarth:
		{
			double time = framerateController.getLastFrameEndTime() - landEarthStartTime;
			landToEarthProgress = time / LandToEarthTime;

			cameraFlyingMode = 1 - landToEarthProgress;

			if (landToEarthProgress > 1) break;

			auto quat1 = rotateAxisToQuat(LandEarthStartRy, float(landEarthStartRz));
			auto quat2 = rotateAxisToQuat(EarthBaseRy, EarthBaseRz);
			float transformedProgress = float(pow(landToEarthProgress, 3));
			this->objEarthShipPlacement->transR = DirectX::XMQuaternionSlerp(quat1, quat2, float(landToEarthProgress));
			this->objShipCameraSysCoordLayer->transT = {
				float((OrbitHeightEarth + EarthRadius) * (1 - landToEarthProgress) + EarthRadius * landToEarthProgress), 
				0, 
				0 
			};

			auto orient = DirectX::XMQuaternionMultiply(
				DirectX::XMQuaternionRotationAxis({ 0, 1, 0 }, float(le::PI * (1 - landToEarthProgress) + 0 * landToEarthProgress)),
				DirectX::XMQuaternionRotationAxis({ -1, 0, 0 }, float(le::PI / 2 * (1 - transformedProgress) + 0 * transformedProgress))
			);

			this->objShipCameraSysOrientationLayer->transR = orient;

		}
		break;
		case OrbitState::LandMoon:
		{

			
			double time = framerateController.getLastFrameEndTime() - landOnTheMoonStartTime;
			landOnTheMoonProgress = time / LandToMoonTime;
			double transformedTime = pow(landOnTheMoonProgress, 3);
			
			cameraFlyingMode = 1 - landOnTheMoonProgress;

			shipOnMoonRotationAngle = le::PI * 2 * (fmod(time, ShipRevolutionPeriodAroundMoon) / ShipRevolutionPeriodAroundMoon) + landOnTheMoonRadianOffset;
			shipOnMoonRotationAngle = fmod(shipOnMoonRotationAngle, 2 * le::PI);

			objShipCameraSysMoonOrbitLayer->transR = DirectX::XMQuaternionRotationAxis({ 0, -1, 0 }, float(shipOnMoonRotationAngle));

			objShipCameraSysOrientationLayer->transR = DirectX::XMQuaternionRotationAxis({ -1, 0, 0 }, float((-le::PI / 2) * (1 - transformedTime) + 0 * transformedTime));
			
			objShipCameraSysCoordLayer->transT = {
				float(MoonRadius + (1 - landOnTheMoonProgress) * OrbitHeightMoon + landOnTheMoonProgress * 0),
				0, 0
			};
		}
		break;
		case OrbitState::OnTheMoon:
		{
			cameraFlyingMode = 0;
			objShipCameraSysMoonOrbitLayer->transR = DirectX::XMQuaternionRotationAxis({ 0, -1, 0 }, float(onTheMoonRadian));
		}
		break;
		case OrbitState::LaunchMoon:
		{
			double time = framerateController.getLastFrameEndTime() - launchFromMoonStartTime;
			launchFromMoonProgress = time / LaunchFromMoonTime;
			double transformedTime = pow(launchFromMoonProgress, 3);

			cameraFlyingMode = launchFromMoonProgress;

			shipOnMoonRotationAngle = le::PI + le::PI * 2 * (fmod(time, ShipRevolutionPeriodAroundMoon) / ShipRevolutionPeriodAroundMoon) + landOnTheMoonRadianOffset;
			shipOnMoonRotationAngle = fmod(shipOnMoonRotationAngle, 2 * le::PI);

			objShipCameraSysMoonOrbitLayer->transR = DirectX::XMQuaternionRotationAxis({ 0, -1, 0 }, float(shipOnMoonRotationAngle));

			objShipCameraSysOrientationLayer->transR = DirectX::XMQuaternionRotationAxis({ -1, 0, 0 }, float((0) * (1 - transformedTime) + (-le::PI / 2) * transformedTime));

			objShipCameraSysCoordLayer->transT = {
				float(MoonRadius + (1 - launchFromMoonProgress) * 0 + launchFromMoonProgress * OrbitHeightMoon),
				0, 0
			};
		}
		break;
		default:
			assert(false);
		}
	}

	void updateEarthRevolutionAnimation() {
		// ����ת
		double time = framerateController.getLastFrameEndTime();
		double rotationAngle = le::PI * 2 * (fmod(time, EarthRevolutionPeriod) / EarthRevolutionPeriod);
		objEarthMoonSysOrbitLayer->transR = DirectX::XMQuaternionRotationAxis({0, 1, 0}, (float)rotationAngle);
	}

	void updateEarthRotationAnimation() {
		double time = framerateController.getLastFrameEndTime();
		earthRotationAngle = le::PI * 2 * (fmod(time, EarthRotationPeriod) / EarthRotationPeriod);
		objEarth->transR = DirectX::XMQuaternionRotationAxis({0, 1, 0}, (float)earthRotationAngle);
	}
	
	void updateMoonRevolutionAnimation() {
		double time = framerateController.getLastFrameEndTime();
		double rotationAngle = le::PI * 2 * (fmod(time, MoonRevolutionPeriod) / MoonRevolutionPeriod);
		moonRevolutionOrbitRotationAngle = rotationAngle;
		objMoonOrbit->transR = DirectX::XMQuaternionRotationAxis({0, 1, 0}, (float)rotationAngle);
	}

	void updateCameraAnimation() {
		cmShipFreePresetLayer->transT = { 
			0, 
			float((1 - cameraFlyingMode) * SpaceshipHeight * 3 + cameraFlyingMode * (-SpaceshipHeight)), 
			float((1 - cameraFlyingMode) * SpaceshipHeight * 2 + cameraFlyingMode * (-SpaceshipHeight))
		};

		static auto cmShipFreeQuat1 = DirectX::XMQuaternionMultiply(
			DirectX::XMQuaternionRotationAxis({ -1, 0, 0 }, -le::PI / 2 * 0.5),
			DirectX::XMQuaternionRotationAxis({ 0, 1, 0 },le::PI)
		);
		static auto cmShipFreeQuat2 = DirectX::XMQuaternionMultiply(
			DirectX::XMQuaternionRotationAxis({ -1, 0, 0 }, (le::PI / 2 * 0.8)),
			DirectX::XMQuaternionRotationAxis({ 0, 1, 0 }, 0)
		);

		cmShipFreePresetLayer->transR = DirectX::XMQuaternionSlerp(cmShipFreeQuat1, cmShipFreeQuat2, float(cameraFlyingMode));
	}

	void updateAnimation() {
		updateSpaceshipAnimation();
		updateCameraAnimation();
		updateEarthRevolutionAnimation();
		updateEarthRotationAnimation();
		updateMoonRevolutionAnimation();
	}

	void processEvents(const std::vector<std::tuple<UINT, WPARAM, LPARAM>>& events) {
		constexpr float NORTH_FIX_SHIP_SCALE = 10;
		for (auto [msg, wparam, lparam]: events) {
			if (msg == WM_KEYDOWN && wparam == VK_SPACE) {
				objShip->multiplyScale({ NORTH_FIX_SHIP_SCALE, NORTH_FIX_SHIP_SCALE, NORTH_FIX_SHIP_SCALE });
				if (activeCamera == CameraSetting::FirstPerson) {
					// first person -> third person
					scene->activeCamera = this->cmShipThirdPerson;
					activeCamera = CameraSetting::ThirdPerson;
					objShip->transS = { 1, 1, 1 };
				} else if(activeCamera == CameraSetting::ThirdPerson){
					// third person -> north fixed person
					scene->activeCamera = this->cmNorthFixed;
					activeCamera = CameraSetting::North;
					objShip->transS = { NORTH_FIX_SHIP_SCALE, NORTH_FIX_SHIP_SCALE, NORTH_FIX_SHIP_SCALE };
				} else {	
					scene->activeCamera = this->cmShipFree;
					activeCamera = CameraSetting::FirstPerson;
					objShip->transS = { 1, 1, 1 };
				}
			}


			// left -> Earth; right -> Moon
			if (msg == WM_KEYDOWN && (wparam == VK_LEFT || wparam == VK_RIGHT) && !forbitCommand) {
				switch (orbitState) {
				case OrbitState::AroundEarth:
					if (wparam == VK_LEFT) {
						// land on the earth
						commandInQueue = true;
						nextState = OrbitState::LandEarth;
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
						forbitCommand = true;
						this->switchOrbitState(OrbitState::LandMoon);
					}
					break;
				case OrbitState::OnTheEarth:
					if (wparam == VK_RIGHT) {
						// launch from the earth
						this->switchOrbitState(OrbitState::LaunchEarth);
						forbitCommand = true;
					} else {
						// invalid
					}
					break;
				case OrbitState::OnTheMoon:
					if (wparam == VK_RIGHT) {
						// invalid
					} else {
						// launch from the moon
						this->switchOrbitState(OrbitState::LaunchMoon);
					}
				default:
					break;
				}
			}
		}
	}

	void updatetValidOperationInfo() {

		switch (orbitState) {
		case OrbitState::AroundEarth:

			leftKeyValid = true;
			leftKeyOp = OrbitState::LandEarth;
			
			rightKeyValid = true;
			rightKeyOp = OrbitState::EarthToMoon;
			
			break;
		case OrbitState::AroundMoon:

			leftKeyValid = true;
			leftKeyOp = OrbitState::MoonToEarth;

			rightKeyValid = true;
			rightKeyOp = OrbitState::LandMoon;

			break;
		case OrbitState::OnTheEarth:
			leftKeyValid = false;

			rightKeyValid = true;
			rightKeyOp = OrbitState::LaunchEarth;

			break;
		case OrbitState::OnTheMoon:

			leftKeyValid = true;
			leftKeyOp = OrbitState::LaunchMoon;

			rightKeyValid = false;
			break;
		default:
			leftKeyValid = false;
			rightKeyValid = false;
			break;
		}

	}


	void moveCameraNorthFixed() {
		auto pos = this->objEarthMoonSys->getPositionInAncestor(this->objRoot);
		cmNorthFixed->transT.x = pos.x / 2;
		cmNorthFixed->transT.z = pos.z / 2;
	}

	void moveCameraThirdPerson(const std::vector<std::tuple<UINT, WPARAM, LPARAM>>& events) {
		if (activeCamera != CameraSetting::ThirdPerson) return;
		this->cmShipThirdPersonRotate.receiveEvent(events, framerateController.getLastFrameDuration());
		auto [rz, ry, rx] = this->cmShipThirdPersonRotate.getXYZ();
		this->cmShipThirdInnerOrientationPerson->transT.x = (float)rx;
		
		this->cmShipThirdPerson->transR = DirectX::XMQuaternionRotationAxis({ 0, 1, 0 }, le::PI / 2);

		this->cmShipThirdPersonOrientationLayer->transR = rotateAxisToQuat((float)rz, (float)ry);
	}

	void renderCallback(const io::RenderingWindow&, const std::vector<std::tuple<UINT, WPARAM, LPARAM>>& events) {
		// ������� begin �޺�
		framerateController.begin();

		processEvents(events);
		updatetValidOperationInfo();
		updateWindowTitle();

		changeState();
		updateAnimation();
		moveCameraShipFree(events);
		moveCameraThirdPerson(events);
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
