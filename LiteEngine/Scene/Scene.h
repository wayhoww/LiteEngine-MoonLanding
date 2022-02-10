#pragma once

#include "../Renderer/Resources.h"
#include "../Renderer/Renderer.h"

#include <string>
#include <any>
#include <type_traits>

namespace LiteEngine::SceneManagement {

	// SceneManagement ģ�鲻Ҫ�� Renderer ���ã��Ա�����һ���߳���ǰ׼������


	// Object: ���в�νṹ�����пռ�任 TRS
	// Component: ���������ɸ� property���������� key-value


	struct Object {
		std::vector<std::shared_ptr<Object>> children;
		std::weak_ptr<Object> parent;
		std::string name;

		DirectX::XMFLOAT3 transT{0, 0, 0};
		DirectX::XMVECTOR transR{0, 0, 0, 1};
		DirectX::XMFLOAT3 transS{1, 1, 1};

		DirectX::XMMATRIX getTransformMatrix() const {
			return DirectX::XMMatrixAffineTransformation(
				{ transS.x, transS.y, transS.z, 1 },
				{ 0, 0, 0, 1 },
				transR,
				{ transT.x, transT.y, transT.z, 1 }
			);
		}

		std::shared_ptr<Object> getChild(const std::string& name) const {
			for (auto sub : children) {
				if (sub->name == name) return sub;
			}
			return nullptr;
		}
		
		// û���κ� cache����ò�Ҫֱ�ӵ���~
		std::shared_ptr<Object> searchDescendant(const std::string& name) const {
			for (auto sub : children) {
				if (sub->name == name) return sub;
				if (auto out = sub->searchDescendant(name); out) return out;
			}
			return nullptr;
		}

		virtual ~Object() = default;

		Object& addScale(const DirectX::XMFLOAT3& deltaLocalScale) {
			transS.x += deltaLocalScale.x;
			transS.y += deltaLocalScale.y;
			transS.z += deltaLocalScale.z;
			return *this;
		}

		Object& multiplyScale(const DirectX::XMFLOAT3& deltaLocalScale) {
			transS.x *= deltaLocalScale.x;
			transS.y *= deltaLocalScale.y;
			transS.z *= deltaLocalScale.z;
			return *this;
		}

		Object& moveParentCoord(const DirectX::XMFLOAT3& deltaParentPos) {
			transT.x += deltaParentPos.x;
			transT.y += deltaParentPos.y;
			transT.z += deltaParentPos.z;
			return *this;
		}

		DirectX::XMFLOAT3 getDirectionFloat3InParent(const DirectX::XMFLOAT3& vec) {
			auto trans = this->getTransformMatrix();
			auto deltaParentPos4 = DirectX::XMVector4Transform({ vec.x, vec.y, vec.z, 0 }, trans);
			DirectX::XMFLOAT3 out;
			DirectX::XMStoreFloat3(&out, deltaParentPos4);
			return out;
		}

		Object& moveLocalCoord(const DirectX::XMFLOAT3& deltaLocalPos) {
			// transT ���ڸ���������ϵ�е��ƶ�
			// move ����Ҫ�ڱ�������ϵ���ƶ�
			
			DirectX::XMFLOAT3 deltaParentPos3 = getDirectionFloat3InParent(deltaLocalPos);
			transT.x += deltaParentPos3.x;
			transT.y += deltaParentPos3.y;
			transT.z += deltaParentPos3.z;
			return *this;
		}

		Object& rotateParentCoord(const DirectX::XMVECTOR& deltaRotate) {
			if(DirectX::XMQuaternionIsIdentity(deltaRotate)) return *this;
			this->transR = DirectX::XMQuaternionMultiply(this->transR, deltaRotate);
			return *this;
		}

		Object& rotateLocalCoord(const DirectX::XMVECTOR& deltaRotate) {
			if(DirectX::XMQuaternionIsIdentity(deltaRotate)) return *this;
			DirectX::XMVECTOR axisLocal;
			float angle;
			DirectX::XMQuaternionToAxisAngle(&axisLocal, &angle, deltaRotate);
			DirectX::XMFLOAT3 axisLocal3;
			DirectX::XMStoreFloat3(&axisLocal3, axisLocal);

			auto axisParent3 = getDirectionFloat3InParent(axisLocal3);
			auto axisParent = DirectX::XMLoadFloat3(&axisParent3);
			this->transR = DirectX::XMQuaternionMultiply(
				this->transR, 
				DirectX::XMQuaternionRotationAxis(axisParent, angle)
			);
			return *this;
		}
	};

	// TODO: vertex-shader-material ʵ����У��
	struct Material {
	protected:
		Material() = default;
	public:
		std::shared_ptr<Rendering::Shader> shader;
		std::shared_ptr<Rendering::ConstantBuffer> consantBuffers;
		virtual std::vector<std::pair<std::shared_ptr<Rendering::ShaderResourceView>, uint32_t>> getShaderResourceViews() const = 0;
		virtual std::vector<std::pair<std::shared_ptr<Rendering::SamplerState>, uint32_t>> getSamplerStates() const = 0;
		virtual std::shared_ptr<Rendering::InputLayout> getInputLayout() const = 0;
	};

