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
	constexpr uint32_t NUMBER_SHADOW_MAP_PER_LIGHT = 4;

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

	namespace TextureSlots {
		constexpr uint32_t CSM_DEPTH_MAP = 15;
	}

	namespace SamplerSlots {
		constexpr uint32_t CSM_DEPTH_MAP = 15;
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

			DirectX::XMMATRIX getV2CMatrix() const {
				if (projectionType == RenderingScene::CameraInfo::ProjectionType::PERSPECTIVE) {
					return DirectX::XMMatrixPerspectiveFovLH(fieldOfViewYRadian, aspectRatio, nearZ, farZ);
				}else {
					return DirectX::XMMatrixOrthographicLH(viewWidth, viewHeight, nearZ, farZ);
				}
			}
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
	};

	struct alignas(16) FixedPerframePSConstantBufferData {
		// NOTE: 不要忘记修改 hlsli
		DirectX::XMMATRIX trans_W2V;
		DirectX::XMMATRIX trans_V2W;

		float exposure;
		uint32_t numberOfLights;
	private:
		char space[2];
	public:

		LightDesc lights[MAX_NUMBER_OF_LIGHTS];

		float CSMZList[NUMBER_SHADOW_MAP_PER_LIGHT + 1][4];	// 1 2 3 are discarded..

		// order: light0: map0 map1 map2; light1: map0 map1 map2, ...
		uint32_t CSMValid[MAX_NUMBER_OF_LIGHTS][NUMBER_SHADOW_MAP_PER_LIGHT][4] = {};	// 1 2 3 are discarded..

		DirectX::XMMATRIX trans_W2CMS[MAX_NUMBER_OF_LIGHTS][NUMBER_SHADOW_MAP_PER_LIGHT];
	};

	struct alignas(16) FixedLongtermConstantBufferData {
		float widthInPixel;
		float heightInPixel;
		float desiredFPS;
		char _space[4];
	};

	struct PerpassModifiable {
		// scene 不会持有 pass，也不会持有 modifiable
		std::shared_ptr<RenderingScene> scene;
		FixedPerframeVSConstantBufferData* fixedPerframeVSConstants = nullptr;
		FixedPerframePSConstantBufferData* fixedPerframePSConstants = nullptr;
		void* customPerframeVSConstants = nullptr;
		void* customPerframePSConstants = nullptr;
	};

	struct RenderingPass {
		std::shared_ptr<RenderingScene> scene;
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView> renderTargetView;
		Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthStencilView;
		Microsoft::WRL::ComPtr<ID3D11RasterizerState> rasterizerState;
		Microsoft::WRL::ComPtr<ID3D11DepthStencilState> depthStencilState;
		
		std::shared_ptr<DepthTextureArray> CSMDepthMapArray;
		Microsoft::WRL::ComPtr<ID3D11SamplerState> CSMDepthMapSampler;

		bool clearDepth = true;
		bool clearStencil = true;
		float depthValue = 1;
		int stencilValue = 0;

		bool clearColor = false;
		DirectX::XMFLOAT4 colorValue{0, 0, 0, 0};

		D3D11_VIEWPORT viewport;

		std::string vsSemantic = ShaderSemantics::DEFAULT;
		std::string psSemantic = ShaderSemantics::DEFAULT;

		bool disableRendering = false;

		std::function<void(PerpassModifiable)> beforeRenderModifier = nullptr;
		std::function<void(PerpassModifiable)> afterRenderModifier = nullptr;
	};



	class Renderer {

	public:
		void createShadowMapPasses(
			std::vector<std::shared_ptr<RenderingPass>>& renderingPasses,
			std::shared_ptr<RenderingScene> scene,
			std::vector<float> zList,
			std::shared_ptr<DepthTextureArray> depthMap = nullptr
		);

		std::shared_ptr<RenderingPass> createDepthMapPass(
			std::shared_ptr<RenderingScene> scene,
			PtrDepthStencilView depthView,
			uint32_t width,
			uint32_t height
		) {
			
			static Microsoft::WRL::ComPtr<ID3D11RasterizerState> rasterizerState;
			static Microsoft::WRL::ComPtr<ID3D11DepthStencilState> depthStencilState;
			static std::once_flag once;

			std::call_once(once, [&]() {
				CD3D11_RASTERIZER_DESC rasterizerDesc{ CD3D11_DEFAULT{} };
				rasterizerDesc.CullMode = D3D11_CULL_FRONT;
				CD3D11_DEPTH_STENCIL_DESC depthStencilDesc{ CD3D11_DEFAULT{} };
				this->device->CreateRasterizerState(&rasterizerDesc, &rasterizerState);
				this->device->CreateDepthStencilState(&depthStencilDesc, &depthStencilState);
			});

			std::shared_ptr<RenderingPass> pass(new RenderingPass());
			pass->rasterizerState = rasterizerState;
			pass->depthStencilState = depthStencilState;
			pass->scene = scene;
			pass->renderTargetView = nullptr;
			pass->depthStencilView = depthView;
			pass->clearDepth = true;
			pass->clearStencil = false;

			D3D11_VIEWPORT viewport = {};
			viewport.Width = (float)width;
			viewport.Height = (float)height;
			viewport.MaxDepth = 1;
			viewport.MinDepth = 0;
			pass->viewport = viewport;

			pass->vsSemantic = ShaderSemantics::DEPTH_MAP;

			return pass;
		}

		std::shared_ptr<DepthTextureArray> createDepthTextureArray(uint32_t count, uint32_t width, uint32_t height) {
			std::shared_ptr<DepthTextureArray> out(new DepthTextureArray());
			out->depthBuffers.resize(count);
			out->width = width;
			out->height = height;

			constexpr auto DXGI_FORMAT_RESOURCE = DXGI_FORMAT_R24G8_TYPELESS;
			constexpr auto DXGI_FORMAT_DEPTH_STENCIL = DXGI_FORMAT_D24_UNORM_S8_UINT;
			constexpr auto DXGI_FORMAT_SHADER_RESOURCE = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
			constexpr uint32_t mipLevels = 1;

			ID3D11Texture2D* depthStencilBuffer;
			CD3D11_TEXTURE2D_DESC depthStencilTextureDesc(
				DXGI_FORMAT_RESOURCE, width, height, 
				count, mipLevels, D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE
			);

			device->CreateTexture2D(&depthStencilTextureDesc, nullptr, &depthStencilBuffer);
			
			for (uint32_t i = 0; i < count; i++) {
				CD3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc(
					D3D11_DSV_DIMENSION_TEXTURE2DARRAY,
					DXGI_FORMAT_DEPTH_STENCIL, 0, i, 1
				);
				device->CreateDepthStencilView(depthStencilBuffer, &depthStencilViewDesc, &out->depthBuffers[i]);
			}

			CD3D11_SHADER_RESOURCE_VIEW_DESC shaderResouceViewDesc {
				D3D11_SRV_DIMENSION_TEXTURE2DARRAY,
				DXGI_FORMAT_SHADER_RESOURCE, 0, mipLevels, 0, count 
			};

			device->CreateShaderResourceView(depthStencilBuffer, 
				static_cast<D3D11_SHADER_RESOURCE_VIEW_DESC*>(&shaderResouceViewDesc), 
				&out->textureArray);

			depthStencilBuffer->Release();

			return out;
		}

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

		std::shared_ptr<RenderingPass> getSkyboxRenderingPass(
			PtrShaderResourceView skyboxTexture,
			const RenderingScene::CameraInfo& camera,
			DirectX::XMMATRIX skyboxTransform
		);

		PtrDepthStencilView createDepthStencilView(int width, int height) {
			PtrDepthStencilView view;
			ID3D11Texture2D* depthStencilBuffer = nullptr;
			CD3D11_TEXTURE2D_DESC depthStencilTextureDesc(
				DXGI_FORMAT_D24_UNORM_S8_UINT,
				width,
				height, 1, 0, D3D11_BIND_DEPTH_STENCIL
			);
			device->CreateTexture2D(&depthStencilTextureDesc, nullptr, &depthStencilBuffer);
			CD3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc(
				D3D11_DSV_DIMENSION_TEXTURE2D,
				DXGI_FORMAT_D24_UNORM_S8_UINT
			);
			device->CreateDepthStencilView(depthStencilBuffer, &depthStencilViewDesc, &view);
			depthStencilBuffer->Release();
			return view;
		}

		PtrSamplerState getShadowMapSamplerState() {
			static auto depthMapSamplerState = this->createSamplerState([](){
				CD3D11_SAMPLER_DESC desc(CD3D11_DEFAULT{});
				desc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
				desc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
				desc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
				return desc;
			}());

			return depthMapSamplerState;
		}

	public:
		void enableBuiltinShadowMap(std::shared_ptr<RenderingPass> pass) {
			pass->CSMDepthMapArray = shadowDepthBuffer;
			pass->CSMDepthMapSampler = this->getShadowMapSamplerState();
		}

	protected:

		Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;

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

		std::shared_ptr<DepthTextureArray> shadowDepthBuffer = nullptr;

		uint32_t width = 0, height = 0;

		void updateFixedPerframeConstantBuffers(const RenderingScene& scene) const {
			auto& camera = scene.camera;

			auto transDet_V2W = DirectX::XMMatrixDeterminant(camera.trans_W2V);
			auto trans_V2W = DirectX::XMMatrixInverse(&transDet_V2W, camera.trans_W2V);

			{
				// perframe vs
				auto ar = camera.aspectRatio;
				auto& data = this->fixedPerframeVSConstantBuffer->cpuData<FixedPerframeVSConstantBufferData>();
				data.trans_W2V = camera.trans_W2V;
				data.trans_V2W = trans_V2W;
			
				data.trans_V2C = camera.getV2CMatrix();

				data.trans_W2C = DirectX::XMMatrixMultiply(data.trans_W2V, data.trans_V2C);

				this->fixedPerframeVSConstantBuffer->updateBuffer(this->context.Get());
			}

			{
				// perframe ps
				auto& camera = scene.camera;
				auto& data = this->fixedPerframePSConstantBuffer->cpuData<FixedPerframePSConstantBufferData>();
				data.trans_W2V = camera.trans_W2V;
				data.trans_V2W = trans_V2W;

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
		float exposure = 1.0;


	protected:
		Renderer(HWND windowHwnd) {
			// TODO MSAA. 这不是一个特别容易的事情。。

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
			this->recreateShadowDepthBuffer();

			this->fixedPerframeVSConstantBuffer = this->createConstantBuffer(FixedPerframeVSConstantBufferData());
			this->fixedPerframePSConstantBuffer = this->createConstantBuffer(FixedPerframePSConstantBufferData());
			// TODO: 下面的 designed fps 没有设置
			this->fixedLongtermConstantBuffer = this->createConstantBuffer(FixedLongtermConstantBufferData{ (float)this->width, (float)this->height, (float)30 });
		}

		void recreateDepthStencilView() {
			this->depthStencilView.Reset();
			ID3D11Texture2D* depthStencilBuffer = nullptr;
			CD3D11_TEXTURE2D_DESC depthStencilTextureDesc(
				DXGI_FORMAT_D24_UNORM_S8_UINT,
				this->width,
				this->height, 1, 0, D3D11_BIND_DEPTH_STENCIL
			);
			device->CreateTexture2D(&depthStencilTextureDesc, nullptr, &depthStencilBuffer);
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
			this->recreateShadowDepthBuffer();
		}

		// 第一次调用需要传递 hwnd 参数，之后调用
		static bool setHandle(HWND hwnd);
		static Renderer& getInstance();
		static HWND getHandle();

		// 资源创建过程都不加 const，方便以后记录状态。。
		std::shared_ptr<VertexShader> createVertexShader(
			const std::vector<uint8_t>& vertexShaderByteCode
		) {
			ID3D11VertexShader* vShader;
			device->CreateVertexShader(vertexShaderByteCode.data(), vertexShaderByteCode.size(), nullptr, &vShader);
			return std::shared_ptr<VertexShader>(new VertexShader(vShader, vertexShaderByteCode));
		}

		PtrPixelShader createPixelShader(
			const std::vector<uint8_t>& pixelShaderByteCode
		) {
			PtrPixelShader out;
			device->CreatePixelShader(pixelShaderByteCode.data(), pixelShaderByteCode.size(), nullptr, &out);
			return out;
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
			const uint32_t* indices, uint32_t lengthOfIndices,
			std::shared_ptr<VertexShader> vshader,
			PtrInputLayout inputLayout,
			std::shared_ptr<VertexShader> depthMapShader,
			PtrInputLayout depthMapInputLayout
		) {
			auto ido = this->createIndexBufferObject(indices, lengthOfIndices);
			
			return std::shared_ptr<Mesh>(new Mesh(
				vbo, ido, 0, lengthOfIndices,
				vshader, inputLayout, depthMapShader, depthMapInputLayout
			));
		}

		std::shared_ptr<Mesh> createMesh(
			std::shared_ptr<VertexBufferObject> vbo,
			const std::vector<uint32_t>& indices,
			std::shared_ptr<VertexShader> vshader,
			PtrInputLayout inputLayout,
			std::shared_ptr<VertexShader> depthMapShader,
			PtrInputLayout depthMapInputLayout
		) {
			return createMesh(vbo, indices.data(), (uint32_t)indices.size(), 
				vshader, inputLayout, depthMapShader, depthMapInputLayout);
		}

		std::shared_ptr<Mesh> createMesh(
			std::shared_ptr<VertexBufferObject> vbo,
			PtrIndexBufferObject ibo,
			uint32_t start,
			uint32_t count,
			std::shared_ptr<VertexShader> vshader,
			PtrInputLayout inputLayout,
			std::shared_ptr<VertexShader> depthMapShader,
			PtrInputLayout depthMapInputLayout
		) {
			return std::shared_ptr<Mesh>(
				new Mesh(vbo, ibo, start, count,
					vshader, inputLayout,
					depthMapShader, depthMapInputLayout)
			);
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

		PtrInputLayout createInputLayout(std::shared_ptr<InputElementDescriptions> desc, std::shared_ptr<VertexShader> shader) {
			// TODO cache.
			PtrInputLayout out;
			device->CreateInputLayout(desc->data(), (uint32_t)desc->size(),
				shader->vertexShaderByteCode.data(), shader->vertexShaderByteCode.size(), &out);
			return out;
		}

		std::shared_ptr<MeshObject> createMeshObject(
			std::shared_ptr<Mesh> mesh,
			std::shared_ptr<Material> material,
			std::shared_ptr<ConstantBuffer> customConstantBuffer = nullptr
		) {
			static auto fixedConstantBuffer = this->createConstantBuffer(FixedPerobjectConstantData());
			
			return std::shared_ptr<MeshObject>(new MeshObject(mesh, material, 
				fixedConstantBuffer->getSharedInstance(), customConstantBuffer));
		}

		void beginRendering() {
			if (this->autoAdjustSize) {
				this->resizeFitWindow();
			}

			this->clearShaderResourcesAndSamplers();

			float bgColor[4] = { 0, 0, 0, 1 };
			context->ClearRenderTargetView(this->renderTargetView.Get(), bgColor);
			context->ClearDepthStencilView(this->depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1, 0);
		}

		void renderPasses(
			const std::vector<std::shared_ptr<RenderingPass>>& passes,
			uint32_t start = 0,
			uint32_t length = UINT32_MAX
		) {
			auto it = passes.begin() + start;
			uint32_t count = 0;
			while (it < passes.end() && count++ < length) {
				this->renderPass(*it++);
			}
		}

		void renderPass(const std::shared_ptr<RenderingPass>& pass) {
			PerpassModifiable modifiableData;
			modifiableData.scene = pass->scene;
			modifiableData.fixedPerframePSConstants = &this->fixedPerframePSConstantBuffer
				->cpuData<FixedPerframePSConstantBufferData>();
			modifiableData.fixedPerframeVSConstants = &this->fixedPerframeVSConstantBuffer
				->cpuData<FixedPerframeVSConstantBufferData>();
			if(this->customPerframeVSConstantBuffer)
				modifiableData.customPerframeVSConstants = this->customPerframeVSConstantBuffer->getInternalData();
			if(this->customPerframePSConstantBuffer)
				modifiableData.customPerframePSConstants = this->customPerframePSConstantBuffer->getInternalData();
			
			if (pass->beforeRenderModifier) {
				pass->beforeRenderModifier(modifiableData);
			}

			if (!pass->disableRendering) {
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

				if (pass->CSMDepthMapArray) {
					context->PSSetShaderResources(TextureSlots::CSM_DEPTH_MAP, 1, pass->CSMDepthMapArray->textureArray.GetAddressOf());
					context->PSSetSamplers(SamplerSlots::CSM_DEPTH_MAP, 1, pass->CSMDepthMapSampler.GetAddressOf());
				}


				context->OMSetRenderTargets(1, pass->renderTargetView.GetAddressOf(), pass->depthStencilView.Get());
				context->OMSetDepthStencilState(pass->depthStencilState.Get(), 1);

				context->RSSetState(pass->rasterizerState.Get());

				this->setConstantBuffers();

				for (auto obj : pass->scene->meshObjects) {
					obj->draw(this->context.Get(), pass->vsSemantic);
				}

				this->clearShaderResourcesAndSamplers();

				ID3D11RenderTargetView* nullTarget = nullptr;
				context->OMSetRenderTargets(1, &nullTarget, nullptr);
			}

			if (pass->afterRenderModifier) {
				pass->afterRenderModifier(modifiableData);
			}
		}

	protected:
		void clearShaderResourcesAndSamplers() {
			static ID3D11ShaderResourceView* nullViews[16] = {};
			static ID3D11SamplerState* nullSamplers[16] = {};
			context->PSSetShaderResources(0, 16, nullViews);
			context->PSSetSamplers(0, 16, nullSamplers);
		}

		float shadowWidth, shadowHeight;
		void recreateShadowDepthBuffer() {
			this->shadowWidth = (float)this->width;
			this->shadowHeight = (float)this->height;	
			this->shadowDepthBuffer = this->createDepthTextureArray(
				MAX_NUMBER_OF_LIGHTS * NUMBER_SHADOW_MAP_PER_LIGHT, 
				(uint32_t)std::round(this->shadowWidth), 
				(uint32_t)std::round(this->shadowHeight)
			);
		}

	public:
		void renderScene(
			std::shared_ptr<RenderingScene> scene,
			bool renderShadow = true
		);

		void renderSkybox(
			PtrShaderResourceView& skyboxTexture,
			const RenderingScene::CameraInfo& camera,
			DirectX::XMMATRIX skyboxTransform = DirectX::XMMatrixIdentity()
		) {
			auto pass = this->getSkyboxRenderingPass(skyboxTexture, camera, skyboxTransform);
			this->renderPass(pass);
		}

		void swap() {
			swapChain->Present(1, 0);
		}

	};
}