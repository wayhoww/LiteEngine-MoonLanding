#pragma once
#include <d3d11.h>
#include <DirectXMath.h>

#include <memory>

#include "Resources.h"
#include "Renderer.h"
#include "../Utilities/Utilities.h"

// �������� Blinn-Phong ���ʵ����壬����
// 1. ����
// 2. ����
//   2.1 ConstantBuffer  (����ɫ��)
//   2.2 Texture         (������ͼ)
//   2.3 SamplerState
//	 2.4 Shader ���� Shader �ļ����У�
// ��ȫû�� alpha channel

// �����Ƿ��� normal map�������ײ�ͬ�� shader

namespace LiteEngine::Rendering {

	// ����
#pragma pack(push, 1)
	struct BasicVertex {
		// NOTE���޸�����ṹ���ʱ��ǵ�ͬʱ�޸� DESCRIPTION��

		DirectX::XMFLOAT3 position;	// ������Ĭ��ֵ��������Ǳ���ġ�
		DirectX::XMFLOAT3 normal; // ������Ĭ��ֵ��������Ǳ���ġ�

		DirectX::XMFLOAT3 tangent{ 1, 0, 0 }; // ���漰 normal map �Ļ�����ʵ����Ҫ
		DirectX::XMFLOAT2 texCoord{ 0, 0 }; // û����ͼ�Ļ��Ͳ���Ҫ
		DirectX::XMFLOAT3 color{ 1, 1, 1 };    // RGBA. //TODO: ����ʱ�Ƿ�Ҫ x^(2.2)


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


	// ����
	struct alignas(16) BasicMaterialConstantData {
		// Note: �ǵ��޸� hlsl ����
		DirectX::XMFLOAT3 colorAmbient{ 0.1f, 0.1f, 0.1f };		// Ka
		float specularComponent{ 0 };							// Ns
		DirectX::XMFLOAT3 colorDiffuse{ 1, 1, 1 };			// Kd
	private:
		float _space;
	public:
		DirectX::XMFLOAT3 colorSpecular{ 0, 0, 0 };			// Ks
	};

	static_assert(sizeof(BasicMaterialConstantData) == 4 * (4 + 4 + 4), "struct `BasicMaterialConstantData` is not aligned correctly");

	// ���ظ���������������Ҫ�Լ����ù���
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