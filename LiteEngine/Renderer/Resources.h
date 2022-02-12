#pragma once

#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <d3d11.h>
#include <wrl.h>

#include <vector>
#include <atomic>
#include <memory>
#include <set>
#include <mutex>
#include <typeinfo>
#include <functional>

namespace LiteEngine::Rendering {

	// Ŀǰ buffer ȫ��ͬһ�ף�û������ VS PS
	namespace VSConstantBufferSlotID {
		// �Զ��� buffer
		constexpr uint32_t LONGTERM_CUSTOM = 0;		// ���ڣ�����һ֡��
		constexpr uint32_t PERFRAME_CUSTOM = 1;			// һ֡
		constexpr uint32_t MESH_OBJECT_CUSTOM = 2;			// ÿ������

		// �̶� buffer
		constexpr uint32_t LONGTERM_FIXED = 3;
		constexpr uint32_t PERFRAME_FIXED = 4;
		constexpr uint32_t MESH_OBJECT_FIXED = 5;
	};

	namespace PSConstantBufferSlotID {
		// �Զ��� buffer
		constexpr uint32_t LONGTERM_CUSTOM = 0;		// ���ڣ�����һ֡��
		constexpr uint32_t PERFRAME_CUSTOM = 1;			// һ֡
		constexpr uint32_t MESH_OBJECT_CUSTOM = 2;			// ÿ������

		// �̶� buffer
		constexpr uint32_t LONGTERM_FIXED = 3;
		constexpr uint32_t PERFRAME_FIXED = 4;
		constexpr uint32_t MESH_OBJECT_FIXED = 5;

		// ���� 
		constexpr uint32_t MATERIAL = 6;
	};

	// ����ṹ����ָ���ԭ��������С��������ġ���
	using InputElementDescriptions =
		std::vector<D3D11_INPUT_ELEMENT_DESC>;

	struct VertexBufferObject {
		Microsoft::WRL::ComPtr<ID3D11Buffer> vertices;
		std::shared_ptr<InputElementDescriptions> inputElementsDescriptions;
		uint32_t vertexStride;

		VertexBufferObject(ID3D11Buffer* vertices, std::shared_ptr<InputElementDescriptions> desc, uint32_t vertexStride) {
			this->vertices.Attach(vertices);
			this->inputElementsDescriptions = desc;
			this->vertexStride = vertexStride;
		}
	};

	using PtrIndexBufferObject = Microsoft::WRL::ComPtr<ID3D11Buffer>;

	struct Mesh {
		std::shared_ptr<VertexBufferObject> vbo;
		PtrIndexBufferObject indices;
		uint32_t indicesBegin;
		uint32_t indicesLength;

		Mesh(
			std::shared_ptr<VertexBufferObject> vbo, 
			PtrIndexBufferObject indices, 
			uint32_t indicesBegin, 
			uint32_t indicesLength
		) {
			this->vbo = vbo;
			this->indices = indices;
			this->indicesBegin = indicesBegin;
			this->indicesLength = indicesLength;
		}
	};

	struct Shader {

		std::vector<uint8_t> vertexShaderByteCode;

		Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader;
		Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader;

		Shader(
			ID3D11VertexShader* vertexShader,
			ID3D11PixelShader* pixelShader,
			std::vector<uint8_t> vertexShaderByteCode
		) : vertexShaderByteCode(vertexShaderByteCode) {

			this->vertexShader.Attach(vertexShader);
			this->pixelShader.Attach(pixelShader);

			this->setVertexShaderID();
		}

	protected:

		uint64_t vertexShaderID;

		void setVertexShaderID() {
			// �ȼ򵥴����ˡ���
			static std::atomic_uint64_t idCounter = 0;
			this->vertexShaderID = idCounter++;
		}

	};

	// Constant Buffer �ù̶��� slot��shader resource �� sampler �ò��̶��� slot
	class ConstantBuffer {
	protected:
		Microsoft::WRL::ComPtr<ID3D11Buffer> buffer;
		void* internalData;
		size_t dataSize;
		size_t typeHash;

		ConstantBuffer(const ConstantBuffer&) = default;

	public:
		std::shared_ptr<ConstantBuffer> getSharedInstance() {
			auto c = std::shared_ptr<ConstantBuffer>(new ConstantBuffer(*this));
			c->internalData = new uint8_t[dataSize];
			return c;
		}

		~ConstantBuffer() {
			delete[] internalData;
		}

		void operator=(const ConstantBuffer& other) = delete;

		template <typename DataType, 
			typename = std::enable_if_t<std::is_trivially_destructible_v<DataType>>>
		ConstantBuffer(const DataType& value, ID3D11Buffer* buffer) :
			dataSize(sizeof(DataType)),
			internalData(new DataType(value)),
			typeHash(typeid(DataType).hash_code())
		{
			this->buffer.Attach(buffer);
		}

		void updateBuffer(ID3D11DeviceContext* context) const {
			context->UpdateSubresource(this->buffer.Get(), 0, nullptr, this->internalData, 0, 0);
		}

		template <typename DataType>
		DataType& cpuData() {
			assert(typeHash == typeid(DataType).hash_code());
			return *reinterpret_cast<DataType*>(this->internalData);
		}

		ID3D11Buffer* const* getAddressOf() const {
			return buffer.GetAddressOf();
		}
	};


	using PtrShaderResourceView = Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>;
	using PtrRenderTargetView = Microsoft::WRL::ComPtr<ID3D11RenderTargetView>;

	using PtrSamplerState =
		Microsoft::WRL::ComPtr<ID3D11SamplerState>;

