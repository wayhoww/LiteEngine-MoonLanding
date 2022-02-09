#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_MSC_SECURE_CRT

#include "../ThirdParty/tiny_gltf/tiny_gltf.h"

#include "../Utilities/Utilities.h"
#include "../Scene/DefaultDS.h"
#include "../Renderer/Renderer.h"
#include "../Renderer/Resources.h"
#include "../Scene/Scene.h"
#include "DefaultLoader.h"

#include <cassert>
#include <map>
#include <filesystem>


using namespace tinygltf;


namespace LiteEngine::IO {

#pragma pack(push, 1)
    template<size_t M, size_t N, typename T>
    struct SmallMatrix {
        static constexpr size_t ShapeM = M;
        static constexpr size_t ShapeN = N;
        using InnerDataType = T;

        T data[M][N];
    };
#pragma pack(pop)

    using Vec2f = SmallMatrix<2, 1, float>;
    using Vec3f = SmallMatrix<3, 1, float>;
    using Vec4f = SmallMatrix<4, 1, float>;

    template <typename TARGET, typename = std::enable_if_t<std::is_trivial_v<TARGET>&& std::is_standard_layout_v<TARGET>>>
    TARGET load_little_endian(const void* raw_data) {
        // todo: test it on a big-endian machine..

        union {
            uint32_t u32;
            uint8_t u4[4];
        } endian_tester;
        endian_tester.u32 = 0x01000000;
        bool is_big_endian = endian_tester.u4[0] == 1;

        if (is_big_endian && sizeof(TARGET) > 1) {
            // std::byteswap is not currently supported by gcc...
            if constexpr (sizeof(TARGET) == 4) {
                uint32_t data = *reinterpret_cast<const uint32_t*>(raw_data);
                uint32_t new_data =
                    ((data & 0x000000FF) << 24) | ((data & 0x0000FF00) << 8) |
                    ((data & 0x00FF0000) >> 8) | ((data & 0xFF000000) >> 24);
                return *reinterpret_cast<TARGET*>(&new_data);
            }
            else if constexpr (sizeof(TARGET) == 2) {
                uint16_t data = *reinterpret_cast<const uint16_t*>(raw_data);
                uint16_t new_data = ((data & 0x00FF) << 8) | ((data & 0xFF00) >> 8);
                return *reinterpret_cast<TARGET*>(&new_data);
            }
            else {
                auto data = *reinterpret_cast<const TARGET*>(raw_data);
                auto* p_data = reinterpret_cast<uint8_t*>(&data);
                size_t size = sizeof(TARGET);
                for (int i = 0; i < size / 2; i++) {
                    std::swap(p_data[i], p_data[size - 1 - i]);
                }
                return data;
            }
        }
        else {
            // x86
            return *reinterpret_cast<const TARGET*>(raw_data);
        }
    }

