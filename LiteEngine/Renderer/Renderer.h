#pragma once
#pragma comment(lib, "d3d11")

#include <wrl.h>
#include <Windows.h>

#include <vector>
#include <memory>
#include <iostream>

#include "Resources.h"

namespace LiteEngine::Rendering {

	constexpr uint32_t MAX_NUMBER_OF_LIGHTS = 4;

	namespace LightType {
		constexpr uint32_t LIGHT_TYPE_POINT = 0x0;
		constexpr uint32_t LIGHT_TYPE_DIRECTIONAL = 0x1;
		constexpr uint32_t LIGHT_TYPE_SPOT = 0x2;
	}

	namespace LightShadow {
		constexpr uint32_t LIGHT_SHADOW_EMPTY = 0x1;
		constexpr uint32_t LIGHT_SHADOW_HARD = 0x2;
		constexpr uint32_t LIGHT_SHADOW_SOFT = 0x3;	// TODO: unimplemented
	}


	struct alignas(16) LightDesc {
		// NOTE: 不要忘记修改 hlsli
		uint32_t type;						// all
		uint32_t shadow;					// all
		float innerConeAngle;			    // spot
		float outerConeAngle;				// spot
		
		DirectX::XMFLOAT3 position_W;		// spot & point
		float maximumDistance;				// spot & point
	
		DirectX::XMFLOAT3 direction_W;		// spot & directional
	private:
		float _space2;

	public:
		DirectX::XMFLOAT3 intensity;		// all
	};

	static_assert(sizeof(LightDesc) == 4 * 4 + 3 * 4 * 4, "`LightDesc` is not aligned correctly");

	struct RenderingScene {
		struct CameraInfo {
			enum class ProjectionType { PERSPECTIVE, ORTHOGRAPHICS };

			DirectX::XMMATRIX trans_W2V;		// world to view transformation
			ProjectionType projectionType = ProjectionType::PERSPECTIVE;

			union {
				// perspective only
				struct {
					float fieldOfViewYRadian;
					float aspectRatio;	// width / height
				};

				// ortho only
				struct {
					float viewWidth;
					float viewHeight;
				};
			};

			float nearZ;
			float farZ;

		};

		CameraInfo camera;

		std::vector<LightDesc> lights;
		std::vector<std::shared_ptr<MeshObject>> meshObjects;

		// also probe and many other..

	};

	struct alignas(16) FixedPerframeVSConstantBufferData {
		// NOTE: 不要忘记修改 hlsli
		DirectX::XMMATRIX trans_W2V;
		DirectX::XMMATRIX trans_V2W;
		DirectX::XMMATRIX trans_V2C;
		DirectX::XMMATRIX trans_W2C;
		float timeInSecond;
		float currentFPS;
	};

	struct alignas(16) FixedPerframePSConstantBufferData {
		// NOTE: 不要忘记修改 hlsli
		DirectX::XMMATRIX trans_W2V;
		DirectX::XMMATRIX trans_V2W;

		float timeInSecond;
		float currentFPS;
		float exposure;
		uint32_t numberOfLights;

		LightDesc lights[MAX_NUMBER_OF_LIGHTS];
	};

	struct alignas(16) FixedLongtermConstantBufferData {
		float widthInPixel;
		float heightInPixel;
		float desiredFPS;
		char _space[4];
	};

	// TODO COM 内存泄漏检测

	struct RenderingPass {
		std::shared_ptr<RenderingScene> scene;
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView> renderTargetView;
		Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthStencilView;
		Microsoft::WRL::ComPtr<ID3D11RasterizerState> rasterizerState;
		Microsoft::WRL::ComPtr<ID3D11DepthStencilState> depthStencilState;

		bool clearDepth = true;
		bool clearStencil = true;
		float depthValue = 1;
		int stencilValue = 0;

		bool clearColor = false;
		DirectX::XMFLOAT4 colorValue;

		D3D11_VIEWPORT viewport;
	};

	class Renderer {

	public:
		std::shared_ptr<RenderingPass> createRenderingPassWithoutSceneAndTarget(
			D3D11_RASTERIZER_DESC rasterizerDesc,
			D3D11_DEPTH_STENCIL_DESC depthStencilDesc
		) {
			std::shared_ptr<RenderingPass> pass(new RenderingPass());
			this->device->CreateRasterizerState(&rasterizerDesc, &pass->rasterizerState);
			this->device->CreateDepthStencilState(&depthStencilDesc, &pass->depthStencilState);

			return pass;
		}

