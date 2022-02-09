#include "BasicMeshObject.h"

#include <d3d11.h>
#include <DirectXMath.h>

#include <memory>

#include "Resources.h"
#include "Renderer.h"
#include "../Utilities/Utilities.h"


namespace LiteEngine::Rendering {

	namespace BasicMaterialTextureSlotID {
		constexpr uint32_t COLOR = 0;
		constexpr uint32_t NORMAL = 1;
	}

	// 会重复创建！调用者需要自己做好管理
	std::shared_ptr<Shader> createBasicMaterialShader(Renderer& renderer, bool withNormalMap) {
		// configurable directory ? 
		if (withNormalMap) {
			return renderer.createShader(
				loadBinaryFromFile(L"BasicMaterialVS.cso"),
				loadBinaryFromFile(L"BasicMaterialPSWithNormalMap.cso")
			);
		}
		else {
			return renderer.createShader(
				loadBinaryFromFile(L"BasicMaterialVS.cso"),
				loadBinaryFromFile(L"BasicMaterialPSWithoutNormalMap.cso")
			);
		}
	}


	/*
	* TODO
	  几个想法：
	  1. PS 和 VS 要分离。当然现在其实就是分离的。。只要自己组装下面这个就行

	  2. 很多资源都被重复创建了。如果 Renderer 是个单例的话，那么只要创建一次就够了。。
		 要么就把 Renderer 做成单例，要么在 Renderer 里面弄一个 cache 功能，可以存储数据

	*/
	std::shared_ptr<MeshObject> createBasicMeshObject(
		Renderer& renderer,
		std::shared_ptr<VertexBufferObject> vbo,
		const uint32_t* indices,
		uint32_t lengthOfIndices,
		std::shared_ptr<Shader> basicShader,
		const BasicMaterialConstantData& constantData,
		const std::wstring& colorMapPath,
		const std::wstring& normalMapPath
	) {

		auto mesh = renderer.createMesh(vbo, indices, lengthOfIndices);
		// TODO: 可以一类物体共享一个 constant buffer，
		// 比如所有这一类的 material 共享一个 constant buffer，只是配置上不同的数据 ?
		// 基本上是容易的。。 createConstantBuffer 加一个参数就可以了
		// 一般是怎么做的呢？
		auto materialConstant = renderer.createConstantBuffer(constantData);
		//renderer.createShaderResourceView(resource, desc);

		auto material = std::shared_ptr<Material>(new Material());

		material->constants = materialConstant;

		if (colorMapPath.size() > 0) {
			material->shaderResourceViews.push_back({
				renderer.createSimpleTexture2DFromWIC(colorMapPath),
				BasicMaterialTextureSlotID::COLOR });
		} else {
			material->shaderResourceViews.push_back({
				renderer.createDefaultTexture({ 1, 1, 1, 1 }), 
				BasicMaterialTextureSlotID::COLOR });
		}

		if (normalMapPath.size() > 0) {
			material->shaderResourceViews.push_back({
				renderer.createSimpleTexture2DFromWIC(
					normalMapPath), BasicMaterialTextureSlotID::NORMAL});
		}

		material->shader = basicShader;

		CD3D11_SAMPLER_DESC samplerDesc{};
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		auto sampler = renderer.createSamplerState(samplerDesc);
		material->samplerStates.push_back({ sampler, 0 });

		auto inputLayout = renderer.createInputLayout(BasicVertex::getDescription(), basicShader);

		return renderer.createMeshObject(mesh, material, inputLayout, nullptr);
	}
	std::shared_ptr<MeshObject> createBasicMeshObject(
		Renderer& renderer,
		std::shared_ptr<VertexBufferObject> vbo,
		const std::vector<uint32_t>& indices,
		std::shared_ptr<Shader> basicShader,
		const BasicMaterialConstantData& constantData,
		const std::wstring& colorMapPath,
		const std::wstring& normalMapPath
	) {
		return createBasicMeshObject(renderer, vbo, indices.data(),
			(uint32_t)indices.size(), basicShader, constantData, colorMapPath, normalMapPath);

	}

}
