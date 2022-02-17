#pragma once

#include "../Renderer/Resources.h"
#include "../Renderer/Renderer.h"
#include "../Utilities/Utilities.h"
#include "DefaultDS.h"

#include <string>
#include <any>
#include <type_traits>

namespace LiteEngine::SceneManagement {

	// SceneManagement 模块不要有 Renderer 调用，以便在另一个线程提前准备数据


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

		std::shared_ptr<Object> insertParent() {
			if (this->parent.expired()) {
				return nullptr;
			}

			std::shared_ptr<Object> newParent(new Object());
			static uint64_t parent_id = 0;
			newParent->name = this->name + "__#PARENT_" + std::to_string(parent_id++);
			newParent->parent = this->parent;

			auto oldParent = this->parent.lock();
			for (auto& child : oldParent->children) {
				if (&*child == this) {
					newParent->children.push_back(child);
					child = newParent;
					break;
				}
			}

			this->parent = newParent;

			newParent->transT = transT;
			newParent->transR = transR;
			newParent->transS = transS;

			this->transR = { 0, 0, 0, 1 };
			this->transT = { 0, 0, 0 };
			this->transS = { 1, 1, 1 };

			return newParent;
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
			// transT 是在父物体坐标系中的移动
			// move 函数要在本地坐标系做移动
			
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

		Object& rotateLocalCoord(const DirectX::XMFLOAT3& axis, float angle) {
			auto axisParent3 = getDirectionFloat3InParent(axis);
			auto axisParent = DirectX::XMLoadFloat3(&axisParent3);
			this->transR = DirectX::XMQuaternionMultiply(
				this->transR, 
				DirectX::XMQuaternionRotationAxis(axisParent, angle)
			);
			return *this;
		}

		DirectX::XMMATRIX getLocalToWorldMatrix() const {
			auto out = this->getTransformMatrix();
		
			if (auto ptr = this->parent.lock(); ptr) {
				auto prev = ptr->getLocalToWorldMatrix();
				auto rst = DirectX::XMMatrixMultiply(out, prev);
				return rst;
			} else {
				return out;
			}
			
		}

		DirectX::XMFLOAT3 getWorldPosition() const {
			auto mat = this->getLocalToWorldMatrix();
			DirectX::XMFLOAT3 pos;
			DirectX::XMStoreFloat3(&pos, mat.r[3]); // TRS 矩阵 M33 == 1
			return pos;
		}

		void setLocalPos(const DirectX::XMFLOAT3& val) {
			this->transT = val;
		}

		void dump(int level = 0, Object* parent = nullptr) {
			bool parentValid = &*this->parent.lock() == parent;
			DirectX::XMVECTOR axis;
			float angle;
			DirectX::XMFLOAT3 axis3;
			DirectX::XMQuaternionToAxisAngle(&axis, &angle, this->transR);
			DirectX::XMStoreFloat3(&axis3, axis);


			OutputDebugStringA(
				(std::string(level * 4 + 2, ' ') + " > " +
					(parentValid ? "" : "[Invalid Parent] ") + this->name).c_str());
			
			auto pos = this->getWorldPosition();

			static char buffer[1000];
			sprintf_s(buffer, " R: [%.2f %.2f %.2f] %.2f deg; T: [%.2f %.2f %.2f]([%.2f %.2f %.2f])\n",
				axis3.x, axis3.y, axis3.z, angle / PI * 180,
				transT.x, transT.y, transT.z,
				pos.x, pos.y, pos.z
			);

			OutputDebugStringA(buffer);

			for (auto child : this->children) {
				child->dump(level + 1, this);
			}
		}
	};

	struct Material: public Rendering::Material {
	protected:
		Material() = default;
	};

	struct Mesh : public Object {
		// 必须设置一个材料。不同的 vbo 结构不一样，做不了默认材料
		std::shared_ptr<Material> material;
		std::shared_ptr<Rendering::MeshObject> data;
	};

	struct Camera : public Object {
	protected:

		static float signof(float f) {
			if (f >= 0) return 1;
			else return -1;
		}

		static DirectX::XMMATRIX setAbsScaleComponentToOne(const DirectX::XMMATRIX& mat) {
			DirectX::XMVECTOR scale, rotate, translate;
			DirectX::XMMatrixDecompose(&scale, &rotate, &translate, mat);
			DirectX::XMFLOAT3 scale3;
			DirectX::XMStoreFloat3(&scale3, scale);
			scale3.x = signof(scale3.x);
			scale3.y = signof(scale3.y);
			scale3.z = signof(scale3.z);
			scale = DirectX::XMLoadFloat3(&scale3);
			return DirectX::XMMatrixAffineTransformation(
				scale,
				{ 0, 0, 0, 1 },
				rotate,
				translate
			);
		}
	public:
		Rendering::RenderingScene::CameraInfo data{};

