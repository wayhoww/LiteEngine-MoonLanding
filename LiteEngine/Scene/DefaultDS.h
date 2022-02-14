#pragma once

#include <DirectXMath.h>
#include <memory>
#include <vector>
#include "../Renderer/Resources.h"
#include "Scene.h"

namespace LiteEngine::SceneManagement {

	enum class DefaultShaderSlot: uint32_t {
		BASE_COLOR, 
		EMISSION_COLOR,
		METALLIC,
		ROUGHNESS,
		AMBIENT_OCCLUSION,
		NORMAL
	};

#pragma pack(push, 1)
	struct DefaultVertexData {
		DirectX::XMFLOAT3 position{ 0, 0, 0 };
		DirectX::XMFLOAT3 normal{ 0, 1, 0 };
		DirectX::XMFLOAT3 tangent{ 1, 0, 0 };
		DirectX::XMFLOAT4 color{ 1, 1, 1, 1 };
		DirectX::XMFLOAT2 texCoord0{ 0, 0 };
		DirectX::XMFLOAT2 texCoord1{ 0, 0 };
	
		static std::shared_ptr<Rendering::InputElementDescriptions> getDescription() {
			static std::shared_ptr<Rendering::InputElementDescriptions> desc(
				new Rendering::InputElementDescriptions{
					std::vector<D3D11_INPUT_ELEMENT_DESC>{
						{ "POSITION",   0, DXGI_FORMAT_R32G32B32_FLOAT,     0, 0,                            D3D11_INPUT_PER_VERTEX_DATA, 0 },
						{ "NORMAL",     0, DXGI_FORMAT_R32G32B32_FLOAT,     0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
						{ "TANGENT",    0, DXGI_FORMAT_R32G32B32_FLOAT,     0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
						{ "COLOR",      0, DXGI_FORMAT_R32G32B32A32_FLOAT,     0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
						{ "TEXCOORD",   0, DXGI_FORMAT_R32G32_FLOAT,        0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
						{ "TEXCOORD",   1, DXGI_FORMAT_R32G32_FLOAT,        0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
					}
				});
			return desc;
		}
	};
#pragma pack(pop)

	struct alignas(16) DefaultMaterialConstantData {
		DirectX::XMFLOAT4 baseColor{1, 1, 1, 1};
		DirectX::XMFLOAT4 emissionColor{0, 0, 0, 1};
		
		float metallic = 0;
		float roughness = 1;
		float anisotropy = 0;
		float occlusionStrength = 1;

		float normalMapScale = 1;
		uint32_t uvBaseColor = UINT32_MAX;
		uint32_t uvEmissionColor = UINT32_MAX;
		uint32_t uvMetallic = UINT32_MAX;

		uint32_t uvRoughness = UINT32_MAX;
		uint32_t uvAO = UINT32_MAX;
		uint32_t uvNormal= UINT32_MAX;
		uint32_t channelRoughness= 0;

		uint32_t channelMetallic = 0;
		uint32_t channelAO = 0;
	};
	static_assert(sizeof(DefaultMaterialConstantData) == 6 * 4 * 4);

	struct DefaultMaterial : public Material {
		DefaultMaterial() {
			static auto shader = Rendering::Renderer::getInstance().createPixelShader(
				loadBinaryFromFile(L"DefaultPS.cso")
			);
			this->pixelShader = shader;
		}

		Rendering::PtrSamplerState sampBaseColor;
		Rendering::PtrSamplerState sampEmissionColor;
		Rendering::PtrSamplerState sampMetallic;
		Rendering::PtrSamplerState sampRoughness;
		Rendering::PtrSamplerState sampAO;
		Rendering::PtrSamplerState sampNormal;

		Rendering::PtrShaderResourceView texBaseColor;
		Rendering::PtrShaderResourceView texEmissionColor;
		Rendering::PtrShaderResourceView texMetallic;
		Rendering::PtrShaderResourceView texRoughness;
		Rendering::PtrShaderResourceView texAO;
		Rendering::PtrShaderResourceView texNormal;

		virtual std::vector<std::pair<Rendering::PtrShaderResourceView, uint32_t>> getShaderResourceViews() const {
			return {
				{ texBaseColor, (uint32_t)DefaultShaderSlot::BASE_COLOR },
				{ texEmissionColor, (uint32_t)DefaultShaderSlot::EMISSION_COLOR },
				{ texMetallic, (uint32_t)DefaultShaderSlot::METALLIC },
				{ texRoughness, (uint32_t)DefaultShaderSlot::ROUGHNESS },
				{ texAO, (uint32_t)DefaultShaderSlot::AMBIENT_OCCLUSION },
				{ texNormal, (uint32_t)DefaultShaderSlot::NORMAL }
			};
		}

		virtual std::vector<std::pair<Rendering::PtrSamplerState, uint32_t>> getSamplerStates() const {
			return {
				{ sampBaseColor, (uint32_t)DefaultShaderSlot::BASE_COLOR },
				{ sampEmissionColor, (uint32_t)DefaultShaderSlot::EMISSION_COLOR },
				{ sampMetallic, (uint32_t)DefaultShaderSlot::METALLIC },
				{ sampRoughness, (uint32_t)DefaultShaderSlot::ROUGHNESS },
				{ sampAO, (uint32_t)DefaultShaderSlot::AMBIENT_OCCLUSION },
				{ sampNormal, (uint32_t)DefaultShaderSlot::NORMAL }
			};
		}
	};
}