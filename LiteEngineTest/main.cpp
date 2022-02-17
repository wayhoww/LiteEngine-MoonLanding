#include <Windows.h>
#include <wrl.h>

#include "LiteEngine/Window/RenderingWindow.h"
#include "LiteEngine/Renderer/Renderer.h"
#include "LiteEngine/Renderer/Resources.h"
#include "LiteEngine/Utilities/Utilities.h"
#include "LiteEngine/IO/DefaultLoader.h"
#include "LiteEngine/Renderer/Shadow.h"
#include "LiteEngine/IO/CameraController.h"

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
	auto mainCameraPitchLayer = mainCamera->insertParent();
	auto mainCameraYawLayer = mainCameraPitchLayer->insertParent();
	auto mainCameraPositionLayer = mainCameraYawLayer->insertParent();

	smScene.rootObject->dump();

	//// mainCamera->data.farZ = 1000;
	//mainCamera->fixedWorldUp = true;
	//mainCamera->worldUp = { 0, 0, 1 };

	auto probeCamera = smScene.search<lesm::Camera>("ProbeCamera");
	// probeCamera->data.farZ = 1000;
	if (probeCamera) {
		probeCamera->data.aspectRatio = 1;
	}
	auto skybox = renderer.createCubeMapFromDDS(L"skybox.dds");

	auto movingObject = smScene.searchObject("Icosphere.014");
	if (movingObject) {
		movingObject->moveParentCoord({ 0, 0, 0.5 });
	}

	auto renderableTexture = renderer.createRenderableTexture(1000, 1000, 1);
	
	auto screenPlane = smScene.searchObject("ScreenPlane");
	auto screenObj = screenPlane ? screenPlane->children[0] : nullptr;
	std::shared_ptr<lesm::DefaultMaterial> screenMat;
	if (screenObj) {
		auto screenPlane = std::dynamic_pointer_cast<lesm::Mesh>(screenObj);
		screenMat = std::dynamic_pointer_cast<lesm::DefaultMaterial>(screenPlane->material);
	}

	auto offscreenPassRastDesc = CD3D11_RASTERIZER_DESC(CD3D11_DEFAULT());
	offscreenPassRastDesc.CullMode = D3D11_CULL_NONE;
	auto offscreenPass = screenObj ? renderer.createRenderingPassWithoutSceneAndTarget(
		offscreenPassRastDesc,
		CD3D11_DEPTH_STENCIL_DESC(CD3D11_DEFAULT())
	) : nullptr;
	if (offscreenPass) {
		offscreenPass->renderTargetView = renderableTexture.renderTargetView;
		offscreenPass->depthStencilView = renderer.createDepthStencilView(1000, 1000);
		offscreenPass->clearStencil = false;

		D3D11_VIEWPORT viewport = {};
		viewport.Width = 1000;
		viewport.Height = 1000;
		viewport.MaxDepth = 1;
		viewport.MinDepth = 0;
		offscreenPass->viewport = viewport;
	}
	
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

	auto oldFOV = mainCamera->data.fieldOfViewYRadian;

	le::IO::HoldRotateController controller;
	le::IO::MoveController moveWS('W', 'S');
	le::IO::MoveController moveAD('A', 'D');
	le::IO::MoveController moveEC('E', 'C');

	window.renderCallback = [&](
		const le::RenderingWindow& window,
		const std::vector<le::RenderingWindow::EventType>& events
		) {

		auto duration = (float)(1. / renderer.getCurrentFPS());
		controller.receiveEvent(events, duration);
		moveWS.receiveEvent(events, duration);
		moveAD.receiveEvent(events, duration);
		moveEC.receiveEvent(events, duration);

		auto [accX, accY, accZ] = controller.getXYZ();

		float moveUnit = float(1. / 60 * 3);
		float rotateUnit = float(3.14159 / 60 / 10);

		if (movingObject) {
			movingObject->rotateLocalCoord({ 0, 0, 1 }, 0.02f / le::PI);
			movingObject->moveLocalCoord({ 0.02f, 0, 0 });
		}
		mainCameraYawLayer->moveLocalCoord({-moveAD.popValue(), moveEC.popValue(), moveWS.popValue()});
		// i.e. 先沿着 Z 轴转，再沿着旋转之后的新 X 轴转
		mainCameraYawLayer->transR = DirectX::XMQuaternionRotationAxis({ 0, -1, 0 }, accX);
		mainCameraPitchLayer->transR = DirectX::XMQuaternionRotationAxis({ -1, 0, 0 }, accY);

		mainCamera->data.fieldOfViewYRadian = (float)std::min<double>(le::PI * 0.8, ((accZ + 1) / 2 + 0.5) * oldFOV);

	//	mainCamera->rotateParentCoord(DirectX::XMQuaternionRotationRollPitchYaw(mouseDy * 0.001, mouseDx * 0.001, 0));
	/*	mainCamera->rotateLocalCoord(DirectX::XMQuaternionRotationAxis({0, -1, 0, 0}, rotateUnit * countRotateRight));
		mainCamera->rotateLocalCoord(DirectX::XMQuaternionRotationAxis({1, 0, 0, 0}, rotateUnit * countRotateUp));*/

		smScene.activeCamera = mainCamera;


		// 其实 getRenderingScene 可以在另外一个线程访问，完全不涉及图形 API
		std::shared_ptr<ler::RenderingScene> scene = smScene.getRenderingScene();
		auto mainRenderingCamera = scene->camera;

		// 
		auto cameras = ler::getSuggestedDepthCamera(mainRenderingCamera, scene->lights[0], { 0.1f, 1.f, 2.f, 4.f });
		// renderer.renderFrame(*scene);

		renderer.beginRendering();

		if (probeCamera) {
			offscreenPass->scene = scene;
			offscreenPass->scene->camera = smScene.getCameraInfo(probeCamera);
		}

		if (screenMat) {
			screenMat->texEmissionColor = nullptr;
			screenMat->constants->cpuData<lesm::DefaultMaterialConstantData>().uvEmissionColor = UINT32_MAX;
		}
		std::vector<std::shared_ptr<ler::RenderingPass>> passes;
		renderer.createShadowMapPasses(passes, scene, {scene->camera.nearZ, 3, 10, 30, 100});
		renderer.renderPasses(passes);

		if (offscreenPass) {
			renderer.enableBuiltinShadowMap(offscreenPass);
			renderer.renderPass(offscreenPass);
		}

		scene->camera = mainRenderingCamera;
		if (screenMat) {
			screenMat->texEmissionColor = renderableTexture.textureView;
			screenMat->constants->cpuData<lesm::DefaultMaterialConstantData>().uvEmissionColor = 0;
		}
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
