#pragma once

#include "../Renderer/Resources.h"
#include "../Scene/DefaultDS.h"

namespace LiteEngine::IO {
    
    struct DefaultMeshGLTF {
        std::string name;
        uint32_t materialID = 0;
        uint32_t indexBegin;
        uint32_t indexLength;
    };

    struct DefaultTexture2D {
        // 目前先只支持外挂的 texture
        // 加载模型的时候可以把非外挂的 texture 也存到文件中
        std::string path;
    };
    
    // T: 可以序列化的数据
    template<typename T>
    struct Hierarchy {
        uint32_t lchild = UINT32_MAX;
        uint32_t rchild = UINT32_MAX;
        T value;
    };

    std::shared_ptr<SceneManagement::Object> loadDefaultResourceGLTF(const std::string& pathStr);

}