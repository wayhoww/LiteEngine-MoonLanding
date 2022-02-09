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
		uint32_t uvBaseColor = UINT32_MAX;
		
		uint32_t uvEmissionColor = UINT32_MAX;
		uint32_t uvMetallic = UINT32_MAX;
		uint32_t uvRoughness = UINT32_MAX;
		uint32_t uvAO = UINT32_MAX;

		uint32_t uvNormal= UINT32_MAX;
	};
	static_assert(sizeof(DefaultMaterialConstantData) == 5 * 4 * 4);

	struct DefaultMaterial : public Material {
		DefaultMaterial() {
			auto& renderer = Rendering::Renderer::getInstance();
			static std::shared_ptr<Rendering::Shader> shader(renderer.createShader(
				loadBinaryFromFile(L"DefaultVS.cso"),
				loadBinaryFromFile(L"DefaultPS.cso")
			));
			this->shader = shader;
		}

		std::shared_ptr<Rendering::SamplerState> sampBaseColor;
		// std::shared_ptr<Rendering::SamplerState> sampBaseColor;
		// std::shared_ptr<Rendering::SamplerState> sampBaseColor;
		// std::shared_ptr<Rendering::SamplerState> sampBaseColor;
		// std::shared_ptr<Rendering::SamplerState> sampBaseColor;
		// std::shared_ptr<Rendering::SamplerState> sampBaseColor;

		std::shared_ptr<Rendering::ShaderResourceView> texBaseColor;
		std::shared_ptr<Rendering::ShaderResourceView> texEmissionColor;
		std::shared_ptr<Rendering::ShaderResourceView> texMetallic;
		std::shared_ptr<Rendering::ShaderResourceView> texRoughness;
		std::shared_ptr<Rendering::ShaderResourceView> texAO;
		std::shared_ptr<Rendering::ShaderResourceView> texNormal;

		virtual std::vector<std::pair<std::shared_ptr<Rendering::ShaderResourceView>, uint32_t>> getShaderResourceViews() const {
			return { {texBaseColor, (uint32_t)DefaultShaderSlot::BASE_COLOR} };
		}

		virtual std::vector<std::pair<std::shared_ptr<Rendering::SamplerState>, uint32_t>> getSamplerStates() const {
			return { {sampBaseColor, (uint32_t)DefaultShaderSlot::BASE_COLOR} };
		}

		virtual std::shared_ptr<Rendering::InputLayout> getInputLayout() const {
			auto& renderer = Rendering::Renderer::getInstance();
			static std::shared_ptr<Rendering::InputLayout> sLayout(
				renderer.createInputLayout(DefaultVertexData::getDescription(), this->shader));
			return sLayout;
		}

	};
}