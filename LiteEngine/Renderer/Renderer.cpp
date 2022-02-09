#include "Renderer.h"
#include <DirectXTex.h>

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

	std::shared_ptr<ShaderResourceView> doCreateSimpleTexture2DFromWIC(
		DirectX::TexMetadata& meta,			// out_opt
		DirectX::ScratchImage& image,		// out
		Renderer& renderer,
		ID3D11Device* device
	) {
		// check
		auto imageData = image.GetImage(0, 0, 0);

		// Create texture
		D3D11_TEXTURE2D_DESC desc;
		desc.Width = (uint32_t)imageData->width;
		desc.Height = (uint32_t)imageData->height;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Format = imageData->format;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = 0;

		D3D11_SUBRESOURCE_DATA initData;
		initData.pSysMem = imageData->pixels;
		initData.SysMemPitch = static_cast<uint32_t>(imageData->rowPitch);
		initData.SysMemSlicePitch = static_cast<UINT>(imageData->slicePitch);

		ID3D11Texture2D* tex = nullptr;
		device->CreateTexture2D(&desc, &initData, &tex);

		CD3D11_SHADER_RESOURCE_VIEW_DESC descView(D3D11_SRV_DIMENSION_TEXTURE2D, imageData->format);
		image.Release();

		auto out = renderer.createShaderResourceView(tex, descView);

		tex->Release();

		return out;
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