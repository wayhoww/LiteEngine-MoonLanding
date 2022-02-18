#include "Renderer.h"
#include <DirectXTex.h>
#include "../Utilities/Utilities.h"
#include "Shadow.h"

#pragma comment(lib, "runtimeobject")

namespace LiteEngine::Rendering {

	static HWND rendererHandle = nullptr;

	void initializeWRL() {
		static thread_local Microsoft::WRL::Wrappers::RoInitializeWrapper initialize(RO_INIT_MULTITHREADED);
	}

	bool Renderer::setHandle(HWND hwnd) {
		if (rendererHandle) return false;
		rendererHandle = hwnd;
		return true;
	}

	HWND Renderer::getHandle() {
		return rendererHandle;
	}

	Renderer& Renderer::getInstance() {
		if (rendererHandle) {
			static Renderer renderer(rendererHandle);
			return renderer;
		} else {
			throw std::exception("hwnd is not init");
		}
	}


	std::shared_ptr<MeshObject> createSkyboxMeshObject(Rendering::Renderer& renderer) {

		float cubeVertices[8 * 3] = {
			+1, +1, -1, +1, -1, -1, +1, +1, +1, +1, -1, +1,
			-1, +1, -1, -1, -1, -1, -1, +1, +1, -1, -1, +1,
		};

		std::shared_ptr<InputElementDescriptions> desc(
			new InputElementDescriptions(InputElementDescriptions({
				{ "POSITION",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0}}))
			);

		auto vbo = renderer.createVertexBufferObject(cubeVertices, 8, 3 * sizeof(float), desc);
		auto vshader = renderer.createVertexShader(loadBinaryFromFile(L"SkyboxVS.cso"));
		auto mesh = renderer.createMesh(
			vbo, {
				0, 4, 2, 3, 2, 7, 7, 6, 5, 5, 1, 7, 1, 0, 3, 5, 4, 1, 
				2, 4, 6, 7, 2, 6, 5, 6, 4, 7, 1, 3, 3, 0, 2, 1, 4, 0 },
			vshader, renderer.createInputLayout(desc, vshader),
			nullptr, nullptr
		);

		
		std::shared_ptr<StoredMaterial> material(new StoredMaterial());
		material->defaultShader = renderer.createPixelShader(loadBinaryFromFile(L"SkyboxPS.cso"));
		material->samplerStates = { { renderer.createSamplerState(CD3D11_SAMPLER_DESC(CD3D11_DEFAULT())), 0 } };
		material->constants = renderer.createConstantBuffer(DirectX::XMMatrixIdentity());

		auto obj = renderer.createMeshObject(mesh, material, nullptr);
		return obj;
	}

	std::shared_ptr<RenderingPass> Renderer::createDefaultRenderingPass(
		std::shared_ptr<RenderingScene>& scene
	) {

		static auto pass = this->createRenderingPassWithoutSceneAndTarget(
			[]() {
				CD3D11_RASTERIZER_DESC desc{CD3D11_DEFAULT()};
				desc.CullMode = D3D11_CULL_NONE;
				return desc;
			}(),
			CD3D11_DEPTH_STENCIL_DESC(CD3D11_DEFAULT())
		);
		pass->scene = scene;
		pass->renderTargetView = this->renderTargetView;
		pass->depthStencilView = this->depthStencilView;
		pass->clearStencil = false;

		D3D11_VIEWPORT viewport = {};
		viewport.Width = (float)this->width;
		viewport.Height = (float)this->height;
		viewport.MaxDepth = 1;
		viewport.MinDepth = 0;
		pass->viewport = viewport;

		pass->CSMDepthMapSampler = this->getShadowMapSamplerState();
		pass->CSMDepthMapArray = this->shadowDepthBuffer;

		return pass;
	}

