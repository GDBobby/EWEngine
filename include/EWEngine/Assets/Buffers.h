#pragma once

#include "EWEngine/Assets/Hash.h"
#include "EightWinds/Buffer.h"
#include "EightWinds/Data/KeyValueContainer.h"

#include "EWEngine/Assets/Base.h"


namespace EWE{
namespace Asset{

    template<>
    struct Manager<Buffer>{
        //FileSystem files; //no files for the buffers, yet at least

        Hive<Buffer, 64> data_arena;
        KeyValueContainer<AssetHash, Buffer*> association_container{};

        [[nodiscard]] explicit Manager(std::filesystem::path const& root_path);

        static AssetHash GetHash(Buffer const& buf){
            return CrossPlatformPathHash(buf.name);
        }

        void UpdateMetaFile(AssetHash hash);
        void UpdateMetaFile(AssetHash hash, Buffer& img);

        void Destroy(AssetHash hash);
        void Destroy(Buffer& buf);

        Buffer& Get(AssetHash hash);
        Buffer& Get(std::filesystem::path const& name);
        Buffer& Get(std::filesystem::path const& name,
            VkDeviceSize instanceSize, uint32_t instanceCount, 
            VmaAllocationCreateInfo const& vmaAllocCreateInfo, 
            VkBufferUsageFlags usageFlags = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
        );

        AssetHash ConvertBDAToHash(VkDeviceAddress addr);

#ifdef EWE_IMGUI
        void Imgui();
#endif
    };
} //namespace Asset
} //namespace EWE