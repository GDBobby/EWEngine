#pragma once

#include "EWEngine/Assets/Base.h"
#include "EWEngine/Assets/Images.h"
#include "EightWinds/ImageView.h"

namespace EWE{
namespace Asset{

    //a little specialized, views are limited to a full-image-view
    template<>
    struct Manager<ImageView>{
        using Type = ImageView;

        Manager<Image>& images;

        [[nodiscard]] explicit Manager(std::filesystem::path const& root_dir, Manager<Image>& images);


        Hive<ImageView, 64> data_arena;
        KeyValueContainer<AssetHash, ImageView*> association_container;

        void Destroy(AssetHash hash);
        void Destroy(ImageView& view);

        ImageView* Get(AssetHash hash);
        ImageView* Get(Image& image);

#if EWE_IMGUI
        void Imgui();
#endif
    };
} //namespace Asset
} //namespace EWE