	std::shared_ptr<RenderingPass> Renderer::createSkyboxRenderingPass(
		PtrShaderResourceView skyboxTexture,
		const RenderingScene::CameraInfo& camera,
		DirectX::XMMATRIX skyboxTransform
	) {
		static auto pass = this->createRenderingPassWithoutSceneAndTarget(
			CD3D11_RASTERIZER_DESC(CD3D11_DEFAULT()),
			[]() {
				auto desc = CD3D11_DEPTH_STENCIL_DESC(CD3D11_DEFAULT()); 
				desc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
				return desc;
			}()
		);

		static auto skyboxMeshObject = createSkyboxMeshObject(*this);

		std::dynamic_pointer_cast<StoredMaterial>(skyboxMeshObject->material)->shaderResourceViews = {{ skyboxTexture, 0 }};
		auto& trans_CubeMap = skyboxMeshObject->material->constants->cpuData<DirectX::XMMATRIX>();
		auto det = DirectX::XMMatrixDeterminant(skyboxTransform);
		trans_CubeMap = DirectX::XMMatrixInverse(&det, skyboxTransform);
		
		RenderingScene* scene = new RenderingScene();
		scene->camera = camera;
		scene->meshObjects = { skyboxMeshObject };

		pass->scene = std::shared_ptr<RenderingScene>(scene);
		pass->renderTargetView = this->renderTargetView;
		pass->depthStencilView = this->depthStencilView;
		pass->clearColor = false;
		pass->clearDepth = false;
		pass->clearStencil = false;

		D3D11_VIEWPORT viewport = {};
		viewport.Width = (float)this->width;
		viewport.Height = (float)this->height;
		viewport.MaxDepth = 1;
		viewport.MinDepth = 0;
		pass->viewport = viewport;

		return pass;
	}

	PtrShaderResourceView Renderer::createCubeMapFromDDS(const std::wstring& file) {
		initializeWRL();
		DirectX::DDS_FLAGS flags = DirectX::DDS_FLAGS_FORCE_RGB;
		DirectX::TexMetadata meta;			// out_opt
		DirectX::ScratchImage image;		// out
		DirectX::LoadFromDDSFile(file.c_str(), flags, &meta, image);
		auto count = image.GetImageCount();
		if (count != 6) {
			throw std::exception("shader map dds should contains exact 6 images.");
		}
		PtrShaderResourceView view = nullptr;
		DirectX::CreateShaderResourceView(device.Get(), image.GetImages(), 6, meta, &view);
		return view;
	}

	PtrShaderResourceView doCreateSimpleTexture2DFromWIC(
		DirectX::TexMetadata& meta,			// out_opt
		DirectX::ScratchImage& image,		// out
		Renderer& renderer,
		ID3D11Device* device
	) {
		initializeWRL();
		if (image.GetImageCount() <= 0) {
			throw std::exception("no image is loaded");
		}
		CD3D11_SHADER_RESOURCE_VIEW_DESC descView(D3D11_SRV_DIMENSION_TEXTURE2D, meta.format);
		PtrShaderResourceView view;
		DirectX::CreateShaderResourceView(device, image.GetImages(), 1, meta, &view);
		return view;
	}

	PtrShaderResourceView Renderer::createSimpleTexture2DFromWIC(const std::wstring& file) {
		initializeWRL();
		DirectX::WIC_FLAGS flags = DirectX::WIC_FLAGS_FORCE_RGB;
		DirectX::TexMetadata meta;			// out_opt
		DirectX::ScratchImage image;		// out
		DirectX::LoadFromWICFile(file.c_str(), flags, &meta, image);

		return doCreateSimpleTexture2DFromWIC(meta, image, *this, device.Get());
	}


	PtrShaderResourceView  Renderer::createSimpleTexture2DFromWIC(const uint8_t* memory, size_t size) {
		initializeWRL();
		DirectX::WIC_FLAGS flags = DirectX::WIC_FLAGS_FORCE_RGB;
		DirectX::TexMetadata meta;			// out_opt
		DirectX::ScratchImage image;		// out
		DirectX::LoadFromWICMemory(memory, size, flags, &meta, image);

		return doCreateSimpleTexture2DFromWIC(meta, image, *this, device.Get());
	}

