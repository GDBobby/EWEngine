#pragma once

#include "EWEngine/Assets/FileResource.h"

#include "EWEngine/Assets/Hash.h"
#include "EWEngine/Assets/Base.h"


namespace EWE{
    struct Shader;
namespace Asset{

        template<>
        struct Manager<Shader>{
        FileSystem files;

        Hive<Shader, 64> data_arena;
        KeyValueContainer<AssetHash, Shader*> association_container;

        static AssetHash GetHash(Shader const& shader){
            return CrossPlatformPathHash(shader.filepath);
        }

        //read from file
        //optional
        bool GetMeta(ShaderMeta& meta, AssetHash hash);
        bool GetMeta(ShaderMeta& meta, std::filesystem::path const& file_path);

        [[nodiscard]] explicit Manager(std::filesystem::path root_directory);

        void Destroy(AssetHash hash);
        void Destroy(Shader& shader);

        Shader& Get(AssetHash hash);
        Shader& Get(std::filesystem::path const& file_path);

#if EWE_IMGUI
        void Imgui();
#endif
    };
} //namespace Asset
} //namespace EWE