		std::shared_ptr<RenderingPass> createDefaultRenderingPass(
			std::shared_ptr<RenderingScene>& scene
		);

		std::shared_ptr<RenderingPass> createSkyboxRenderingPass(
			PtrShaderResourceView skyboxTexture,
			const RenderingScene::CameraInfo& camera,
			DirectX::XMMATRIX skyboxTransform
		);

		PtrDepthStencilView createDepthStencilView(int width, int height) {
			PtrDepthStencilView view;
			ID3D11Texture2D* depthStencilBuffer = NULL;
			CD3D11_TEXTURE2D_DESC depthStencilTextureDesc(
				DXGI_FORMAT_D24_UNORM_S8_UINT,
				width,
				height, 1, 0, D3D11_BIND_DEPTH_STENCIL
			);
			device->CreateTexture2D(&depthStencilTextureDesc, NULL, &depthStencilBuffer);
			CD3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc(
				D3D11_DSV_DIMENSION_TEXTURE2D,
				DXGI_FORMAT_D24_UNORM_S8_UINT
			);
			device->CreateDepthStencilView(depthStencilBuffer, &depthStencilViewDesc, &view);
			depthStencilBuffer->Release();
			return view;
		}

		Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
	protected:


		Microsoft::WRL::ComPtr<ID3D11Device> device;
		Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain;
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView> renderTargetView;
		Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthStencilView;

		std::shared_ptr<ConstantBuffer> fixedPerframeVSConstantBuffer;
		std::shared_ptr<ConstantBuffer> fixedPerframePSConstantBuffer;
		std::shared_ptr<ConstantBuffer> fixedLongtermConstantBuffer;

		std::shared_ptr<ConstantBuffer> customPerframeVSConstantBuffer;
		std::shared_ptr<ConstantBuffer> customPerframePSConstantBuffer;
		std::shared_ptr<ConstantBuffer> customLongtermVSConstantBuffer;
		std::shared_ptr<ConstantBuffer> customLongtermPSConstantBuffer;

		uint32_t width = 0, height = 0;

		int64_t time_in_ticks = 0;
		int64_t timer_frequency;
		double currentFPS = -1;
		double averageFPS = -1;
		double desiredFPS = 0;

		uint64_t id = 0;

		void updateFixedPerframeConstantBuffers(const RenderingScene& scene) const {

			float timeInSecond = 1.0f * time_in_ticks / timer_frequency;
			float currentFPS = (float)this->currentFPS;

			auto& camera = scene.camera;

			auto transDet_V2W = DirectX::XMMatrixDeterminant(camera.trans_W2V);
			auto trans_V2W = DirectX::XMMatrixInverse(&transDet_V2W, camera.trans_W2V);

			{
				// perframe vs
				auto ar = camera.aspectRatio;
				if (autoAdjustAspectRatio) ar = float(1.0 * this->width / this->height);
				auto& data = this->fixedPerframeVSConstantBuffer->cpuData<FixedPerframeVSConstantBufferData>();
				data.trans_W2V = camera.trans_W2V;
				data.trans_V2W = trans_V2W;

				if (camera.projectionType == RenderingScene::CameraInfo::ProjectionType::PERSPECTIVE) {
					data.trans_V2C = DirectX::XMMatrixPerspectiveFovLH(camera.fieldOfViewYRadian, ar, camera.nearZ, camera.farZ);
				}else {
					data.trans_V2C = DirectX::XMMatrixOrthographicLH(camera.viewWidth, camera.viewHeight, camera.nearZ, camera.farZ);
				}

				data.trans_W2C = DirectX::XMMatrixMultiply(data.trans_W2V, data.trans_V2C);
				data.timeInSecond = timeInSecond;
				data.currentFPS = currentFPS;

				this->fixedPerframeVSConstantBuffer->updateBuffer(this->context.Get());
			}

			{
				// perframe ps
				auto& camera = scene.camera;
				auto& data = this->fixedPerframePSConstantBuffer->cpuData<FixedPerframePSConstantBufferData>();
				data.trans_W2V = camera.trans_W2V;
				data.trans_V2W = trans_V2W;

				data.timeInSecond = timeInSecond;
				data.currentFPS = currentFPS;
				data.exposure = this->exposure;

				auto numberOfLights = scene.lights.size();
				if (numberOfLights > 4) {
					// TODO: log warning..
					numberOfLights = 4;
				}
				data.numberOfLights = (uint32_t)numberOfLights;
				memcpy(data.lights, scene.lights.data(), sizeof(LightDesc) * numberOfLights);

				this->fixedPerframePSConstantBuffer->updateBuffer(this->context.Get());
			}
		}

