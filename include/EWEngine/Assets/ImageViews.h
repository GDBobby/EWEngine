#pragma once

#include "EightWinds/Data/KeyValueContainer.h"

#include "EWEngine/Assets/Images.h"

namespace EWE{
namespace Asset{

    template<>
    struct Manager<ImageView>{
        LogicalDevice& logicalDevice;
        Manager<Image>& images;

        [[nodiscard]] explicit Manager(LogicalDevice& logicalDevice, Manager<Image>& images);

        static AssetHash GetHash(ImageView const& view){
            return Manager<Image>::GetHash(view.image);
        }

        [[nodiscard]] explicit Manager(LogicalDevice& logicalDevice);

        Hive<ImageView, 64> data_arena;
        KeyValueContainer<AssetHash, ImageView*> association_container;

        void Destroy(AssetHash condensed_val);
        void Destroy(ImageView* sampler);

        ImageView& Get(AssetHash hash);

        ImageView& Get(Image& image);
    };
} //namespace Asset
} //namespace EWE