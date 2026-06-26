#pragma once

#include "EWEngine/Assets/Base.h"
#include "EightWinds/DescriptorImageInfo.h"


#include "EWEngine/Assets/Samplers.h"
#include "EWEngine/Assets/ImageViews.h"

namespace EWE{
namespace Asset{

    struct DiiCreation{
        Sampler* sampler = nullptr;
        ImageView* view = nullptr;
        DescriptorType type = DescriptorType::Combined;
        VkImageLayout layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    };
    //this is a litle specialized, so I'll leave it as is
    template<>
    struct Manager<DescriptorImageInfo>{
        FileSystem files;

        [[nodiscard]] explicit Manager(
            std::filesystem::path const& root_path
        );

        [[nodiscard]] explicit Manager(LogicalDevice& logicalDevice);
        
        std::mutex mut;
        Hive<DescriptorImageInfo, 64> data_arena;
        KeyValueContainer<AssetHash, DescriptorImageInfo*> association_container;

        void Destroy(AssetHash hash);
        void Destroy(DescriptorImageInfo* dii);

        DescriptorImageInfo* Get(AssetHash hash);
        DescriptorImageInfo* Get(std::filesystem::path const& name);

        //dont use this over ConstructInto, this is specifically fgor imgui and will be replaced
        DescriptorImageInfo& Get(DiiCreation const& params);

        template<typename... Args>
        requires std::constructible_from<DescriptorImageInfo, Args...>
        DescriptorImageInfo& ConstructInto(Args&&... args) {
            std::unique_lock<std::mutex> lock{mut};
            auto& ele = data_arena.AddElement(std::forward<Args>(args)...);
            AssetHash hash = GetHash(ele);
            association_container.push_back(hash, &ele);
            return ele;
        }

        AssetHash ConvertTextureIndexToHash(TextureIndex index) const;
        DescriptorImageInfo const& RevertIndex(TextureIndex index) const;

#ifdef EWE_IMGUI
        KeyValueContainer<DescriptorImageInfo*, ImTextureRef> imgui_texture_refs;
        DiiCreation creation_params;
        bool showCreation = false;
        void Imgui();
#endif
    };
    template<>
    bool LoadAssetFromFile(DescriptorImageInfo* ptr_to_raw_mem, std::filesystem::path const& root_directory, std::filesystem::path const& file_name);
    template<>
    bool WriteAssetToFile(DescriptorImageInfo const& dii, std::filesystem::path const& root_directory, std::filesystem::path const& file_name);

} //namespace Asset
} //namespace EWE