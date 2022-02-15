#include <Windows.h>
#include <wrl.h>

#include "LiteEngine/Window/RenderingWindow.h"
#include "LiteEngine/Renderer/Renderer.h"
#include "LiteEngine/Renderer/Resources.h"
#include "LiteEngine/Utilities/Utilities.h"
#include "LiteEngine/IO/DefaultLoader.h"
#include "LiteEngine/Renderer/Shadow.h"

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

	auto res = le::IO::loadDefaultResourceGLTF("testcase.gltf");

	lesm::Scene smScene;
	smScene.rootObject = res;
	auto mainCamera = smScene.search<lesm::Camera>("Camera");
	// mainCamera->data.farZ = 1000;
	
	auto probeCamera = smScene.search<lesm::Camera>("ProbeCamera");
	// probeCamera->data.farZ = 1000;
	probeCamera->data.aspectRatio = 1;
	auto skybox = renderer.createCubeMapFromDDS(L"skybox.dds");

	auto movingObject = smScene.searchObject("Icosphere.014");
	movingObject->moveParentCoord({ 0, 0, 0.5 });

	auto renderableTexture = renderer.createRenderableTexture(1000, 1000, 1);
	
	auto screenObj = smScene.searchObject("ScreenPlane")->children[0];
	auto screenPlane = std::dynamic_pointer_cast<lesm::Mesh>(screenObj);
	auto screenMat = std::dynamic_pointer_cast<lesm::DefaultMaterial>(screenPlane->material);
	
	auto offscreenPassRastDesc = CD3D11_RASTERIZER_DESC(CD3D11_DEFAULT());
	offscreenPassRastDesc.CullMode = D3D11_CULL_NONE;
	auto offscreenPass = renderer.createRenderingPassWithoutSceneAndTarget(
		offscreenPassRastDesc,
		CD3D11_DEPTH_STENCIL_DESC(CD3D11_DEFAULT())
	);
	offscreenPass->renderTargetView = renderableTexture.renderTargetView;
	offscreenPass->depthStencilView = renderer.createDepthStencilView(1000, 1000);
	offscreenPass->clearStencil = false;
	D3D11_VIEWPORT viewport = {};
	viewport.Width = 1000;
	viewport.Height = 1000;
	viewport.MaxDepth = 1;
	viewport.MinDepth = 0;
	offscreenPass->viewport = viewport;
	
	// Z-up 右手系 -> Y-up 左手系
	// C++ 中出现的所有矩阵都应该是用于右乘（vec * mat）的矩阵
	DirectX::XMMATRIX skyboxTransform = {
		0, 1, 0, 0,
		0, 0, 1, 0,
		-1, 0, 0, 0,
		0, 0, 0, 1
	};

	window.resizeClientArea((int)(1080 * mainCamera->data.aspectRatio), 1080);
	renderer.resizeFitWindow();

	
	auto t_begin = clock();

	auto cameraBackup = *mainCamera;

	auto redTexture = renderer.createDefaultTexture({ 1, 0, 0, 0 }, 2);
	auto blueTexture = renderer.createDefaultTexture({ 0, 0, 1, 0 }, 2);

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
		float moveUnit = float(1. / 60 * 3);
		float rotateUnit = float(3.14159 / 60 / 10);

		movingObject->rotateLocalCoord({ 0, 0, 1 }, 0.02f / le::PI);
		movingObject->moveLocalCoord({ 0.02f, 0, 0 });
		mainCamera->moveLocalCoord({countRight * moveUnit, countUp * moveUnit, countForward * moveUnit});
		mainCamera->rotateLocalCoord(DirectX::XMQuaternionRotationAxis({0, -1, 0, 0}, rotateUnit * countRotateRight));
		mainCamera->rotateLocalCoord(DirectX::XMQuaternionRotationAxis({1, 0, 0, 0}, rotateUnit * countRotateUp));
		if (resetCamera) {
			*mainCamera = cameraBackup;
		}

		smScene.activeCamera = mainCamera;


		// 其实 getRenderingScene 可以在另外一个线程访问，完全不涉及图形 API
		std::shared_ptr<ler::RenderingScene> scene = smScene.getRenderingScene();
		auto mainRenderingCamera = scene->camera;

		// 
		auto cameras = ler::getSuggestedDepthCamera(mainRenderingCamera, scene->lights[0], { 0.1f, 1.f, 2.f, 4.f });
		// renderer.renderFrame(*scene);

		renderer.beginRendering();

		offscreenPass->scene = scene;
		offscreenPass->scene->camera = smScene.getCameraInfo(probeCamera);
		screenMat->texEmissionColor = nullptr; 
		screenMat->constants->cpuData<lesm::DefaultMaterialConstantData>().uvEmissionColor = UINT32_MAX;

		std::vector<std::shared_ptr<ler::RenderingPass>> passes;
		renderer.createShadowMapPasses(passes, scene, {scene->camera.nearZ, 3, 10, 30, 100});
		renderer.renderPasses(passes);

		renderer.enableBuiltinShadowMap(offscreenPass);
		renderer.renderPass(offscreenPass);
		
		scene->camera = mainRenderingCamera;

		screenMat->texEmissionColor = renderableTexture.textureView;
		screenMat->constants->cpuData<lesm::DefaultMaterialConstantData>().uvEmissionColor = 0;
		renderer.renderScene(scene);

		
		renderer.renderSkybox(skybox, scene->camera, skyboxTransform);

		renderer.swap();

		auto fps = renderer.getAverageFPS();
		wchar_t buffer[100];
		swprintf_s(buffer, L"Recent Average FPS: %3.0lf", fps);
		SetWindowText(window.getHwnd(), buffer);
	};

	window.show();

	window.runRenderingLoop();

	return 0;
}
