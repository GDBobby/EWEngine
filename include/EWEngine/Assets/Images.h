#pragma once

#include "EWEngine/Assets/Hash.h"
#include "EightWinds/Sampler.h"
#include "EightWinds/Data/KeyValueContainer.h"

#include "EWEngine/Assets/Manager.h"

namespace EWE{
namespace Asset{

    template<>
    struct Manager<Image>{
        LogicalDevice& logicalDevice;
        FileSystem image_files;
        FileSystem meta_files;

        Hive<Image, 64> data_arena;
        KeyValueContainer<AssetHash, Image*> association_container{};

        [[nodiscard]] explicit Manager(LogicalDevice& logicalDevice, std::filesystem::path const& root_path);

        static AssetHash GetHash(Image const& img){
            return CrossPlatformPathHash(img.name);
        }

        Image::Data LoadMetaData(AssetHash hash);

        void UpdateMetaFile(AssetHash hash);
        void UpdateMetaFile(AssetHash hash, Image& img);

        void Destroy(AssetHash hash);
        void Destroy(Image* sampler);

        Image* Get(AssetHash hash);
        Image* Get(std::string_view name);

#ifdef EWE_IMGUI
        void Imgui();
#endif
    };
} //namespace Asset
} //namespace EWE