		void setConstantBuffers() const {
			context->VSSetConstantBuffers(VSConstantBufferSlotID::PERFRAME_FIXED, 1, fixedPerframeVSConstantBuffer->getAddressOf());
			context->PSSetConstantBuffers(PSConstantBufferSlotID::PERFRAME_FIXED, 1, fixedPerframePSConstantBuffer->getAddressOf());

			if (customPerframeVSConstantBuffer) context->VSSetConstantBuffers(VSConstantBufferSlotID::PERFRAME_CUSTOM, 1, customPerframeVSConstantBuffer->getAddressOf());
			if (customPerframePSConstantBuffer) context->PSSetConstantBuffers(PSConstantBufferSlotID::PERFRAME_CUSTOM, 1, customPerframePSConstantBuffer->getAddressOf());

			context->VSSetConstantBuffers(VSConstantBufferSlotID::LONGTERM_FIXED, 1, fixedLongtermConstantBuffer->getAddressOf());
			context->PSSetConstantBuffers(PSConstantBufferSlotID::LONGTERM_FIXED, 1, fixedLongtermConstantBuffer->getAddressOf());

			if (customLongtermVSConstantBuffer) context->VSSetConstantBuffers(VSConstantBufferSlotID::LONGTERM_CUSTOM, 1, customLongtermVSConstantBuffer->getAddressOf());
			if (customLongtermPSConstantBuffer) context->PSSetConstantBuffers(PSConstantBufferSlotID::LONGTERM_CUSTOM, 1, customLongtermPSConstantBuffer->getAddressOf());
		}

	public:
		Renderer(const Renderer&) = delete;
		void operator=(const Renderer&) = delete;

		bool autoAdjustSize = false;
		bool autoAdjustAspectRatio = false;
		float exposure = 1.0;


	protected:
		Renderer(HWND windowHwnd) {
			// TODO MSAA. 这不是一个特别容易的事情。。
			static uint64_t idCounter = 0;
			this->id = idCounter++;

			RECT rect;
			GetClientRect(windowHwnd, &rect);

			this->width = rect.right - rect.left;
			this->height = rect.bottom - rect.top;

			D3D_FEATURE_LEVEL accepted_feature_levels[] = {
				D3D_FEATURE_LEVEL_11_0,
				D3D_FEATURE_LEVEL_11_1
			};
			D3D_FEATURE_LEVEL feature_level_taken = {};
			DXGI_SWAP_CHAIN_DESC swap_chain_desc = {};
			swap_chain_desc.BufferCount = 2;
			swap_chain_desc.BufferDesc.Height = this->height;
			swap_chain_desc.BufferDesc.Width = this->width;
			// RGBA（没有SRGB）: pixel shader 应当返回光强
			swap_chain_desc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
			swap_chain_desc.BufferDesc.RefreshRate.Numerator = 0;
			swap_chain_desc.BufferDesc.RefreshRate.Denominator = 1;
			swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			swap_chain_desc.OutputWindow = windowHwnd;
			swap_chain_desc.Windowed = true;
			swap_chain_desc.SampleDesc.Count = 1;
			swap_chain_desc.SampleDesc.Quality = 0;
			swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

			UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#if defined(DEBUG) || defined(_DEBUG)
			flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

			D3D11CreateDeviceAndSwapChain(
				nullptr,
				D3D_DRIVER_TYPE_HARDWARE,
				nullptr,
				flags,
				accepted_feature_levels, ARRAYSIZE(accepted_feature_levels),
				D3D11_SDK_VERSION,
				&swap_chain_desc, &this->swapChain,
				&this->device,
				&feature_level_taken,
				&this->context
			);

			ID3D11Texture2D* frameBuffer;
			this->swapChain->GetBuffer(0, IID_PPV_ARGS(&frameBuffer));
			this->device->CreateRenderTargetView(frameBuffer, nullptr, &this->renderTargetView);
			frameBuffer->Release();
			
			this->recreateDepthStencilView();

			this->fixedPerframeVSConstantBuffer = this->createConstantBuffer(FixedPerframeVSConstantBufferData());
			this->fixedPerframePSConstantBuffer = this->createConstantBuffer(FixedPerframePSConstantBufferData());
			// TODO: 下面的 designed fps 没有设置
			this->fixedLongtermConstantBuffer = this->createConstantBuffer(FixedLongtermConstantBufferData{ (float)this->width, (float)this->height, (float)30 });

			LARGE_INTEGER frequency;
			QueryPerformanceFrequency(&frequency);
			this->timer_frequency = frequency.QuadPart;
		}

