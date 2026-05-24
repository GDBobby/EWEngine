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
            struct SubTaskPayload{
                NodeType type = TaskGroup;
                std::vector<SubmissionTask*>* sub_group;

                //sub_task_index, task_index
                std::vector<std::vector<GPUTaskMeta_Helper>> task_meta_helpers{};

                //per sub task, then per task, then per raster pkg
                std::vector<std::vector<std::vector<bool>>> raster_pkg_open;
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

            bool SaveFunc() override final;
            bool LoadFunc() override final;

            void InitFromObject(RenderGraph& _renderGraph);
        };
    }
}