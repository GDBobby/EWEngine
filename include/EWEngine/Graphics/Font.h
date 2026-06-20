#pragma once

#include "EWEngine/Data/Vertex.h"

#include "EightWinds/VulkanHeader.h"
#include "EightWinds/Image.h"
#include "EightWinds/ImageView.h"
#include "EightWinds/DescriptorImageInfo.h"
#include "EightWinds/Sampler.h"
#include "EightWinds/Command/ObjectInstructionPackage.h"

namespace EWE {

    struct FontObject;

    static constexpr VertexProperty FontVertexProperty2{
        .position = 3,
        .uv = true
    };
    //i dont think i want to put these items in the asset manager?
    //but i'd like them to be in one specific memory area?
    //hive in textoverlay maybe?
    struct Font {
        using Vert = Vertex<FontVertexProperty2>;
        static constexpr uint16_t MAX_CHAR_COUNT = 4096;

        std::filesystem::path name;

        //implementation is hidden, it includes harfbuzz and freetype which I want separated
        FontObject* font;

        Command::ObjectPackage objPkg;

        PerFlight<Buffer> buffer;
        PerFlight<std::string> string_debugger;

        Image image;
        Sampler& sampler;

        struct GraphicsPackage {
            ImageView view;
            DescriptorImageInfo dii;
        };
        ConstructionDelayer<GraphicsPackage> graphicsPkg;
        ImageView& view; //ref to graphicsPkg.view
        DescriptorImageInfo& dii;//ref to graphicsPkg.dii


        void AddText(TextStruct const& ts);
        bool ReadyForUsage() const{ return graphicsPkg.constructed; }
        float GetStringWidth(TextStruct const& ts);

        InstructionPointer<ParamPack<Inst::If>> ifPack;
        InstructionPointer<ParamPack<Inst::Push>> pushPack;
        InstructionPointer<ParamPack<Inst::Draw>> drawPack;

        std::string label_name;
        uint16_t char_instance_count = 0;
        void EndRenderUpdate();

        [[nodiscard]] explicit Font(std::filesystem::path const& path, int pxSize, Sampler& _sampler);
        ~Font();
    };
}//namespace EWE