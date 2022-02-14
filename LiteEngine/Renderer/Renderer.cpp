#include "Renderer.h"
#include <DirectXTex.h>
#include "../Utilities/Utilities.h"

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
		material->pixelShader = renderer.createPixelShader(loadBinaryFromFile(L"SkyboxPS.cso"));
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

}