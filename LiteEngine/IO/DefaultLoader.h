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
        // Ŀǰ��ֻ֧����ҵ� texture
        // ����ģ�͵�ʱ����԰ѷ���ҵ� texture Ҳ�浽�ļ���
        std::string path;
    };
    
    // T: �������л�������
    template<typename T>
    struct Hierarchy {
        uint32_t lchild = UINT32_MAX;
        uint32_t rchild = UINT32_MAX;
        T value;
    };

    std::shared_ptr<SceneManagement::Object> loadDefaultResourceGLTF(const std::string& pathStr);

}