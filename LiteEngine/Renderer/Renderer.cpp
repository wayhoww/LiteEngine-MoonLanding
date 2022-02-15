#include "Renderer.h"
#include <DirectXTex.h>
#include "../Utilities/Utilities.h"
#include "Shadow.h"

namespace LiteEngine::Rendering {

	static HWND rendererHandle = nullptr;

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
		if (image.GetImageCount() <= 0) {
			throw std::exception("no image is loaded");
		}
		CD3D11_SHADER_RESOURCE_VIEW_DESC descView(D3D11_SRV_DIMENSION_TEXTURE2D, meta.format);
		PtrShaderResourceView view;
		DirectX::CreateShaderResourceView(device, image.GetImages(), 1, meta, &view);
		return view;
	}

	PtrShaderResourceView Renderer::createSimpleTexture2DFromWIC(const std::wstring& file) {
		DirectX::WIC_FLAGS flags = DirectX::WIC_FLAGS_FORCE_RGB;
		DirectX::TexMetadata meta;			// out_opt
		DirectX::ScratchImage image;		// out
		DirectX::LoadFromWICFile(file.c_str(), flags, &meta, image);

		return doCreateSimpleTexture2DFromWIC(meta, image, *this, device.Get());
	}


	PtrShaderResourceView  Renderer::createSimpleTexture2DFromWIC(const uint8_t* memory, size_t size) {
		DirectX::WIC_FLAGS flags = DirectX::WIC_FLAGS_FORCE_RGB;
		DirectX::TexMetadata meta;			// out_opt
		DirectX::ScratchImage image;		// out
		DirectX::LoadFromWICMemory(memory, size, flags, &meta, image);

		return doCreateSimpleTexture2DFromWIC(meta, image, *this, device.Get());
	}

	void Renderer::renderScene(
		std::shared_ptr<RenderingScene> scene,
		bool renderShadow // = true
	) {
		if (renderShadow) {
			auto mainCamera = scene->camera;
			auto ratio = scene->camera.farZ / scene->camera.nearZ;
			const std::vector<float> zList{
				scene->camera.nearZ,
				3, 10, 30, 300
			};
			auto& constants = this->fixedPerframePSConstantBuffer->cpuData<FixedPerframePSConstantBufferData>();
			for (int i = 0; i < 4; i++) {
				constants.CSMZList[i][0] = zList[i];
			}

			int count = 0;
			for (uint32_t lightID = 0; lightID < (uint32_t)scene->lights.size(); lightID++) {

				auto maps = getSuggestedDepthCamera(mainCamera, scene->lights[lightID], zList);
				for (uint32_t mapID = 0; mapID < (uint32_t)maps.size(); mapID++) {
					auto& [feasible, lightCamera, z1, z2] = maps[mapID];
					if (!feasible) {
						constants.CSMValid[lightID][mapID][0] = false;
						continue;
					} 
					count += 1;
					constants.CSMValid[lightID][mapID][0] = true;
					constants.trans_W2CMS[lightID][mapID] = DirectX::XMMatrixMultiply(
						DirectX::XMMatrixMultiply(
							lightCamera.trans_W2V,
							lightCamera.getV2CMatrix()
						),
						DirectX::XMMATRIX {
							0.5, 0, 0, 0,
							0, -0.5, 0, 0,
							0, 0, 1.0, 0,
							0.5, 0.5, 0, 1
						}
					);

					scene->camera = lightCamera;

					auto shadowPass = this->createDepthMapPass(scene, 
						this->shadowDepthBuffer->depthBuffers[lightID * NUMBER_SHADOW_MAP_PER_LIGHT + mapID], 
						(uint32_t)std::round(this->shadowWidth), 
						(uint32_t)std::round(this->shadowHeight)
					);

					this->renderPass(shadowPass);

				}
			}	
			
			char buffer[10];
			sprintf_s(buffer, "%d\n", count);
			OutputDebugStringA(buffer);

			ID3D11RenderTargetView* nullViews = nullptr;
			context->OMSetRenderTargets(1, &nullViews, nullptr);

			scene->camera = mainCamera;
		}

		// const std::vector<float> zList = {}
		//auto shadowPass = this->createDepthMapPass(scene, texArray->depthBuffers[i]);
		static auto depthMapSamplerState = this->createSamplerState([](){
			CD3D11_SAMPLER_DESC desc(CD3D11_DEFAULT{});
			desc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
			desc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
			desc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
			return desc;
		}());

		auto mainPass = this->createDefaultRenderingPass(scene);
		mainPass->CSMDepthMapArray = this->shadowDepthBuffer;
		mainPass->CSMDepthMapSampler = depthMapSamplerState;
		this->renderPass(mainPass);
	}

}