		void recreateDepthStencilView() {
			this->depthStencilView.Reset();
			ID3D11Texture2D* depthStencilBuffer = NULL;
			CD3D11_TEXTURE2D_DESC depthStencilTextureDesc(
				DXGI_FORMAT_D24_UNORM_S8_UINT,
				this->width,
				this->height, 1, 0, D3D11_BIND_DEPTH_STENCIL
			);
			device->CreateTexture2D(&depthStencilTextureDesc, NULL, &depthStencilBuffer);
			CD3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc(
				D3D11_DSV_DIMENSION_TEXTURE2D,
				DXGI_FORMAT_D24_UNORM_S8_UINT
			);
			device->CreateDepthStencilView(depthStencilBuffer, &depthStencilViewDesc, &this->depthStencilView);
			depthStencilBuffer->Release();
		}

	
	public:

		void resizeFitWindow() {
			RECT rect;
			GetClientRect(getHandle(), &rect);
			if (rect.bottom - rect.top == height && rect.right - rect.left == width) return;
			this->height = rect.bottom - rect.top;
			this->width = rect.right - rect.left;

			// https://docs.microsoft.com/en-us/windows/win32/direct3ddxgi/d3d10-graphics-programming-guide-dxgi#handling-window-resizing
			context->OMSetRenderTargets(0, 0, 0);

			this->renderTargetView.Reset();

			this->swapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);

			ID3D11Texture2D* frameBuffer;
			this->swapChain->GetBuffer(0, IID_PPV_ARGS(&frameBuffer));
			this->device->CreateRenderTargetView(frameBuffer, nullptr, &this->renderTargetView);
			frameBuffer->Release();
			
			this->recreateDepthStencilView();
		}

		// 第一次调用需要传递 hwnd 参数，之后调用
		static bool setHandle(HWND hwnd);
		static Renderer& getInstance();
		static HWND getHandle();

		// 资源创建过程都不加 const，方便以后记录状态。。
		std::shared_ptr<Shader> createShader(
			const std::vector<uint8_t>& vertexShaderByteCode,
			const std::vector<uint8_t>& pixelShaderByteCode
		) {
			ID3D11VertexShader* vShader;
			ID3D11PixelShader* pShader;
			device->CreateVertexShader(vertexShaderByteCode.data(), vertexShaderByteCode.size(), nullptr, &vShader);
			device->CreatePixelShader(pixelShaderByteCode.data(), pixelShaderByteCode.size(), nullptr, &pShader);

			return std::shared_ptr<Shader>(new Shader(vShader, pShader, vertexShaderByteCode));
		}

		// IndexBufferObject
		PtrIndexBufferObject createIndexBufferObject(
			const uint32_t* indices,
			uint32_t count
		) {
			PtrIndexBufferObject indexBuffer;
			CD3D11_BUFFER_DESC indexBufferDesc(count * sizeof(*indices), D3D11_BIND_INDEX_BUFFER);
			D3D11_SUBRESOURCE_DATA indexSubresData = {};
			indexSubresData.pSysMem = indices;
			device->CreateBuffer(&indexBufferDesc, &indexSubresData, &indexBuffer);
			return indexBuffer;
		}

		PtrIndexBufferObject createIndexBufferObject(
			const std::vector<uint32_t>& indices
		) {
			return this->createIndexBufferObject(indices.data(), (uint32_t)indices.size());
		}

		std::shared_ptr<VertexBufferObject> createVertexBufferObject(
			const void* vertices,
			uint32_t count, uint32_t elementSize,
			std::shared_ptr<InputElementDescriptions> desc
		) {
			ID3D11Buffer* vertexBuffer;
			CD3D11_BUFFER_DESC vertexBufferDesc(count * elementSize, D3D11_BIND_VERTEX_BUFFER);
			D3D11_SUBRESOURCE_DATA vertexSubresData = {};
			vertexSubresData.pSysMem = vertices;
			device->CreateBuffer(&vertexBufferDesc, &vertexSubresData, &vertexBuffer);

			return std::shared_ptr<VertexBufferObject>(new VertexBufferObject(vertexBuffer, desc, elementSize));
		}

