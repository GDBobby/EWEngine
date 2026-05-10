#pragma once

#include "EightWinds/RenderGraph/RenderGraph.h"
#include "EWEngine/Tools/ImguiFileExplorer.h"

#include "EWEngine/Imgui/ImNodes/imnodes_ewe.h"

namespace EWE{
    namespace Node{
        struct RenderGraph_NG : public ImNodes::EWE::Editor {

            ExplorerContext explorer;
            ImNodes::EWE::Node* headNode;

            enum NodeType{
                TaskGroup,
                RenderInfo,
            };
            struct NodePayload{
                NodeType type;
                void* payload;
            };

            [[nodiscard]] explicit RenderGraph_NG();
            
            void RenderEditorTitle() override final;
            
            ImNodes::EWE::Node* CreateHeadNode();
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