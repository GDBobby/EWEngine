#pragma once

#include "EightWinds/Data/KeyValueContainer.h"

#include "EWEngine/Assets/Images.h"

#include "EightWinds/ImageView.h"

namespace EWE{
namespace Asset{

    template<>
    struct Manager<ImageView>{
        using Type = ImageView;

        Manager<Image>& images;

        [[nodiscard]] explicit Manager(std::filesystem::path const& root_dir, Manager<Image>& images);

        static AssetHash GetHash(ImageView const& view){
            return Manager<Image>::GetHash(view.image);
        }

        Hive<ImageView, 64> data_arena;
        KeyValueContainer<AssetHash, ImageView*> association_container;

        void Destroy(AssetHash condensed_val);
        void Destroy(ImageView* sampler);

        ImageView& Get(AssetHash hash);

        ImageView& Get(Image& image);

#if EWE_IMGUI
        void Imgui();
#endif
    };
} //namespace Asset
} //namespace EWE