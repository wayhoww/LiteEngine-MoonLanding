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
		float occlusionStrength = 0;

		float normalMapScale = 1;
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
		std::shared_ptr<Rendering::SamplerState> sampEmissionColor;
		std::shared_ptr<Rendering::SamplerState> sampMetallic;
		std::shared_ptr<Rendering::SamplerState> sampRoughness;
		std::shared_ptr<Rendering::SamplerState> sampAO;
		std::shared_ptr<Rendering::SamplerState> sampNormal;

		std::shared_ptr<Rendering::ShaderResourceView> texBaseColor;
		std::shared_ptr<Rendering::ShaderResourceView> texEmissionColor;
		std::shared_ptr<Rendering::ShaderResourceView> texMetallic;
		std::shared_ptr<Rendering::ShaderResourceView> texRoughness;
		std::shared_ptr<Rendering::ShaderResourceView> texAO;
		std::shared_ptr<Rendering::ShaderResourceView> texNormal;

		virtual std::vector<std::pair<std::shared_ptr<Rendering::ShaderResourceView>, uint32_t>> getShaderResourceViews() const {
			std::vector<std::pair<std::shared_ptr<Rendering::ShaderResourceView>, uint32_t>> out;
			if (texBaseColor) out.push_back({ texBaseColor, (uint32_t)DefaultShaderSlot::BASE_COLOR });
			if (texEmissionColor) out.push_back({ texEmissionColor, (uint32_t)DefaultShaderSlot::EMISSION_COLOR });
			if (texMetallic) out.push_back({ texMetallic, (uint32_t)DefaultShaderSlot::METALLIC });
			if (texRoughness) out.push_back({ texRoughness, (uint32_t)DefaultShaderSlot::ROUGHNESS });
			if (texAO) out.push_back({ texAO, (uint32_t)DefaultShaderSlot::AMBIENT_OCCLUSION });
			if (texNormal) out.push_back({ texNormal, (uint32_t)DefaultShaderSlot::NORMAL });
			return out;
		}

		virtual std::vector<std::pair<std::shared_ptr<Rendering::SamplerState>, uint32_t>> getSamplerStates() const {
			std::vector<std::pair<std::shared_ptr<Rendering::SamplerState>, uint32_t>> out;
			if (sampBaseColor) out.push_back({ sampBaseColor, (uint32_t)DefaultShaderSlot::BASE_COLOR });
			if (sampEmissionColor) out.push_back({ sampEmissionColor, (uint32_t)DefaultShaderSlot::EMISSION_COLOR });
			if (sampMetallic) out.push_back({ sampMetallic, (uint32_t)DefaultShaderSlot::METALLIC });
			if (sampRoughness) out.push_back({ sampRoughness, (uint32_t)DefaultShaderSlot::ROUGHNESS });
			if (sampAO) out.push_back({ sampAO, (uint32_t)DefaultShaderSlot::AMBIENT_OCCLUSION });
			if (sampNormal) out.push_back({ sampNormal, (uint32_t)DefaultShaderSlot::NORMAL });
			return out;
		}

		virtual std::shared_ptr<Rendering::InputLayout> getInputLayout() const {
			auto& renderer = Rendering::Renderer::getInstance();
			static std::shared_ptr<Rendering::InputLayout> sLayout(
				renderer.createInputLayout(DefaultVertexData::getDescription(), this->shader));
			return sLayout;
		}

	};
}