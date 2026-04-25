#pragma once

#include "EightWinds/Buffer.h"
#include "EWEngine/Assets/Base.h"



namespace EWE{
namespace Asset{

    //buffers are a little specialized, they won't be reading from files

    template<>
    struct Manager<Buffer>{
        //FileSystem files; //no files for the buffers, yet at least

        Hive<Buffer, 64> data_arena;
        KeyValueContainer<AssetHash, Buffer*> association_container{};

        [[nodiscard]] explicit Manager(std::filesystem::path const& root_path);


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

    //npt currently in use
    //template<>
    //void WriteMetaFile(Buffer& buf);
} //namespace Asset
} //namespace EWE