		template<typename ElementType>
		std::shared_ptr<VertexBufferObject> createVertexBufferObject(
			const std::vector<ElementType> vertices,
			std::shared_ptr<InputElementDescriptions> desc
		) {
			return createVertexBufferObject(vertices.data(), (uint32_t)vertices.size(), (uint32_t)sizeof(ElementType), desc);
		}

		template<typename DataType>
		std::shared_ptr<ConstantBuffer> createConstantBuffer(const DataType& init) {
			ID3D11Buffer* buffer;
			CD3D11_BUFFER_DESC desc(sizeof(DataType), D3D11_BIND_CONSTANT_BUFFER);
			device->CreateBuffer(&desc, nullptr, &buffer);

			return std::shared_ptr<ConstantBuffer>(new ConstantBuffer(init, buffer));
		}


		std::shared_ptr<Mesh> createMesh(
			std::shared_ptr<VertexBufferObject> vbo,
			const uint32_t* indices, uint32_t lengthOfIndices
		) {
			auto ido = this->createIndexBufferObject(indices, lengthOfIndices);
			
			return std::shared_ptr<Mesh>(new Mesh(vbo, ido, 0, lengthOfIndices));
		}

		std::shared_ptr<Mesh> createMesh(
			std::shared_ptr<VertexBufferObject> vbo,
			const std::vector<uint32_t>& indices
		) {
			return createMesh(vbo, indices.data(), (uint32_t)indices.size());
		}

		std::shared_ptr<Mesh> createMesh(
			std::shared_ptr<VertexBufferObject> vbo,
			PtrIndexBufferObject ibo,
			uint32_t start,
			uint32_t count
		) {
			return std::shared_ptr<Mesh>(new Mesh(vbo, ibo, start, count));
		}

		PtrSamplerState createSamplerState(D3D11_SAMPLER_DESC desc) {
			PtrSamplerState state;
			device->CreateSamplerState(&desc, &state);
			return state;
		}

		PtrShaderResourceView createShaderResourceView(
			ID3D11Resource* resource,
			const D3D11_SHADER_RESOURCE_VIEW_DESC& desc
		) {
			PtrShaderResourceView view;
			device->CreateShaderResourceView(resource, &desc, &view);
			return view;
		}

		RenderableTexture createRenderableTexture(
			int width,
			int height,
			int mipLevels = 1
		) {
			RenderableTexture rt;

			D3D11_TEXTURE2D_DESC textureDesc = CD3D11_TEXTURE2D_DESC(
				DXGI_FORMAT_R8G8B8A8_UNORM,
				width,
				height,
				1,				// count
				mipLevels,
				D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET
			);
			ID3D11Texture2D* frameBuffer = nullptr;
			this->device->CreateTexture2D(&textureDesc, nullptr, &frameBuffer);
			this->device->CreateRenderTargetView(frameBuffer, nullptr, rt.renderTargetView.GetAddressOf());
			frameBuffer->Release();

			D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc = CD3D11_SHADER_RESOURCE_VIEW_DESC(
				D3D_SRV_DIMENSION_TEXTURE2D,
				DXGI_FORMAT_R8G8B8A8_UNORM,
				0,
				mipLevels
			);
			device->CreateShaderResourceView(frameBuffer, &viewDesc, &rt.textureView);
			
			return rt;
		}

		// 大小为 1 的 texture，写 shader 的时候不用因为各种 texture 的有无而排列组合。。
		PtrShaderResourceView createDefaultTexture(
			const DirectX::XMFLOAT4& data = DirectX::XMFLOAT4(1, 1, 1, 1),
			int textureDim = 2
		) {
			assert(textureDim == 0 || textureDim == 1 || textureDim == 2);

			D3D11_TEXTURE2D_DESC desc;
			desc.Width = 1;
			desc.Height = 1;
			desc.MipLevels = 1;
			desc.ArraySize = 1;
			desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
			desc.SampleDesc.Count = 1;
			desc.SampleDesc.Quality = 0;
			desc.Usage = D3D11_USAGE_DEFAULT;
			desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
			desc.CPUAccessFlags = 0;
			desc.MiscFlags = 0;

			D3D11_SUBRESOURCE_DATA initData;
			initData.pSysMem = &data;
			initData.SysMemPitch = sizeof(float) * 4;
			initData.SysMemSlicePitch = sizeof(float) * 4;

			ID3D11Texture2D* tex = nullptr;
			device->CreateTexture2D(&desc, &initData, &tex);

			CD3D11_SHADER_RESOURCE_VIEW_DESC descView(D3D11_SRV_DIMENSION_TEXTURE2D, DXGI_FORMAT_R32G32B32A32_FLOAT);

			auto out = createShaderResourceView(tex, descView);

			tex->Release();

			return out;
		}