	struct Mesh : public Object {
		// ��������һ�����ϡ���ͬ�� vbo �ṹ��һ����������Ĭ�ϲ���
		std::shared_ptr<Material> material;
		std::shared_ptr<Rendering::MeshObject> data;
	};

	struct Camera : public Object {
		Rendering::RenderingScene::CameraInfo data{};
	};

	struct Light : public Object {
		uint32_t type;						// all
		uint32_t shadow;					// all
		float innerConeAngle;			    // spot
		float outerConeAngle;				// spot
		float maximumDistance;				// spot & point
		DirectX::XMFLOAT3 direction_L;		// spot & directional
		DirectX::XMFLOAT3 intensity;		// all
	};

	struct Scene {
		std::shared_ptr<Object> rootObject;
		std::shared_ptr<Camera> activeCamera;

		void buildRenderingSceneRecursively(
			std::shared_ptr<Rendering::RenderingScene> dest,
			std::shared_ptr<Object> node,
			DirectX::XMMATRIX transform
		) {
			if (node == nullptr) return;

			// DirectX: ������
			auto newTransform = DirectX::XMMatrixMultiply(transform, node->getTransformMatrix());

			if (auto mesh = std::dynamic_pointer_cast<Mesh>(node); mesh) {
				auto& meshObj = mesh->data;
				std::shared_ptr<Rendering::Material> mat(new Rendering::Material());
				meshObj->material->constants = mesh->material->consantBuffers;
				meshObj->material->samplerStates = mesh->material->getSamplerStates();
				meshObj->material->shaderResourceViews = mesh->material->getShaderResourceViews();
				meshObj->material->shader = mesh->material->shader;
				meshObj->transform = newTransform;
				dest->meshObjects.push_back(meshObj);
			} else if (auto light = std::dynamic_pointer_cast<Light>(node); light) {
				Rendering::LightDesc lightDesc;
				
				lightDesc.type = light->type;
				lightDesc.innerConeAngle = light->innerConeAngle;
				lightDesc.outerConeAngle = light->outerConeAngle;
				lightDesc.intensity = light->intensity;
				lightDesc.shadow = light->shadow;
				lightDesc.maximumDistance = light->maximumDistance;

				// normal
				DirectX::XMVECTOR dir_L = DirectX::XMLoadFloat3(&light->direction_L);
				auto dir_W = DirectX::XMVector3TransformNormal(dir_L, newTransform);
				DirectX::XMStoreFloat3(&lightDesc.direction_W, dir_W);

				// position
				// ��Ϊԭ����ԭ�㣬���в����� scale �� rotate
				DirectX::XMFLOAT4 trans;
				DirectX::XMStoreFloat4(&trans, newTransform.r[3]);
				lightDesc.position_W = { trans.x, trans.y, trans.z };

				dest->lights.push_back(lightDesc);
			} else if (auto camera = std::dynamic_pointer_cast<Camera>(node); camera && camera->name == activeCamera->name) {
				dest->camera = camera->data;
				auto det = DirectX::XMMatrixDeterminant(newTransform);
				dest->camera.trans_W2V = DirectX::XMMatrixInverse(&det, newTransform);
			}

			for (auto child : node->children) {
				buildRenderingSceneRecursively(dest, child, newTransform);
			}
		}

		std::shared_ptr<Rendering::RenderingScene> getRenderingScene() {
			if (this->activeCamera == nullptr) {
				throw std::exception("no active camera");
			}
			std::shared_ptr<Rendering::RenderingScene> out(new Rendering::RenderingScene());
			buildRenderingSceneRecursively(out, rootObject, DirectX::XMMatrixIdentity());
			return out;
		}

		const std::shared_ptr<const Object> searchObject(const std::string& name) const {
			// �Ժ�������� cache����������Ԥ����
			if (rootObject->name == name) {
				return rootObject;
			}
			return rootObject->searchDescendant(name);
		}

		template<typename T, typename = decltype(dynamic_cast<T*>(new Object()))>
		std::shared_ptr<const T> search(const std::string& name) const {
			return std::dynamic_pointer_cast<const T>(this->searchObject(name));
		}

		std::shared_ptr<Object> searchObject(const std::string& name) {
			// �Ժ�������� cache����������Ԥ����
			if (rootObject->name == name) {
				return rootObject;
			}
			return rootObject->searchDescendant(name);
		}

		template<typename T, typename = decltype(dynamic_cast<T*>(new Object()))>
		std::shared_ptr<T> search(const std::string& name) {
			return std::dynamic_pointer_cast<T>(this->searchObject(name));
		}

	};

}