    template <typename TargetMatrix>
    std::vector<TargetMatrix> load_data(tinygltf::Model model, int accessor_index) {
        constexpr int M = TargetMatrix::ShapeM;
        constexpr int N = TargetMatrix::ShapeN;

        auto accessor = model.accessors[accessor_index];
        if (accessor.sparse.isSparse) {
            throw std::exception("mesh uses sparse accessor, which involves an "
                "unimplemented feature");
        }

        auto view = model.bufferViews[accessor.bufferView];
        auto buffer = model.buffers[view.buffer];
        auto stride = accessor.ByteStride(view);
        auto count = view.byteLength / stride;

        auto component_length = 0;

        switch (accessor.componentType) {
        case TINYGLTF_COMPONENT_TYPE_BYTE:
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
            component_length = 1;
            break;
        case TINYGLTF_COMPONENT_TYPE_SHORT:
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
            component_length = 2;
            break;
        case TINYGLTF_COMPONENT_TYPE_FLOAT:
        case TINYGLTF_COMPONENT_TYPE_INT:
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
            component_length = 4;
            break;
        case TINYGLTF_COMPONENT_TYPE_DOUBLE:
            component_length = 8;
            break;
        default:
            throw std::exception("undefined component type. corrupted glTF file.");
        }

        using TargetDataType = typename TargetMatrix::InnerDataType;

        auto loader_wrapper = [&accessor](TargetDataType* dest,
            const void* raw_data) {
            switch (accessor.componentType) {
            case TINYGLTF_COMPONENT_TYPE_BYTE:
                *dest = TargetDataType(load_little_endian<int8_t>(raw_data));
                break;

            case TINYGLTF_COMPONENT_TYPE_DOUBLE:
                *dest = TargetDataType(load_little_endian<double>(raw_data));
                break;

            case TINYGLTF_COMPONENT_TYPE_FLOAT:
                *dest = TargetDataType(load_little_endian<float>(raw_data));
                break;

            case TINYGLTF_COMPONENT_TYPE_INT:
                *dest = TargetDataType(load_little_endian<int32_t>(raw_data));
                break;

            case TINYGLTF_COMPONENT_TYPE_SHORT:
                *dest = TargetDataType(load_little_endian<int16_t>(raw_data));
                break;

            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
                *dest = TargetDataType(load_little_endian<uint8_t>(raw_data));
                break;

            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
                *dest = TargetDataType(load_little_endian<uint32_t>(raw_data));
                break;

            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                *dest = TargetDataType(load_little_endian<uint16_t>(raw_data));
                break;

            default:
                throw std::exception(
                    "undefined component type. corrupted glTF file.");
            }
        };

        std::vector<TargetMatrix> out;

        for (int index = 0; index < count; index++) {
            size_t offset = view.byteOffset + accessor.byteOffset + stride * index;
            TargetMatrix mat;
            for (int i = 0; i < M; i++) {
                for (int j = 0; j < N; j++) {
                    loader_wrapper(&mat.data[i][j], &buffer.data[offset]);
                    offset += component_length;
                }
            }
            out.push_back(mat);
        }

        return out;
    }

    std::vector<Vec3f> load_mesh_position(const tinygltf::Model& model,
        const tinygltf::Primitive& primitive) {

        if (!primitive.attributes.count("POSITION")) {
            throw std::exception("mesh does not have attribute POSITION");
        }

        auto attr = primitive.attributes.at("POSITION");
        auto accessor = model.accessors[attr];

        if (accessor.type != TINYGLTF_TYPE_VEC3 ||
            accessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT) {
            throw std::exception(
                "POSITION accessor's type or component type is invalid."
                "the gltf file is corrupted!");
        }

        return load_data<Vec3f>(model, attr);
    }

    std::vector<Vec3f> load_mesh_normal(const tinygltf::Model& model,
        const tinygltf::Primitive& primitive) {
        auto attr = primitive.attributes.at("NORMAL");
        auto accessor = model.accessors[attr];

        if (accessor.type != TINYGLTF_TYPE_VEC3 ||
            accessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT) {

            throw std::exception(
                "NORMAL accessor's is invalid. the gltf file is corrupted!");
        }

        auto out = load_data<Vec3f>(model, attr);

        return out;
    }

    std::vector<Vec2f> load_mesh_uvcoord(const tinygltf::Model& model,
        const tinygltf::Primitive& primitive, uint32_t uvID) {
        auto attr = primitive.attributes.at("TEXCOORD_" + std::to_string(uvID));
        auto accessor = model.accessors[attr];

        if (accessor.type != TINYGLTF_TYPE_VEC2 ||
            accessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT) {

            throw std::exception(("TEXCOORD_" + std::to_string(uvID) + " accessor's is invalid. the gltf file is corrupted!").c_str());
        }

        auto out = load_data<Vec2f>(model, attr);

        return out;
    }

    std::vector<Vec4f> load_mesh_tangent(const tinygltf::Model& model,
        const tinygltf::Primitive& primitive) {
        auto attr = primitive.attributes.at("TANGENT");
        auto accessor = model.accessors[attr];

        if (accessor.type != TINYGLTF_TYPE_VEC4 ||
            accessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT) {

            throw std::exception(
                "TANGENT accessor's is invalid. the gltf file is corrupted!");
        }

        auto out = load_data<Vec4f>(model, attr);

        return out;
    }