	public:
		PtrShaderResourceView createSimpleTexture2DFromWIC(const std::wstring& file);
		PtrShaderResourceView createSimpleTexture2DFromWIC(const uint8_t* memory, size_t size);
		PtrShaderResourceView createCubeMapFromDDS(const std::wstring& file);

		PtrInputLayout createInputLayout(std::shared_ptr<InputElementDescriptions> desc, std::shared_ptr<Shader> shader) {
			// TODO cache.
			PtrInputLayout out;
			device->CreateInputLayout(desc->data(), (uint32_t)desc->size(),
				shader->vertexShaderByteCode.data(), shader->vertexShaderByteCode.size(), &out);
			return out;
		}

		std::shared_ptr<MeshObject> createMeshObject(
			std::shared_ptr<Mesh> mesh,
			std::shared_ptr<Material> material,
			PtrInputLayout inputLayout,
			std::shared_ptr<ConstantBuffer> customConstantBuffer = nullptr
		) {
			static auto fixedConstantBuffer = this->createConstantBuffer(FixedPerobjectConstantData());
			
			return std::shared_ptr<MeshObject>(new MeshObject(mesh, material,
				inputLayout, fixedConstantBuffer->getSharedInstance(), customConstantBuffer));
		}

		double getCurrentFPS() const {
			return this->currentFPS;
		}

		double getAverageFPS() const {
			return this->averageFPS;
		}

		void beginRendering() {
			if (this->autoAdjustSize) {
				this->resizeFitWindow();
			}

			// https://docs.microsoft.com/en-us/windows/win32/sysinfo/acquiring-high-resolution-time-stamps
			LARGE_INTEGER currentTime;
			QueryPerformanceCounter(&currentTime);
			auto elapsedTicks = currentTime.QuadPart - this->time_in_ticks;
			this->time_in_ticks = currentTime.QuadPart;

			if (this->currentFPS < 0) {
				this->currentFPS = this->desiredFPS > 0 ? this->desiredFPS : 60;
				this->averageFPS = this->currentFPS;
			} else {
				this->currentFPS = 1.0 / elapsedTicks * this->timer_frequency;
				this->averageFPS = this->averageFPS * 0.95 + this->currentFPS * (1 - 0.95);
			}
		}


		void renderPass(std::shared_ptr<RenderingPass>& pass) {		
			context->RSSetViewports(1, &pass->viewport);

			if (pass->clearColor) {
				context->ClearRenderTargetView(pass->renderTargetView.Get(), reinterpret_cast<float*>(&pass->colorValue));
			}
			D3D11_CLEAR_FLAG clearFlag = D3D11_CLEAR_FLAG(0);
			if (pass->clearDepth) clearFlag = D3D11_CLEAR_FLAG(clearFlag | D3D11_CLEAR_DEPTH);
			if (pass->clearStencil) clearFlag = D3D11_CLEAR_FLAG(clearFlag | D3D11_CLEAR_STENCIL);

			if (clearFlag) {
				context->ClearDepthStencilView(pass->depthStencilView.Get(), clearFlag, pass->depthValue, pass->stencilValue);
			}

			this->updateFixedPerframeConstantBuffers(*pass->scene);

			context->OMSetRenderTargets(1, pass->renderTargetView.GetAddressOf(), pass->depthStencilView.Get());
			context->OMSetDepthStencilState(pass->depthStencilState.Get(), 1);

			context->RSSetState(pass->rasterizerState.Get());
			
			this->setConstantBuffers();

			for (auto obj : pass->scene->meshObjects) {
				obj->draw(this->context.Get());
			}
		}

		void renderScene(std::shared_ptr<RenderingScene> scene) {
			auto pass = this->createDefaultRenderingPass(scene);
			this->renderPass(pass);
		}

		void renderSkybox(
			PtrShaderResourceView& skyboxTexture,
			const RenderingScene::CameraInfo& camera,
			DirectX::XMMATRIX skyboxTransform = DirectX::XMMatrixIdentity()
		) {
			auto pass = this->createSkyboxRenderingPass(skyboxTexture, camera, skyboxTransform);
			this->renderPass(pass);
		}

		void swap() {
			swapChain->Present(0, 0);
		}

	};
}