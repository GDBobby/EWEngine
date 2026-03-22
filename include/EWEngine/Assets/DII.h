#pragma once

#include "EWEngine/Assets/Hash.h"
#include "EightWinds/DescriptorImageInfo.h"
#include "EightWinds/Data/KeyValueContainer.h"

#include "EWEngine/Assets/Manager.h"

#include "EWEngine/Assets/Samplers.h"
#include "EWEngine/Assets/ImageViews.h"
#include <vulkan/vulkan_core.h>

namespace EWE{
namespace Asset{

    template<>
    struct Manager<DescriptorImageInfo>{
        LogicalDevice& logicalDevice;
        Manager<Sampler>& samplers;
        Manager<ImageView>& views;
        FileSystem filesystem;

        [[nodiscard]] explicit Manager(
            LogicalDevice& logicalDevice,
            Manager<Sampler>& samplers,
            Manager<ImageView>& views,
            std::filesystem::path const& root_path
        );

        static AssetHash GetHash(DescriptorImageInfo const& dii){
            return CrossPlatformPathHash(dii.name);
        }

        [[nodiscard]] explicit Manager(LogicalDevice& logicalDevice);
        
        Hive<DescriptorImageInfo, 64> data_arena;
        KeyValueContainer<AssetHash, DescriptorImageInfo*> association_container;

        void Destroy(AssetHash hash);
        void Destroy(DescriptorImageInfo* dii);

        DescriptorImageInfo& Get(AssetHash hash);
        DescriptorImageInfo& Get(std::string_view name);

        void Read(std::filesystem::path const& file_name);
        void Write(DescriptorImageInfo const& dii, std::filesystem::path const& file_name);

        struct Creation{
            Sampler* sampler = nullptr;
            ImageView* view = nullptr;
            DescriptorType type = DescriptorType::Combined;
            VkImageLayout layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        };

        DescriptorImageInfo& Get(Creation params);

#ifdef EWE_IMGUI
        KeyValueContainer<DescriptorImageInfo*, ImTextureRef> imgui_texture_refs;
        Creation creation_params;
        bool showCreation = false;
        void Imgui();
#endif
    };
} //namespace Asset
} //namespace EWE