    std::vector<Vec4f> load_mesh_color(const tinygltf::Model& model,
        const tinygltf::Primitive& primitive, uint32_t color_id) {
        auto attr = primitive.attributes.at("COLOR_" + std::to_string(color_id));
        auto accessor = model.accessors[attr];

        if (accessor.type == TINYGLTF_TYPE_VEC4 &&
            accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) {
            return load_data<Vec4f>(model, attr);
        } else if (accessor.type == TINYGLTF_TYPE_VEC3 &&
            accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) {
            auto out3 = load_data<Vec3f>(model, attr);
            std::vector<Vec4f> out(out3.size());
            for (auto i = 0; i < out.size(); i++) {
                for (int j = 0; j < 3; j++) {
                    out[i].data[j][0] = out3[i].data[j][0];
                }
                out[i].data[3][0] = 1;
            }
            return out;
        } else {
            throw std::exception(
                "COLOR_n accessor's is invalid. the gltf file is corrupted!");
        }
    }



    // NOTE: 
    std::vector<DefaultMeshGLTF> loadDefaultMesh(
        const tinygltf::Mesh& mesh,
        const Model& model,
        std::vector<SceneManagement::DefaultVertexData>& vb,
        std::vector<uint32_t>& indices
    ) {
        // load indices
        std::vector<DefaultMeshGLTF> meshOut;

        auto meshOutOffset = meshOut.size();
        meshOut.resize(meshOut.size() + mesh.primitives.size());

        for (auto pID = 0; pID < mesh.primitives.size(); pID++) {
            auto& primitive = mesh.primitives[pID];

            if (primitive.mode != TINYGLTF_MODE_TRIANGLES) {
                throw std::exception(
                    "mode of primitive other than TRIANGLES has not been supported yet."
                    "please triangulate the model using another tool like Blender.");
            }

            auto& out = meshOut[meshOutOffset + pID];
            // name
            static uint64_t meshCounter = 0;
            out.name = mesh.name + "__#MESH_" + std::to_string(pID);

            // materialID
            out.materialID = primitive.material;

            uint32_t offset = (uint32_t)vb.size();

            // indices
            auto newIndices = load_data<SmallMatrix<1, 1, uint32_t>>(model, primitive.indices);
            out.indexBegin = (uint32_t)indices.size();
            out.indexLength = (uint32_t)newIndices.size();
            std::transform(newIndices.begin(), newIndices.end(),
                std::back_inserter(indices), [offset](auto x) { return x.data[0][0] + offset; });
            

            // vbo
            std::vector<Vec3f> positions, normals, tangents;
            std::vector<Vec2f> uv0, uv1;
            std::vector<Vec4f> color0;

            if (primitive.attributes.count("POSITION")) {
                positions = load_mesh_position(model, primitive);
            }
            else {
                throw std::exception("POSITION is not set for a mesh");
            }

            if (primitive.attributes.count("NORMAL")) {
                // has been normalized..
                normals = load_mesh_normal(model, primitive);
            } else {
                throw std::exception("missing normal");
            }


            if (primitive.attributes.count("TANGENT")) {
                auto tangents4d = load_mesh_tangent(model, primitive);
                for (auto val : tangents4d) {
                    // should be normalized...
                    Vec3f out;
                    double squaredSum = 0;
                    for (int i = 0; i < 3; i++) {
                        out.data[i][0] = val.data[i][0];
                        squaredSum += (double)val.data[i][0] * val.data[i][0];
                    }
                    float distance = (float)sqrt(squaredSum);
                    for (int i = 0; i < 3; i++) {
                        out.data[i][0] /= distance;
                    }

                    tangents.emplace_back(out);

                    // TODO: handedness
                    // out.handedness.emplace_back(val[3] < 0 ? -1 : 1);
                }
            } else {
                throw std::exception("missing tangent");
            }

            if (primitive.attributes.count("TEXCOORD_0")) {
                // has been normalized..
                uv0 = load_mesh_uvcoord(model, primitive, 0);
            } else {
                for (int i = 0; i < positions.size(); i++) {
                    uv0.emplace_back(Vec2f{ 0, 0 });
                }
            }

            if (primitive.attributes.count("TEXCOORD_1")) {
                // has been normalized..
                uv1 = load_mesh_uvcoord(model, primitive, 1);
            } else {
                for (int i = 0; i < positions.size(); i++) {
                    uv1.emplace_back(Vec2f{ 0, 0 });
                }
            }

            if (primitive.attributes.count("COLOR_0")) {
                // has been normalized..
                color0 = load_mesh_color(model, primitive, 0);
            } else {
                for (int i = 0; i < positions.size(); i++) {
                    color0.emplace_back(Vec4f{ 1, 1, 1, 1 });
                }
            }

            vb.resize(vb.size() + positions.size());

            for (int i = 0; i < positions.size(); i++) {
                auto& vert = vb[offset + i];
#define COPY_MEMORY(dest, src) memcpy(&dest, &src, sizeof(dest))
                COPY_MEMORY(vert.color, color0[i]);
                COPY_MEMORY(vert.normal, normals[i]);
                COPY_MEMORY(vert.position, positions[i]);
                COPY_MEMORY(vert.tangent, tangents[i]);
                COPY_MEMORY(vert.texCoord0, uv0[i]);
                COPY_MEMORY(vert.texCoord1, uv1[i]);
#undef COPY_MEMORY
            }
        }

        return meshOut;
    }