	class RenderableTexture {
	public:
		PtrShaderResourceView textureView;
		PtrRenderTargetView renderTargetView;
	};


	// ���Զ���̶���΢�е�͡���
	struct Material {
		std::shared_ptr<ConstantBuffer> constants;
		std::vector<std::pair<PtrShaderResourceView, uint32_t>> shaderResourceViews;
		std::vector<std::pair<PtrSamplerState, uint32_t>> samplerStates;
		std::shared_ptr<Shader> shader;

		virtual void updateAndBindResources(ID3D11DeviceContext* context) {
			if (this->constants) {
				this->constants->updateBuffer(context);
				context->PSSetConstantBuffers(PSConstantBufferSlotID::MATERIAL, 1, this->constants->getAddressOf());
			}

			for (auto& [view, slot] : this->shaderResourceViews) {
				if (view == nullptr) {
					ID3D11ShaderResourceView* view = nullptr;
					context->PSSetShaderResources(slot, 1, &view);
				} else {
					context->PSSetShaderResources(slot, 1, view.GetAddressOf());
				}
			}

			for (auto& [sampler, slot] : this->samplerStates) {
				if (sampler == nullptr) {
					ID3D11SamplerState* sampler = nullptr;
					context->PSSetSamplers(slot, 1, &sampler);
				} else {
					context->PSSetSamplers(slot, 1, sampler.GetAddressOf());
				}
			}
		}
	};

	using PtrInputLayout = Microsoft::WRL::ComPtr<ID3D11InputLayout>;

	// L: local
	// W: world
	// V: view
	// C: clip
	struct FixedPerobjectConstantData {
		// Note: �޸ĸýṹ���ʱ��ǵ�ͬʱ�޸� shader ����! (ObjectFixedConstants)

		// ����
		DirectX::XMMATRIX trans_L2W;

		// ����
		DirectX::XMMATRIX trans_W2L;

		// ���߽���
	};

	class MeshObject {
		std::shared_ptr<Mesh> mesh;
		// ��� Mesh �Ĳ��� �� Shader ��ƥ�䣬��ô���� InputLayout ��ʱ��Ӧ�þͻᱨ��ɣ�

		PtrInputLayout inputLayout;

		// fixedConstant: VS PS ����
		std::shared_ptr<ConstantBuffer> fixedConstantBuffer;

		// customConstant: ����
		std::shared_ptr<ConstantBuffer> customVSConstantBuffer;
		std::shared_ptr<ConstantBuffer> customPSConstantBuffer;

		void updateFixedConstantBuffer(ID3D11DeviceContext* context) const {
			auto& val = fixedConstantBuffer->cpuData<FixedPerobjectConstantData>();
			val.trans_L2W = this->transform;
			auto det = DirectX::XMMatrixDeterminant(this->transform);
			val.trans_W2L = DirectX::XMMatrixInverse(&det, this->transform);

			fixedConstantBuffer->updateBuffer(context);

			material->updateAndBindResources(context);
		}
	public:

		std::shared_ptr<Material> material;
		DirectX::XMMATRIX transform = DirectX::XMMatrixIdentity(); // 4 x 4 x 4 = 128 bytes, local_to_world

		MeshObject(
			std::shared_ptr<Mesh> mesh,
			std::shared_ptr<Material> material,
			PtrInputLayout inputLayout,
			std::shared_ptr<ConstantBuffer> fixedConstantBuffer,
			std::shared_ptr<ConstantBuffer> customVSConstantBuffer = nullptr,
			std::shared_ptr<ConstantBuffer> customPSConstantBuffer = nullptr
		) : mesh(mesh), material(material), inputLayout(inputLayout),
			fixedConstantBuffer(fixedConstantBuffer),
			customVSConstantBuffer(customVSConstantBuffer),
			customPSConstantBuffer(customPSConstantBuffer) {}

		void draw(ID3D11DeviceContext* context) const {
			this->updateFixedConstantBuffer(context);

			// IA input assembly
			UINT strideVertex = this->mesh->vbo->vertexStride, offsetVertex = 0;
			context->IASetVertexBuffers(0, 1, this->mesh->vbo->vertices.GetAddressOf(), &strideVertex, &offsetVertex);
			context->IASetIndexBuffer(this->mesh->indices.Get(), DXGI_FORMAT_R32_UINT, 0);
			context->IASetInputLayout(this->inputLayout.Get());
			context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			// VS vertex shader
			// Qs: ʲô�� class instance
			context->VSSetShader(this->material->shader->vertexShader.Get(), nullptr, 0);
			context->VSSetConstantBuffers(VSConstantBufferSlotID::MESH_OBJECT_FIXED, 1, this->fixedConstantBuffer->getAddressOf());
			if (this->customVSConstantBuffer)
				context->VSSetConstantBuffers(VSConstantBufferSlotID::MESH_OBJECT_CUSTOM, 1, this->customVSConstantBuffer->getAddressOf());

			// PS pixel shader
			context->PSSetShader(this->material->shader->pixelShader.Get(), nullptr, 0);
			context->VSSetConstantBuffers(PSConstantBufferSlotID::MESH_OBJECT_FIXED, 1, this->fixedConstantBuffer->getAddressOf());
			if (this->customPSConstantBuffer)
				context->VSSetConstantBuffers(VSConstantBufferSlotID::MESH_OBJECT_CUSTOM, 1, this->customPSConstantBuffer->getAddressOf());

			// draw
			context->DrawIndexed(this->mesh->indicesLength, this->mesh->indicesBegin, 0);
		}

	};

}