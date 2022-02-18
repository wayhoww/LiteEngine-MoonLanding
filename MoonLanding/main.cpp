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

	rd::Renderer* renderer;
	io::RenderingWindow window;
	// framerate controller 提供额外的帧率控制，也提供帧率信息
	io::FramerateController framerateController;

	std::shared_ptr<sm::Scene> scene;

	// mesh-related
	std::shared_ptr<sm::Object> objSum;
	std::shared_ptr<sm::Object> objMoon;
	std::shared_ptr<sm::Object> objEarth;
	std::shared_ptr<sm::Object> objShip;
	std::shared_ptr<sm::Object> objShipCameraSys;
	std::shared_ptr<sm::Object> objEarthMoonSys;
	std::shared_ptr<sm::Object> objRoot;

	// camera-related
	static constexpr float CMShipFreeFOV = le::PI / 2;
	std::shared_ptr<sm::Camera> cmShipFree;		// 相对飞船，自由移动
	std::shared_ptr<sm::Object> cmShipFreePitchLayer;
	std::shared_ptr<sm::Object> cmShipFreeYawLayer;
	io::HoldRotateController cmShipFreeRotate;
	io::MoveController cmShipFreeMoveX{'D', 'A'};
	io::MoveController cmShipFreeMoveY{'E', 'C'};
	io::MoveController cmShipFreeMoveZ{'W', 'S'};

	// light-related
	std::shared_ptr<sm::Light> ltSum; // 位于 Root，直接控制方向

//	std::shared_ptr<sm::Camera> cmShipRotate;	// 相对飞船


	MoonLandingGame(): window(
		L"Rendering Window", 
		WS_OVERLAPPEDWINDOW ^ WS_SIZEBOX ^ WS_MAXIMIZEBOX,
		0, 0, 500, 500), 
		framerateController(55, io::FramerateControlling::WaitableObject)
	{
		rd::Renderer::setHandle(window.getHwnd());
		renderer = &rd::Renderer::getInstance();

		scene = std::shared_ptr<sm::Scene>(new sm::Scene());

		window.renderCallback = [this](auto& x, auto& y) { renderCallback(x, y); };
	}

	void loadResources() {
		objSum = io::loadDefaultResourceGLTF("Earth_10m.glb");
		objEarth = io::loadDefaultResourceGLTF("Earth_10m.glb");
		objMoon = io::loadDefaultResourceGLTF("Moon_10m.glb");
		objShip = io::loadDefaultResourceGLTF("Spaceship_5cm.glb");
	}

	void createFreeCamera() {
		cmShipFree = std::shared_ptr<sm::Camera>(new sm::Camera());
		cmShipFree->data.projectionType = rd::RenderingScene::CameraInfo::ProjectionType::PERSPECTIVE;
		cmShipFree->data.nearZ = 0.1f;
		cmShipFree->data.farZ = 1000;
		cmShipFree->data.aspectRatio = 1; // TODO
		cmShipFree->data.fieldOfViewYRadian = CMShipFreeFOV;
	}

	void createLights() {
		ltSum = std::shared_ptr<sm::Light>(new sm::Light(/*"SumLight"*/));
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

		objShipCameraSys = std::shared_ptr<sm::Object>(new sm::Object("ShipCameraSystem"));
		objEarthMoonSys = std::shared_ptr<sm::Object>(new sm::Object("EarthMoonSystem"));
		objRoot = std::shared_ptr<sm::Object>(new sm::Object("SceneRoot"));

		objSum->multiplyScale({ SumRadius / 10, SumRadius / 10, SumRadius / 10 });
		objEarth->multiplyScale({ EarthRadius / 10, EarthRadius / 10, EarthRadius / 10 });
		objMoon->multiplyScale({ MoonRadius / 10, MoonRadius / 10, MoonRadius / 10 });
		objShip->multiplyScale({ SpaceshipHeight / 0.05f, SpaceshipHeight / 0.05f, SpaceshipHeight / 0.05f });

		scene->rootObject = objRoot;
		objRoot->children = { objEarthMoonSys, ltSum, objSum };
		objShipCameraSys->children = { objShip, cmShipFree };
		objEarthMoonSys->children = { objEarth, objMoon, objShipCameraSys };

		scene->link();		

		objEarthMoonSys->moveParentCoord({ SumEarthDistance, 0, 0 });
		objMoon->moveParentCoord({ EarthMoonDistance, 0, 0 });

		objShip->rotateParentCoord(DirectX::XMQuaternionRotationAxis({ 1, 0, 0 }, le::PI / 2));

		objShipCameraSys->moveParentCoord({ EarthRadius + OrbitHeight, 0, 0 });

		cmShipFreePitchLayer = cmShipFree->insertParent();
		cmShipFreeYawLayer = cmShipFreePitchLayer->insertParent();
		cmShipFreePitchLayer->moveParentCoord({ 0, SpaceshipHeight	* 0.5, 0 });

		updateLights();
	}

	void moveCameraShipFree(const std::vector<std::tuple<UINT, WPARAM, LPARAM>>& events) {
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

	void updateAnimation() {

		// 飞行器绕地球的转动
		// 距离对应角度

		double duration = framerateController.getLastFrameDuration();
		double timePerCycle = 20;

		// delta angle
		auto deltaAngle = duration / timePerCycle * le::PI * 2;
		auto deltaPos = deltaAngle * (EarthRadius + OrbitHeight);

		objShipCameraSys->moveLocalCoord({0, 0, (float)deltaPos});
		objShipCameraSys->rotateLocalCoord(DirectX::XMQuaternionRotationAxis({0, -1, 0}, (float)deltaAngle));
	}

	void renderCallback(const io::RenderingWindow&, const std::vector<std::tuple<UINT, WPARAM, LPARAM>>& events) {
		// 额外调用 begin 无害
		framerateController.begin();

		for (auto [msg, wparam, lparam]: events) {
			if (msg == WM_KEYDOWN && wparam == VK_SPACE) {
				objRoot->dump();
			}
		}

		updateWindowTitle();

		updateAnimation();
		moveCameraShipFree(events);
		updateLights();


		auto rScene = scene->getRenderingScene();

		renderer->beginRendering();
		renderer->renderScene(rScene, false);
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
		createFreeCamera();
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
	auto& game = MoonLandingGame::getInstance();
	game.lanuch();
}