    //static uint32_t registerPath(const aiString& path, std::map<std::string, uint32_t>& reg) {
    //    if (path.length == 0) {
    //        return UINT32_MAX;
    //    }
    //    auto sPath = path.C_Str();
    //    auto it = reg.find(sPath);
    //    if (it != reg.end()) return it->second;
    //    reg[sPath] = (uint32_t)reg.size();
    //    return (uint32_t)reg.size() - 1;
    //}
    std::shared_ptr<Rendering::ShaderResourceView> loadTexture(
        int textureInfoIndex,
        const tinygltf::Model& model
    ) {
        auto& renderer = Rendering::Renderer::getInstance();

        if (textureInfoIndex >= 0) {
            auto textureObj = model.textures[textureInfoIndex];
            auto textureSource = model.images[textureObj.source];

            if (textureSource.bufferView < 0) {
                // todo.. it is pretty simple..
                log(LogLevel::WARNING, "external texture source is not supported yet, skipped");
            } else {
                auto buffer_view = model.bufferViews[textureSource.bufferView];
                auto buffer = model.buffers[buffer_view.buffer];
                auto image_data = &buffer.data[buffer_view.byteOffset];
                return renderer.createSimpleTexture2DFromWIC(image_data, buffer_view.byteLength);
            }
        }

        return nullptr;
    }


    std::shared_ptr<SceneManagement::DefaultMaterial> loadDefaultMaterial(
        const tinygltf::Material& mat, 
        const tinygltf::Model& model, 
        std::map<std::string, uint32_t> textureReg
    ) {
        auto& renderer = Rendering::Renderer::getInstance();
        SceneManagement::DefaultMaterialConstantData constants;
        static auto sConstantBuffer = renderer.createConstantBuffer(constants);

        std::shared_ptr<SceneManagement::DefaultMaterial> out(new SceneManagement::DefaultMaterial());

        constants.baseColor = DirectX::XMFLOAT4{
            (float)mat.pbrMetallicRoughness.baseColorFactor[0],
            (float)mat.pbrMetallicRoughness.baseColorFactor[1],
            (float)mat.pbrMetallicRoughness.baseColorFactor[2],
            (float)mat.pbrMetallicRoughness.baseColorFactor[3]
        };
        constants.emissionColor = DirectX::XMFLOAT4{
            (float)mat.emissiveFactor[0],
            (float)mat.emissiveFactor[1],
            (float)mat.emissiveFactor[2],
            (float)1
        };
        constants.metallic = (float)mat.pbrMetallicRoughness.metallicFactor;
        constants.roughness = (float)mat.pbrMetallicRoughness.roughnessFactor;
   
        constants.anisotropy  = 0; // 确实不支持。。 当然，有 extension
        // out.shader is set by the constructor


        out->texBaseColor = loadTexture(mat.pbrMetallicRoughness.baseColorTexture.index, model);
        if(out->texBaseColor) constants.uvBaseColor = mat.pbrMetallicRoughness.baseColorTexture.texCoord;
        
        out->texEmissionColor = loadTexture(mat.emissiveTexture.index, model);
        if(out->texEmissionColor) constants.uvEmissionColor = mat.emissiveTexture.texCoord;
        
        out->texMetallic = nullptr; // 不支持纹理
        constants.uvMetallic = UINT32_MAX;
        
        out->texRoughness = loadTexture(mat.pbrMetallicRoughness.metallicRoughnessTexture.index, model);
        if(out->texRoughness)constants.uvRoughness = mat.pbrMetallicRoughness.metallicRoughnessTexture.texCoord;

        out->texAO = loadTexture(mat.occlusionTexture.index, model);
        if(out->texAO)constants.uvAO = mat.occlusionTexture.texCoord;
        constants.occlusionStrength = (float)mat.occlusionTexture.strength;

        out->texNormal = loadTexture(mat.normalTexture.index, model);
        if(out->texNormal)constants.uvNormal = mat.normalTexture.texCoord;
        constants.normalMapScale = (float)mat.normalTexture.scale;

        // 不需要设置 default：只要 uv 没有设置，就不会去读取纹理的

        // 这个 sampler state 应该是随着 texture 变化的
        CD3D11_SAMPLER_DESC desc{ CD3D11_DEFAULT() };
        out->sampAO = renderer.createSamplerState(desc);
        out->sampBaseColor = renderer.createSamplerState(desc);
        out->sampEmissionColor = renderer.createSamplerState(desc);
        out->sampMetallic = renderer.createSamplerState(desc);
        out->sampNormal = renderer.createSamplerState(desc);
        out->sampRoughness = renderer.createSamplerState(desc);

        
        out->consantBuffers = sConstantBuffer->getSharedInstance();
        out->consantBuffers->cpuData<decltype(constants)>() = constants;

        return out;
    }

