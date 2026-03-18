#pragma once

#include "EWEngine/Assets/FileResource.h"

#include "EWEngine/Assets/Hash.h"
#include "EWEngine/Assets/Manager.h"


namespace EWE{
    struct Shader;
namespace Asset{

        template<>
        struct Manager<Shader>{
        FileSystem files;
        LogicalDevice& logicalDevice;

        static AssetHash GetHash(Shader const& shader){
            return CrossPlatformPathHash(shader.filepath);
        }

        [[nodiscard]] explicit Manager(LogicalDevice& logicalDevice, std::filesystem::path root_directory);

        std::unordered_map<std::filesystem::path, Shader*> shaders;

        Shader* Get(std::string_view file_path);

#if EWE_IMGUI
        void Imgui();
#endif
    };
} //namespace Asset
} //namespace EWE