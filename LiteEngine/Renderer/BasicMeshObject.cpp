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

	// ���ظ���������������Ҫ�Լ����ù���
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
	  �����뷨��
	  1. PS �� VS Ҫ���롣��Ȼ������ʵ���Ƿ���ġ���ֻҪ�Լ���װ�����������

	  2. �ܶ���Դ�����ظ������ˡ���� Renderer �Ǹ������Ļ�����ôֻҪ����һ�ξ͹��ˡ���
		 Ҫô�Ͱ� Renderer ���ɵ�����Ҫô�� Renderer ����Ūһ�� cache ���ܣ����Դ洢����

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
		// TODO: ����һ�����干��һ�� constant buffer��
		// ����������һ��� material ����һ�� constant buffer��ֻ�������ϲ�ͬ������ ?
		// �����������׵ġ��� createConstantBuffer ��һ�������Ϳ�����
		// һ������ô�����أ�
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
