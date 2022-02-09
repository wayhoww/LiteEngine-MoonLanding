#pragma once

#include "../Renderer/Resources.h"
#include "../Renderer/Renderer.h"

#include <string>
#include <any>
#include <type_traits>

namespace LiteEngine::SceneManagement {
	// Object: 具有层次结构；具有空间变换 TRS
	// Component: 定义了若干个 property，可以设置 key-value


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
		
		// 没有任何 cache，最好不要直接调用~
		std::shared_ptr<Object> searchDescendant(const std::string& name) const {
			for (auto sub : children) {
				if (sub->name == name) return sub;
				if (auto out = sub->searchDescendant(name); out) return out;
			}
			return nullptr;
		}

		virtual ~Object() = default;
	};

	// TODO: vertex-shader-material 实用性校验
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
		// 必须设置一个材料。不同的 vbo 结构不一样，做不了默认材料
		std::shared_ptr<Material> material;
		std::shared_ptr<Rendering::Mesh> data;
	};

	struct Camera : public Object {
		Rendering::RenderingScene::CameraInfo data{};
	};

	struct Light : public Object {
		Rendering::LightDesc data{};
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

			auto& renderer = Rendering::Renderer::getInstance();

			// DirectX: 行主序
			auto newTransform = DirectX::XMMatrixMultiply(transform, node->getTransformMatrix());

			if (auto mesh = std::dynamic_pointer_cast<Mesh>(node); mesh) {
				std::shared_ptr<Rendering::Material> mat(new Rendering::Material());
				mat->constants = mesh->material->consantBuffers;
				mat->samplerStates = mesh->material->getSamplerStates();
				mat->shaderResourceViews = mesh->material->getShaderResourceViews();
				mat->shader = mesh->material->shader;

				auto meshObj = renderer.createMeshObject(mesh->data,
					mat,
					mesh->material->getInputLayout(),
					nullptr
				);
				meshObj->transform = newTransform;
				dest->meshObjects.push_back(meshObj);
			} else if (auto light = std::dynamic_pointer_cast<Light>(node); light) {
				dest->lights.push_back(light->data);
				DirectX::XMFLOAT4 trans;
				DirectX::XMStoreFloat4(&trans, newTransform.r[3]);
				dest->lights.rbegin()->position_W = { trans.x, trans.y, trans.z };
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
			// 以后可以做点 cache，或者做点预处理
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
			// 以后可以做点 cache，或者做点预处理
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