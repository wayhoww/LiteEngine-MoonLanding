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
	static constexpr float OrbitHeight = 5;
	static constexpr float EarthRevolutionPeriod = 240; // ����ת����
	static constexpr float EarthRotationPeriod = 40; // �����Դ�����
	static constexpr float MoonRevolutionPeriod = 80; // ����ת����
	static constexpr float ShipRevolutionPeriodAroundEarth = 20;	// �ɴ��Ƶ���ת����
	static constexpr float EarthAxialTilt = (float) ((23 + 26 / 60.0) / 180 * le::PI); // �Ƴཻ��
	static constexpr float MoonRevolutionAngle = (float)(5.0 / 180 * le::PI); // ������ת����͵���ļн�

	rd::Renderer* renderer;
	io::RenderingWindow window;
	// framerate controller �ṩ�����֡�ʿ��ƣ�Ҳ�ṩ֡����Ϣ
	io::FramerateController framerateController;

	std::shared_ptr<sm::Scene> scene;

	// textures
	rd::PtrShaderResourceView texSkymap;

	// mesh-related
	std::shared_ptr<sm::Object> objSum;
	std::shared_ptr<sm::Object> objMoon;
	std::shared_ptr<sm::Object> objMoonOrbit;
	std::shared_ptr<sm::Object> objMoonOrbitAngleLayer; // ����ͻƵ��ļн�
	std::shared_ptr<sm::Object> objEarth;
	std::shared_ptr<sm::Object> objEarthRotationAngleLayer;	// ����Ƴཻ��
	std::shared_ptr<sm::Object> objShip;
	std::shared_ptr<sm::Object> objShipCameraSys;
	std::shared_ptr<sm::Object> objShipCameraSysOrientationLayer;
	std::shared_ptr<sm::Object> objShipCameraSysEarthOrbitLayer;
	std::shared_ptr<sm::Object> objEarthMoonSys;
	std::shared_ptr<sm::Object> objEarthMoonSysOrbitLayer;
	std::shared_ptr<sm::Object> objRoot;

	// camera-related
	static constexpr float CMShipFreeFOV = le::PI / 2;
	std::shared_ptr<sm::Camera> cmShipFree;		// ��Էɴ��������ƶ�
	std::shared_ptr<sm::Object> cmShipFreePitchLayer;
	std::shared_ptr<sm::Object> cmShipFreeYawLayer;
	std::shared_ptr<sm::Object> cmShipFreePresetLayer;
	io::HoldRotateController cmShipFreeRotate;
	io::MoveController cmShipFreeMoveX{'D', 'A'};
	io::MoveController cmShipFreeMoveY{'E', 'C'};
	io::MoveController cmShipFreeMoveZ{'W', 'S'};

	std::shared_ptr<sm::Camera> cmNorthFixed;		// �̶���̫���Ϸ����������

	// light-related
	std::shared_ptr<sm::Light> ltSum; // λ�� Root��ֱ�ӿ��Ʒ���

	// states
	enum class CameraSetting { ShipFree, FixedNorth };
	CameraSetting activeCamera = CameraSetting::ShipFree;


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
		objShipCameraSysOrientationLayer = std::make_shared<sm::Object>("ShipCameraSystem Moving Dir");
		objShipCameraSysEarthOrbitLayer = std::make_shared<sm::Object>("ShipCameraSystem Earth Orbit");
		objEarthMoonSys = std::make_shared<sm::Object>("EarthMoonSystem");
		objEarthMoonSysOrbitLayer = std::make_shared<sm::Object>("EarthMoonSystem Orbit Layer");
		objEarthRotationAngleLayer = std::make_shared<sm::Object>("Earth Ro Angle Layer");
		objRoot = std::make_shared<sm::Object>("SceneRoot");
		objMoonOrbit = std::make_shared<sm::Object>("Moon Orbit");
		objMoonOrbitAngleLayer = std::make_shared<sm::Object>("Moon Re Angle Layer");

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
				objEarthRotationAngleLayer->rotateParentCoord(DirectX::XMQuaternionRotationAxis({ 1, 0, 0 }, (float)EarthAxialTilt));
			}
			{
				// ����
				objMoonOrbitAngleLayer->transR = DirectX::XMQuaternionRotationAxis({0, 0, 1}, MoonRevolutionAngle);
				objMoonOrbitAngleLayer->children = { objMoonOrbit, objShipCameraSysEarthOrbitLayer };

				{
					objMoonOrbit->children = { objMoon };
					{
						objMoon->moveParentCoord({ EarthMoonDistance, 0, 0 });
					}
				}
				{
					objShipCameraSysEarthOrbitLayer->children = { objShipCameraSysOrientationLayer };
					objShipCameraSysOrientationLayer->children = { objShipCameraSys };
					objShipCameraSys->children = { objShip, cmShipFree };

					objShipCameraSysOrientationLayer->moveParentCoord({ EarthRadius + OrbitHeight, 0, 0 });
					objShipCameraSysOrientationLayer->rotateParentCoord(
						DirectX::XMQuaternionMultiply(
							DirectX::XMQuaternionRotationAxis({ -1, 0, 0 }, le::PI / 2),
							DirectX::XMQuaternionRotationAxis({ 0, 0, 1 }, le::PI / 2)
						)
					);
				} 
			}
		}

		
		scene->link();

		// ������ link ֮�������
		cmShipFreePitchLayer = cmShipFree->insertParent();
		cmShipFreeYawLayer = cmShipFreePitchLayer->insertParent();
		cmShipFreePresetLayer = cmShipFreePitchLayer->insertParent();
		cmShipFreePresetLayer->moveParentCoord({ 0, -SpaceshipHeight, -SpaceshipHeight });
		cmShipFreePresetLayer->rotateParentCoord(DirectX::XMQuaternionRotationAxis({ -1, 0, 0 }, le::PI / 2 * 0.8));
		
		updateLights();
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

	void updateSpaceshipAnimation() {
		// �������Ƶ����ת��
		// �����Ӧ�Ƕ�

		double time = framerateController.getLastFrameEndTime();
		double rotationAngle = le::PI * 2 * (fmod(time, ShipRevolutionPeriodAroundEarth) / ShipRevolutionPeriodAroundEarth);
		objShipCameraSysEarthOrbitLayer->transR = DirectX::XMQuaternionRotationAxis({0, 1, 0}, (float)rotationAngle);


	}

	void updateEarthRevolutionAnimation() {
		// ����ת
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
		objMoonOrbit->transR = DirectX::XMQuaternionRotationAxis({0, 1, 0}, (float)rotationAngle);
	}

	void updateAnimation() {
		updateSpaceshipAnimation();
		updateEarthRevolutionAnimation();
		updateEarthRotationAnimation();
		updateMoonRevolutionAnimation();
	}

	void processEvents(const std::vector<std::tuple<UINT, WPARAM, LPARAM>>& events) {
		constexpr float NORTH_FIX_SHIP_SCALE = 20;
		for (auto [msg, wparam, lparam]: events) {
			if (msg == WM_KEYDOWN && wparam == VK_SPACE) {
				objRoot->dump();
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
		// ������� begin �޺�
		framerateController.begin();

		processEvents(events);

		updateWindowTitle();

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
