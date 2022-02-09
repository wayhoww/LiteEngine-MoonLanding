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

int WINAPI wWinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ PWSTR pCmdLine,
	_In_ int nCmdShow
) {
	// required by DirectXTeX
	// 需要在 ！！每一个！！ 调用 DirectX 的线程调用
	Microsoft::WRL::Wrappers::RoInitializeWrapper initialize(RO_INIT_MULTITHREADED);
	if (FAILED(initialize)) {
		assert(false);
	}

	SetProcessDPIAware();

//	return 0;

	//D3D11_APPEND_ALIGNED_ELEMENT packing?
	namespace le = LiteEngine;
	namespace ler = le::Rendering;
	namespace lesm = le::SceneManagement;

	le::RenderingWindow window(
		L"Rendering Window", 
		WS_OVERLAPPEDWINDOW ^ WS_SIZEBOX ^ WS_MAXIMIZEBOX,
		0, 0, 500, 500);

	ler::Renderer::setHandle(window.getHwnd());
	auto& renderer = ler::Renderer::getInstance();

	auto res = le::IO::loadDefaultResourceGLTF("C:/Users/wweih/Desktop/untitled.gltf");

	lesm::Scene smScene;
	smScene.rootObject = res;
	smScene.activeCamera = smScene.search<lesm::Camera>("Camera");

	window.resizeClientArea((int)(1080 * smScene.activeCamera->data.aspectRatio), 1080);
	renderer.resizeFitWindow();

	std::shared_ptr<ler::RenderingScene> scene = smScene.getRenderingScene();
	
	auto t_begin = clock();

	window.renderCallback = [&](const le::RenderingWindow& window) {
		
		renderer.renderFrame(*scene);

		auto t_end = clock();
		auto time = 1.0 * (t_end - t_begin) / CLOCKS_PER_SEC;
		if (time > 2) {
			t_begin = t_end;

			////auto res = le::IO::loadDefaultResourceAssimp("C:/Users/wweih/Desktop/untitled.blend");
			//auto res = le::IO::loadDefaultResourceGLTF("C:/Users/wweih/Desktop/untitled.gltf");
			//lesm::Scene smScene;
			//smScene.rootObject = res;
			//smScene.activeCamera = smScene.search<lesm::Camera>("Camera");
			//scene = smScene.getRenderingScene();
		}
	};

	window.show();

	window.runRenderingLoop();

	return 0;
}
