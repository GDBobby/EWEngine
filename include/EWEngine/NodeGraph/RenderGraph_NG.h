#pragma once

#include "EightWinds/RenderGraph/RenderGraph.h"

#include "EWEngine/Imgui/ImNodes/imnodes_ewe.h"
#include "imgui.h"

namespace EWE{
    namespace Node{
        struct RenderGraphNodeGraph : public ImNodes::EWE::Editor {
            RenderGraph& renderGraph;

            [[nodiscard]] explicit RenderGraphNodeGraph(RenderGraph& _renderGraph);

            void RenderEditorTitle() override final;

            bool RenderAddMenu(ImVec2 menu_pos);

            void RenderNode(ImNodes::EWE::Node& node) override final;

            void RenderPin(ImNodes::EWE::Node& node, ImNodes::EWE::PinOffset pin_index) override final;
        };
    }
}