		bool fixedWorldUp = false;

		enum class FixedWorldDirection {
			None, LookAtCoord, LookAtObject, LookTo
		};
		FixedWorldDirection fixedWorldLookDirection = FixedWorldDirection::None;

		DirectX::XMFLOAT3 worldUp;
		DirectX::XMFLOAT3 worldLookAtCoord;
		std::weak_ptr<Object> worldLookAtObject;
		DirectX::XMFLOAT3 worldLookTo;

		DirectX::XMMATRIX getW2VMatrix(DirectX::XMMATRIX transformMatrix) {
			auto cameraTrans = setAbsScaleComponentToOne(transformMatrix);
			auto det = DirectX::XMMatrixDeterminant(cameraTrans);
			auto trans = DirectX::XMMatrixInverse(&det, cameraTrans);
			return trans;

			auto lookEye = DirectX::XMVector3TransformCoord({ 0, 0, 0 }, transformMatrix);
			auto lookTo = DirectX::XMVector3TransformNormal({0, 0, 1}, transformMatrix);
			auto lookUp = DirectX::XMVector3TransformNormal({ 0, 1, 0 }, transformMatrix);
			DirectX::XMVECTOR lookAt{0, 0, 0, 0};
			bool lookAtMode = false;

			if (fixedWorldUp) {
				lookUp = DirectX::XMLoadFloat3(&worldUp);
			}

			switch (fixedWorldLookDirection) {
			case FixedWorldDirection::LookTo:
				lookTo = DirectX::XMLoadFloat3(&worldLookTo);
				break;
			case FixedWorldDirection::LookAtObject:
				lookAtMode = true;
				auto atObjCoord = this->worldLookAtObject.lock()->getWorldPosition();
				lookAt = DirectX::XMLoadFloat3(&atObjCoord);
				break;
			case FixedWorldDirection::LookAtCoord:
				lookAtMode = true;
				lookAt = DirectX::XMLoadFloat3(&worldLookAtCoord);
				break;
			case FixedWorldDirection::None:
				break;
			}
			
			if (lookAtMode) {
				return DirectX::XMMatrixLookAtLH(lookEye, lookAt, lookUp);
			} else {
				return DirectX::XMMatrixLookToLH(lookEye, lookTo, lookUp);
			}

			assert(false);
		}
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

	public:
		std::shared_ptr<Object> rootObject;
		std::shared_ptr<Camera> activeCamera;


		void buildRenderingSceneRecursively(
			std::shared_ptr<Rendering::RenderingScene> dest,
			std::shared_ptr<Object> node,
			DirectX::XMMATRIX transform
		) {
			if (node == nullptr) return;

			// DirectX: 行主序
			auto newTransform = DirectX::XMMatrixMultiply(node->getTransformMatrix(), transform);

			if (auto mesh = std::dynamic_pointer_cast<Mesh>(node); mesh) {
				auto& meshObj = mesh->data;
				meshObj->material = mesh->material;
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
				auto dir_W = DirectX::XMVector3Normalize(DirectX::XMVector3TransformNormal(dir_L, newTransform));
				DirectX::XMStoreFloat3(&lightDesc.direction_W, dir_W);

				// position
				// 因为原来在原点，所有不在意 scale 和 rotate
				DirectX::XMFLOAT4 trans;
				DirectX::XMStoreFloat4(&trans, newTransform.r[3]);
				lightDesc.position_W = { trans.x, trans.y, trans.z };

				dest->lights.push_back(lightDesc);
			} else if (auto camera = std::dynamic_pointer_cast<Camera>(node); camera && camera->name == activeCamera->name) {
				dest->camera = camera->data;
				dest->camera.trans_W2V = camera->getW2VMatrix(newTransform);
			}

			for (auto child : node->children) {
				buildRenderingSceneRecursively(dest, child, newTransform);
			}
		}

		Rendering::RenderingScene::CameraInfo getCameraInfo(std::shared_ptr<Camera> camera) {
			auto out = camera->data;
			out.trans_W2V = camera->getW2VMatrix(camera->getLocalToWorldMatrix());
			return out;
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