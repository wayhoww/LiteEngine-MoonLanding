#pragma once
#include <d3d11.h>
#include <DirectXMath.h>

#include <memory>

#include "Resources.h"
#include "Renderer.h"
#include "../Utilities/Utilities.h"

// 基本上是 Blinn-Phong 材质的物体，包括
// 1. 顶点
// 2. 材质
//   2.1 ConstantBuffer  (如颜色等)
//   2.2 Texture         (若干贴图)
//   2.3 SamplerState
//	 2.4 Shader （在 Shader 文件夹中）
// 完全没有 alpha channel

// 根据是否有 normal map，用两套不同的 shader

namespace LiteEngine::Rendering {

	// 顶点
#pragma pack(push, 1)
	struct BasicVertex {
		// NOTE：修改这个结构体的时候记得同时修改 DESCRIPTION！

		DirectX::XMFLOAT3 position;	// 不设置默认值。。这个是必须的。
		DirectX::XMFLOAT3 normal; // 不设置默认值。。这个是必须的。

		DirectX::XMFLOAT3 tangent{ 1, 0, 0 }; // 不涉及 normal map 的话，其实不需要
		DirectX::XMFLOAT2 texCoord{ 0, 0 }; // 没有贴图的话就不需要
		DirectX::XMFLOAT3 color{ 1, 1, 1 };    // RGBA. //TODO: 加载时是否要 x^(2.2)


		static std::shared_ptr<InputElementDescriptions> getDescription() {
			static std::shared_ptr<InputElementDescriptions> desc(new InputElementDescriptions{
				std::vector<D3D11_INPUT_ELEMENT_DESC>{
					{ "POSITION",   0, DXGI_FORMAT_R32G32B32_FLOAT,     0, 0,                            D3D11_INPUT_PER_VERTEX_DATA, 0 },
					{ "NORMAL",     0, DXGI_FORMAT_R32G32B32_FLOAT,     0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
					{ "TANGENT",    0, DXGI_FORMAT_R32G32B32_FLOAT,     0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
					{ "TEXCOORD",   0, DXGI_FORMAT_R32G32_FLOAT,        0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
					{ "COLOR",      0, DXGI_FORMAT_R32G32B32_FLOAT,  0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
				}
				});
			return desc;
		}
	};
#pragma pack(pop)

	static_assert(sizeof(BasicVertex) == 4 * (3 + 3 + 3 + 2 + 3), "struct `BasicVertex` is not tightly packed");


	// 材质
	struct alignas(16) BasicMaterialConstantData {
		// Note: 记得修改 hlsl ！！
		DirectX::XMFLOAT3 colorAmbient{ 0.1f, 0.1f, 0.1f };		// Ka
		float specularComponent{ 0 };							// Ns
		DirectX::XMFLOAT3 colorDiffuse{ 1, 1, 1 };			// Kd
	private:
		float _space;
	public:
		DirectX::XMFLOAT3 colorSpecular{ 0, 0, 0 };			// Ks
	};

	static_assert(sizeof(BasicMaterialConstantData) == 4 * (4 + 4 + 4), "struct `BasicMaterialConstantData` is not aligned correctly");

	// 会重复创建！调用者需要自己做好管理
	std::shared_ptr<Shader> getBasicMaterialShader(Renderer& renderer, bool withNormalMap);

	std::shared_ptr<MeshObject> createBasicMeshObject(
		Renderer& renderer,
		std::shared_ptr<VertexBufferObject> vbo,
		const uint32_t* indices,
		uint32_t lengthOfIndices,
		std::shared_ptr<Shader> basicShader,
		const BasicMaterialConstantData& constantData,
		const std::wstring& colorMapPath = L"",
		const std::wstring& normalMapPath = L""
	);

	std::shared_ptr<MeshObject> createBasicMeshObject(
		Renderer& renderer,
		std::shared_ptr<VertexBufferObject> vbo,
		const std::vector<uint32_t>& indices,
		std::shared_ptr<Shader> basicShader,
		const BasicMaterialConstantData& constantData,
		const std::wstring& colorMapPath = L"",
		const std::wstring& normalMapPath = L""
	);


	std::shared_ptr<Shader> createBasicMaterialShader(Renderer& renderer, bool withNormalMap);

}