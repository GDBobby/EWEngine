#pragma once

#include "EightWinds/RenderGraph/RenderGraph.h"

#include "EWEngine/Imgui/ImNodes/imnodes_ewe.h"
#include "imgui.h"

namespace EWE{
    namespace Node{
        struct RenderGraph_NG : public ImNodes::EWE::Editor {
            enum NodeType{
                Task,
                RenderInfo,
            };
            struct NodePayload{
                NodeType type;
                void* payload;
            };

            RenderGraph* renderGraph;

            [[nodiscard]] explicit RenderGraph_NG();
            [[nodiscard]] explicit RenderGraph_NG(RenderGraph& _renderGraph);

            void RenderEditorTitle() override final;
            ImNodes::EWE::Node& CreateRGNode(SubmissionTask* subTask);
            ImNodes::EWE::Node& CreateRGNode(FullRenderInfo* renderInfo);

            void RenderNodes() override final;
            bool RenderAddMenu() override final;
            void RenderNode(ImNodes::EWE::Node& node) override final;
            void RenderPin(ImNodes::EWE::Node& node, ImNodes::EWE::PinOffset pin_index) override final;

            void InitFromObject(RenderGraph& _renderGraph);
        };
    }
}