	void Renderer::createShadowMapPasses(
		std::vector<std::shared_ptr<RenderingPass>>& renderingPasses,
		std::shared_ptr<RenderingScene> scene,
		std::vector<float> zList,
		std::shared_ptr<DepthTextureArray> depthMap
	) {
		if (depthMap == nullptr) {
			depthMap = this->shadowDepthBuffer;
		}

		auto mainCamera = scene->camera;
		auto ratio = scene->camera.farZ / scene->camera.nearZ;

		// 设置 constants
		std::shared_ptr<RenderingPass> constantSettingPass(new RenderingPass());
		constantSettingPass->disableRendering = true;
		constantSettingPass->afterRenderModifier = [=](PerpassModifiable data) {
			// todo 这里存 far 和 near 反而减少空间占用。。
			for (size_t i = 0; i < std::min<size_t>(NUMBER_SHADOW_MAP_PER_LIGHT + 1, zList.size()); i++) {
				data.fixedPerframePSConstants->CSMZList[i][0] = zList[i];
			}

			memset(data.fixedPerframePSConstants->CSMValid, 
				0, sizeof(data.fixedPerframePSConstants->CSMValid));
		};
		renderingPasses.push_back(constantSettingPass);

		// 渲染深度图
		for (uint32_t lightID = 0; lightID < (uint32_t)scene->lights.size(); lightID++) {

			auto maps = getSuggestedDepthCamera(mainCamera, scene->lights[lightID], zList);

			for (uint32_t mapID = 0; mapID < (uint32_t)maps.size(); mapID++) {
				auto& cameraDesc = maps[mapID];

				// 如果不合适用 CSM（光源在锥体内，或者 fov 会很大），那么跳过
				// CMSValid 默认初始化为了 false
				auto feasible = std::get<0>(cameraDesc);
				if (!feasible) continue;


				auto shadowPass = this->createDepthMapPass(scene, 
					this->shadowDepthBuffer->depthBuffers[lightID * NUMBER_SHADOW_MAP_PER_LIGHT + mapID], 
					(uint32_t)std::round(depthMap->width), 
					(uint32_t)std::round(depthMap->height)
				);

				// 渲染前，改相机
				shadowPass->beforeRenderModifier = [=](PerpassModifiable data) {
					data.scene->camera = std::get<1>(cameraDesc);
				};

				// 渲染后，改回来相机，并且把相关常数设置好
				shadowPass->afterRenderModifier = [=](PerpassModifiable data) {
					data.scene->camera = mainCamera;

					data.fixedPerframePSConstants->CSMValid[lightID][mapID][0] = true;
					data.fixedPerframePSConstants->trans_W2CMS[lightID][mapID] = DirectX::XMMatrixMultiply(
						DirectX::XMMatrixMultiply(
							std::get<1>(cameraDesc).trans_W2V,
							std::get<1>(cameraDesc).getV2CMatrix()
						), DirectX::XMMATRIX {
							0.5, 0, 0, 0,
							0, -0.5, 0, 0,
							0, 0, 1.0, 0,
							0.5, 0.5, 0, 1
						}
					);
				};

				renderingPasses.push_back(shadowPass);
			}
		}	
	}

	void Renderer::renderScene(
		std::shared_ptr<RenderingScene> scene,
		bool renderShadow // = true
	) {
		if (renderShadow) {
			std::vector<std::shared_ptr<RenderingPass>> passes;
			this->createShadowMapPasses(passes, scene, {
				scene->camera.nearZ, 3, 10, 30, 100	
			} /*, default to nullptr*/);

			this->renderPasses(passes);
		}
		
		auto mainPass = this->createDefaultRenderingPass(scene);
		this->renderPass(mainPass);
	}

}