    std::shared_ptr<SceneManagement::Object> buildObjectHierarchy(
        const tinygltf::Node& inNode,
        const tinygltf::Model& model,
        std::shared_ptr<SceneManagement::Object> parent,
        const std::vector<std::vector<std::tuple<std::shared_ptr<Rendering::Mesh>, uint32_t, std::string>>>& meshes,
        const std::vector<std::shared_ptr<SceneManagement::DefaultMaterial>>& materials
    ) {
        std::string nodeName = inNode.name;

        int lightID = -1;

        auto itExtLight = inNode.extensions.find("KHR_lights_punctual");
        if (itExtLight != inNode.extensions.end()) {
            lightID = itExtLight->second.Get("light").GetNumberAsInt();
        }

        // 标准上没有明确说。。
        bool isMesh = inNode.mesh >= 0;
        bool isCamera = inNode.camera >= 0;
        bool isLight = lightID >= 0; // glTF 暂不支持灯光

        if ((isMesh ? 1 : 0) + (isCamera ? 1 : 0) + (isLight ? 1 : 0) > 1) {
            throw std::exception("an object can only be one of mesh, camera or light");
        }

        std::shared_ptr<SceneManagement::Object> outNode;

        if (isCamera) {
            auto& inCamera = model.cameras[inNode.camera];
            auto outCamera = new SceneManagement::Camera();
            
            if (inCamera.type == "orthographic") {
                outCamera->data.projectionType = Rendering::RenderingScene::CameraInfo::ProjectionType::ORTHOGRAPHICS;
                outCamera->data.viewWidth = (float)inCamera.orthographic.xmag * 2;
                outCamera->data.viewHeight = (float)inCamera.orthographic.ymag * 2;
                outCamera->data.nearZ = (float)inCamera.orthographic.znear;
                outCamera->data.farZ = (float)inCamera.orthographic.zfar;
            } else if (inCamera.type == "perspective") {
                outCamera->data.projectionType = Rendering::RenderingScene::CameraInfo::ProjectionType::PERSPECTIVE;
                outCamera->data.aspectRatio = (float)inCamera.perspective.aspectRatio;
                outCamera->data.fieldOfViewYRadian = (float)inCamera.perspective.yfov;
                outCamera->data.nearZ = (float)inCamera.perspective.znear;
                outCamera->data.farZ = (float)inCamera.perspective.zfar;
            } else {
                throw std::exception("invalid camera type");
            }
            outNode = decltype(outNode)(outCamera);
        } else if (isLight) {
            auto inLight = model.lights[lightID];
            auto outLight = new SceneManagement::Light();
            // TODO: point and spot lights use luminous intensity in candela (lm/sr) 
            // while directional lights use illuminance in lux (lm/m2)
            outLight->shadow = Rendering::LightShadow::LIGHT_SHADOW_HARD;
            outLight->maximumDistance =
                inLight.range > 0 ? (float)inLight.range : std::numeric_limits<float>::infinity();
            auto intensity = inLight.intensity;
            if (inLight.type == "point") {
                outLight->type = Rendering::LightType::LIGHT_TYPE_POINT;
                // 一个随意设置的数字。。旨在让渲染器适应 blender 的光照设置
                intensity /= 100;
            } else if (inLight.type == "directional") {
                outLight->type = Rendering::LightType::LIGHT_TYPE_DIRECTIONAL;
                outLight->direction_L = DirectX::XMFLOAT3{ 0, 0, -1 };
            } else if (inLight.type == "spot") {
                outLight->type = Rendering::LightType::LIGHT_TYPE_SPOT;
                outLight->innerConeAngle = (float)inLight.spot.innerConeAngle;
                outLight->outerConeAngle = (float)inLight.spot.outerConeAngle;
                outLight->direction_L = DirectX::XMFLOAT3{ 0, 0, -1 };
            } else {
                throw std::exception("unexpected light type");
            }
            outLight->intensity = DirectX::XMFLOAT3{
                (float)(inLight.color[0] * intensity),
                (float)(inLight.color[1] * intensity),
                (float)(inLight.color[2] * intensity)
            };
            outNode = decltype(outNode)(outLight);
        } else if(isMesh) {
            outNode = decltype(outNode)(new SceneManagement::Object());

            static std::shared_ptr<SceneManagement::DefaultMaterial> defaultMaterial(
                new SceneManagement::DefaultMaterial()
            );
            static std::once_flag onceFlag;
            std::call_once(onceFlag, [&]() {
                defaultMaterial->consantBuffers = Rendering::Renderer::getInstance().createConstantBuffer(SceneManagement::DefaultMaterialConstantData());
            });

            
            for (auto [mesh, matID, name] : meshes[inNode.mesh]) {
                auto meshObj = new SceneManagement::Mesh();
                meshObj->name = name;
                meshObj->parent = outNode;
                meshObj->data = mesh;
                // TODO default material
                if (matID == UINT32_MAX) meshObj->material = defaultMaterial;
                else meshObj->material = materials[matID];
                meshObj->transT = DirectX::XMFLOAT3{ 0, 0, 0 };
                meshObj->transR = DirectX::XMVECTOR{ 0, 0, 0, 1 };
                meshObj->transS = DirectX::XMFLOAT3{ 1, 1, 1 };

                outNode->children.push_back(std::shared_ptr<SceneManagement::Object>(meshObj));
            }
        } else {
            outNode = decltype(outNode)(new SceneManagement::Object());
        }

        // name
        outNode->name = nodeName;

        // transform
        /*aiVector3D aiScaling, aiPosition;
        aiQuaternion aiRotation;
        inNode.mTransformation.Decompose(aiScaling, aiRotation, aiPosition);*/
        if (inNode.matrix.empty()) {
            if (!inNode.translation.empty()) {
                outNode->transT = DirectX::XMFLOAT3{
                    (float)inNode.translation[0],
                    (float)inNode.translation[1],
                    (float)inNode.translation[2]
                };
            }

            if (!inNode.rotation.empty()) {
                outNode->transR = DirectX::XMVECTOR{
                    (float)inNode.rotation[0],
                    (float)inNode.rotation[1],
                    (float)inNode.rotation[2],
                    (float)inNode.rotation[3]
                };
            }

            if (!inNode.scale.empty()) {
                outNode->transS = DirectX::XMFLOAT3{
                    (float)inNode.scale[0],
                    (float)inNode.scale[1],
                    (float)inNode.scale[2]
                };
            }
        } else {
            auto& m = inNode.matrix;
            //DirectX::XMMATRIX trans{
            //    m[0x0], m[0x4], m[0x8], m[0xC],
            //    m[0x1], m[0x5], m[0x9], m[0xD],
            //    m[0x2], m[0x6], m[0xA], m[0xE],
            //    m[0x3], m[0x7], m[0xB], m[0xF]
            //};          
            // m 是列主序的存储方式，和 XM 刚好相反
            // m 是当作左乘的矩阵，XM 是右乘矩阵。。又刚好相反
            DirectX::XMMATRIX trans{
                (float)m[0x0], (float)m[0x1], (float)m[0x2], (float)m[0x3],
                (float)m[0x4], (float)m[0x5], (float)m[0x6], (float)m[0x7],
                (float)m[0x8], (float)m[0x9], (float)m[0xA], (float)m[0xB],
                (float)m[0xC], (float)m[0xD], (float)m[0xE], (float)m[0xF]
            };
            DirectX::XMVECTOR T, S;
            DirectX::XMMatrixDecompose(&S, &outNode->transR, &T, trans);
            DirectX::XMStoreFloat3(&outNode->transS, S);
            DirectX::XMStoreFloat3(&outNode->transT, T);
        }

        if (isCamera) {
            // 就这样就完成了左右手系转换了？？？？？没那么容易吧
            outNode->transS.z *= -1.f;
        }

        // hierarchy
        outNode->parent = parent;
        for (auto child: inNode.children) {
            outNode->children.push_back(buildObjectHierarchy(
                model.nodes[child], model, outNode, meshes, materials));
        }

        return outNode;
    }

