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

		static float cubeVertices[8 * 3] = {
			+1, +1, -1, +1, -1, -1, +1, +1, +1, +1, -1, +1,
			-1, +1, -1, -1, -1, -1, -1, +1, +1, -1, -1, +1,
		};

		static std::shared_ptr<InputElementDescriptions> desc(
			new InputElementDescriptions(InputElementDescriptions({
				{ "POSITION",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0}}))
			);

		auto vbo = renderer.createVertexBufferObject(cubeVertices, 8, 3 * sizeof(float), desc);

		auto mesh = renderer.createMesh(vbo, {
			0, 4, 2, 3, 2, 7, 7, 6, 5, 5, 1, 7, 1, 0, 3, 5, 4, 1, 
			2, 4, 6, 7, 2, 6, 5, 6, 4, 7, 1, 3, 3, 0, 2, 1, 4, 0
		});

		auto shader = renderer.createShader(loadBinaryFromFile(L"SkyboxVS.cso"), loadBinaryFromFile( L"SkyboxPS.cso"));
		auto inputLayout = renderer.createInputLayout(desc, shader);

		std::shared_ptr<Material> material(new Material());
		material->shader = shader;
		material->samplerStates = { { renderer.createSamplerState(CD3D11_SAMPLER_DESC(CD3D11_DEFAULT())), 0 } };
		material->constants = renderer.createConstantBuffer(DirectX::XMMatrixIdentity());

		return renderer.createMeshObject(mesh, material, inputLayout, nullptr);
	}

	void Renderer::renderSkybox(const RenderingScene& scene) {
		auto skybox = scene.skybox;
		if (skybox == nullptr) return;

		static auto skyboxMeshObject = createSkyboxMeshObject(*this);

		skyboxMeshObject->material->shaderResourceViews = {{ skybox, 0 }};
		auto& trans_CubeMap = skyboxMeshObject->material->constants->cpuData<DirectX::XMMATRIX>();
		auto det = DirectX::XMMatrixDeterminant(scene.skyboxTransform);
		trans_CubeMap = DirectX::XMMatrixInverse(&det, scene.skyboxTransform);
		context->OMSetDepthStencilState(this->skyBoxDepthStencilState.Get(), 1);
		skyboxMeshObject->draw(this->context.Get());
	}

	std::shared_ptr<ShaderResourceView> Renderer::createCubeMapFromDDS(const std::wstring& file) {
		DirectX::DDS_FLAGS flags = DirectX::DDS_FLAGS_FORCE_RGB;
		DirectX::TexMetadata meta;			// out_opt
		DirectX::ScratchImage image;		// out
		DirectX::LoadFromDDSFile(file.c_str(), flags, &meta, image);
		auto count = image.GetImageCount();
		if (count != 6) {
			throw std::exception("shader map dds should contains exact 6 images.");
		}
		ID3D11ShaderResourceView* view = nullptr;
		DirectX::CreateShaderResourceView(device.Get(), image.GetImages(), 6, meta, &view);
		return std::shared_ptr<ShaderResourceView>(new ShaderResourceView(view));
	}

	std::shared_ptr<ShaderResourceView> doCreateSimpleTexture2DFromWIC(
		DirectX::TexMetadata& meta,			// out_opt
		DirectX::ScratchImage& image,		// out
		Renderer& renderer,
		ID3D11Device* device
	) {
		if (image.GetImageCount() <= 0) {
			throw std::exception("no image is loaded");
		}
		CD3D11_SHADER_RESOURCE_VIEW_DESC descView(D3D11_SRV_DIMENSION_TEXTURE2D, meta.format);
		ID3D11ShaderResourceView* view;
		DirectX::CreateShaderResourceView(device, image.GetImages(), 1, meta, &view);
		return std::shared_ptr<ShaderResourceView>(new ShaderResourceView(view));
	}

	std::shared_ptr<ShaderResourceView> Renderer::createSimpleTexture2DFromWIC(const std::wstring& file) {
		DirectX::WIC_FLAGS flags = DirectX::WIC_FLAGS_FORCE_RGB;
		DirectX::TexMetadata meta;			// out_opt
		DirectX::ScratchImage image;		// out
		DirectX::LoadFromWICFile(file.c_str(), flags, &meta, image);

		return doCreateSimpleTexture2DFromWIC(meta, image, *this, device.Get());
	}


	std::shared_ptr<ShaderResourceView> Renderer::createSimpleTexture2DFromWIC(const uint8_t* memory, size_t size) {
		DirectX::WIC_FLAGS flags = DirectX::WIC_FLAGS_FORCE_RGB;
		DirectX::TexMetadata meta;			// out_opt
		DirectX::ScratchImage image;		// out
		DirectX::LoadFromWICMemory(memory, size, flags, &meta, image);

		return doCreateSimpleTexture2DFromWIC(meta, image, *this, device.Get());
	}

}