#pragma once

#include "EightWinds/VulkanHeader.h"
#include "EightWinds/Image.h"
#include "EightWinds/ImageView.h"
#include "EightWinds/DescriptorImageInfo.h"
#include "EightWinds/Sampler.h"
#include "EightWinds/Command/ObjectInstructionPackage.h"

namespace EWE {

    struct FontObject;
    //i dont think i want to put these items in the asset manager?
    //but i'd like them to be in one specific memory area?
    //hive in textoverlay maybe?
    struct Font {
        std::string name;

        //implementation is hidden, it includes harfbuzz and freetype which I want separated
        FontObject* font;

        struct GraphicsPackage {
            ImageView view;
            DescriptorImageInfo dii;
        };
        Image image;
        Sampler& sampler;
        ImageView& view; //ref to graphicsPkg.view
        DescriptorImageInfo& dii;//ref to graphicsPkg.dii

        ConstructionDelayer<GraphicsPackage> graphicsPkg;

        bool ReadyForUsage() const{ return graphicsPkg.constructed; }

        //do I want a object package per font?
        Command::ObjectPackage objPkg;

        [[nodiscard]] explicit Font(std::string_view path, int pxSize);
        ~Font();
    };
}//namespace EWE