    std::shared_ptr<SceneManagement::Object> loadDefaultResourceGLTF(const std::string& pathStr) {
        auto& renderer = Rendering::Renderer::getInstance();

        std::filesystem::path path(pathStr);

        Model model;
        TinyGLTF loader;
        std::string err;
        std::string warn;

        bool load_succeeded = false;

        if (path.extension() == ".glb") {
            load_succeeded =
                loader.LoadBinaryFromFile(&model, &err, &warn, pathStr);
        } else if (path.extension() == ".gltf") {
            load_succeeded =
                loader.LoadASCIIFromFile(&model, &err, &warn, pathStr);
        } else {
            throw std::exception("the extension of glTF2 file(`%s`) should be .glb or .gltf");
        }

        if (!warn.empty()) {
            log(LogLevel::WARNING, warn);
        }

        if (!load_succeeded) {
            throw std::exception(err.c_str());
        }

      
        std::map<std::string, std::uint32_t> textureReg;
        std::vector<SceneManagement::DefaultVertexData> vboData;
        std::vector<std::vector<DefaultMeshGLTF>> meshIn;
        std::vector<std::shared_ptr<SceneManagement::DefaultMaterial>> materials;
        std::vector<uint32_t> indices;
      
        for (auto mesh: model.meshes) {
            meshIn.push_back(loadDefaultMesh(mesh, model, vboData, indices));
        }

        std::vector<
            std::vector<
                std::tuple<std::shared_ptr<Rendering::Mesh>, uint32_t, std::string>
            >
        > meshes;

        for (auto& material: model.materials) {
            materials.push_back(loadDefaultMaterial(material, model, textureReg));
        }

        auto ido = renderer.createIndexBufferObject(indices);

        auto vbo = renderer.createVertexBufferObject(vboData, SceneManagement::DefaultVertexData::getDescription());
        for (auto meshGroup : meshIn) {
            meshes.push_back({});
            for (auto mesh : meshGroup) {
                meshes.rbegin()->push_back({ 
                    renderer.createMesh(vbo, ido, mesh.indexBegin, mesh.indexLength), 
                    mesh.materialID, 
                    mesh.name 
                });
            }
        }

        //textures.resize(texturereg.size());
        //for (auto [k, v] : texturereg) {
        //    out.textures[v].path = k;
        //}
        
        std::shared_ptr<SceneManagement::Object> rootObject(new SceneManagement::Object());
        rootObject->name = "__#ROOT_OBJECT";
        for (auto nodeID : model.scenes[model.defaultScene].nodes) {
            rootObject->children.push_back(
                buildObjectHierarchy(model.nodes[nodeID],
                    model, rootObject, meshes, materials)
            );
        }

        return rootObject;
    }
}

