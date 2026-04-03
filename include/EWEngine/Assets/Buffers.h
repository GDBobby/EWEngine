#pragma once

#include "EWEngine/Assets/Hash.h"
#include "EightWinds/Buffer.h"
#include "EightWinds/Data/KeyValueContainer.h"

#include "EWEngine/Assets/Manager.h"

namespace EWE{
namespace Asset{

    template<>
    struct Manager<Buffer>{
        LogicalDevice& logicalDevice;
        FileSystem files;
        //FileSystem meta_files;

        Hive<Buffer, 64> data_arena;
        KeyValueContainer<AssetHash, Buffer*> association_container{};

        [[nodiscard]] explicit Manager(LogicalDevice& logicalDevice, std::filesystem::path const& root_path);

        static AssetHash GetHash(Buffer const& buf){
            return CrossPlatformPathHash(buf.name);
        }

        //Image::Data LoadMetaData(KeyValuePair<AssetHash, std::filesystem::path> const& img_kvp);

        void UpdateMetaFile(AssetHash hash);
        void UpdateMetaFile(AssetHash hash, Buffer& img);

        void Destroy(AssetHash hash);
        void Destroy(Buffer* buf);

        Buffer* Get(AssetHash hash);
        Buffer* Get(std::string_view name);

        AssetHash ConvertBDAToHash(VkDeviceAddress addr);

#ifdef EWE_IMGUI
        void Imgui();
#endif
    };
} //namespace Asset
} //namespace EWE