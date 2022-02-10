#include <Windows.h>
#include <wrl.h>

#include "LiteEngine/Window/RenderingWindow.h"
#include "LiteEngine/Renderer/Renderer.h"
#include "LiteEngine/Renderer/Resources.h"
#include "LiteEngine/Utilities/Utilities.h"
#include "LiteEngine/IO/DefaultLoader.h"

#include <numeric>
#include <algorithm>
#include <cmath>

#pragma comment(lib, "runtimeobject") // required by RoInitializeWrapper

/*
	smScene.activeCamera->data.fieldOfViewYRadian = 1.0247789206214755f;
	smScene.activeCamera->data.aspectRatio = float(1.0 * size.width / size.height);
*/

uint32_t getKeyCode(char c) {
	constexpr UINT KEYCODE_A = 0x41;
	if (c >= 'a' && c <= 'z') {
		return KEYCODE_A + (c - 'a');
	} else if (c >= 'A' && c <= 'Z') {
		return KEYCODE_A + (c - 'A');
	} else {
		throw std::exception("invalid key");
	}
}

int WINAPI wWinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ PWSTR pCmdLine,
	_In_ int nCmdShow
) {
	// required by DirectXTeX
	// 需要在 ！！每一个！！ 调用 DirectXTeX 的线程调用
	Microsoft::WRL::Wrappers::RoInitializeWrapper initialize(RO_INIT_MULTITHREADED);
	if (FAILED(initialize)) {
		assert(false);
	}

	SetProcessDPIAware();

	namespace le = LiteEngine;
	namespace ler = le::Rendering;
	namespace lesm = le::SceneManagement;

	le::RenderingWindow window(
		L"Rendering Window", 
		WS_OVERLAPPEDWINDOW ^ WS_SIZEBOX ^ WS_MAXIMIZEBOX,
		0, 0, 500, 500);

	ler::Renderer::setHandle(window.getHwnd());
	auto& renderer = ler::Renderer::getInstance();

	auto res = le::IO::loadDefaultResourceGLTF("cube.gltf");

	lesm::Scene smScene;
	smScene.rootObject = res;
	smScene.activeCamera = smScene.search<lesm::Camera>("Camera");
	smScene.skybox = renderer.createCubeMapFromDDS(L"skybox.dds");

	window.resizeClientArea((int)(1080 * smScene.activeCamera->data.aspectRatio), 1080);
	renderer.resizeFitWindow();

	
	auto t_begin = clock();

	auto cameraBackup = *smScene.activeCamera;

	window.renderCallback = [&](
		const le::RenderingWindow& window,
		const std::vector<le::RenderingWindow::EventType>& events
		) {
		// 基本上是 -2 ~ 2 的范围
		int countForward = 0;	// +z
		int countUp = 0;		// +y
		int countRight = 0;		// +x

		int countRotateUp = 0;			// +x
		int countRotateRight = 0;		// +x

		bool resetCamera = false;
		for (auto [msg, wparam, lparam] : events) {
			if (msg == WM_KEYDOWN) {
				auto keyCount = LOWORD(lparam);
				if (wparam == getKeyCode('W')) {
					countForward += keyCount;
				} else if (wparam == getKeyCode('S')) {
					countForward -= keyCount;
				} else if (wparam == getKeyCode('D')) {
					countRight += keyCount;
				} else if (wparam == getKeyCode('A')) {
					countRight -= keyCount;
				} else if (wparam == getKeyCode('E')) {
					countUp += keyCount;
				} else if (wparam == getKeyCode('C')) {
					countUp -= keyCount;
				} else if (wparam == VK_UP) {
					countRotateUp += keyCount;
				} else if (wparam == VK_DOWN) {
					countRotateUp -= keyCount;
				} else if (wparam == VK_RIGHT) {
					countRotateRight += keyCount;
				} else if (wparam == VK_LEFT) {
					countRotateRight -= keyCount;
				} else if (wparam == VK_SPACE) {
					resetCamera = true;
				}
			}
		}
		float moveUnit = float(1 / (renderer.getCurrentFPS() / 3.f));
		float rotateUnit = float(3.14159 / renderer.getCurrentFPS() / 10);

		smScene.activeCamera->moveLocalCoord({countRight * moveUnit, countUp * moveUnit, countForward * moveUnit});
		smScene.activeCamera->rotateLocalCoord(DirectX::XMQuaternionRotationAxis({0, -1, 0, 0}, rotateUnit * countRotateRight));
		smScene.activeCamera->rotateLocalCoord(DirectX::XMQuaternionRotationAxis({1, 0, 0, 0}, rotateUnit * countRotateUp));
		if (resetCamera) {
			*smScene.activeCamera = cameraBackup;
		}

		// 其实 getRenderingScene 可以在另外一个线程访问，完全不涉及图形 API
		std::shared_ptr<ler::RenderingScene> scene = smScene.getRenderingScene();
		renderer.renderFrame(*scene);

		auto fps = renderer.getAverageFPS();
		wchar_t buffer[100];
		swprintf_s(buffer, L"Recent Average FPS: %3.0lf", fps);
		SetWindowText(window.getHwnd(), buffer);
	};

	window.show();

	window.runRenderingLoop();

	return 0;
}
