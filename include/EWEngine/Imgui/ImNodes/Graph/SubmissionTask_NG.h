#pragma once

#include "EightWinds/Command/PackageRecord.h"
#include "EightWinds/Command/ParamPointerChain.h"

#include "EightWinds/RenderGraph/GPUTask.h"
#include "EightWinds/RenderGraph/SubmissionTask.h"

#include "EWEngine/Imgui/ImguiFileExplorer.h"

#include "EWEngine/Imgui/ImNodes/imnodes_ewe.h"

namespace EWE{
namespace Node{

    struct SubmissionTask_NG : ImNodes::Editor {
        ExplorerContext explorer;
        ImNodes::Node* headNode;

        Queue::Type current_queue_type = Queue::Type::COUNT;

        AttachmentInfo generate_attachment_info{};

        struct TaskMetaPayload{
            //NodePayload::Type type = NodePayload::Type::Task;
            GPUTask* task;

            std::vector<bool> raster_open;

            //construct on task creation, attempt to load meta file
            GPUTaskMeta_Helper meta_helper;
            
            [[nodiscard]] explicit TaskMetaPayload(Command::PackageRecord* record);
            [[nodiscard]] explicit TaskMetaPayload(GPUTask* task);
        };

        [[nodiscard]] explicit SubmissionTask_NG();

        void RenderNodes() override final;

        ImNodes::Node* CreateHeadNode();
        ImNodes::Node& CreateRGNode(Command::PackageRecord* record);
        ImNodes::Node& CreateRGNode(GPUTask* task);

        //void ImGuiNodeDebugPrint(ImNodes::Node& node) const override final;
        void OpenAddMenu() override final;
        bool RenderAddMenu() override final;
        void LinkEmptyDrop(ImNodes::Node& src_node, ImNodes::PinOffset pin_offset) override final;
        void LinkCreated(ImNodes::NodePair& link) override final;
        void LinkDestroyed(ImNodes::NodePair& link) override final;
        void RenderNode(ImNodes::Node& node) override final;
        void RenderPin(ImNodes::Node& node, ImNodes::PinOffset pin_index) override final;
        bool SaveFunc() override final;
        bool LoadFunc() override final;

        void PopulateFromGraph(SubmissionTask& subTask);
        void InitFromObject(SubmissionTask& subTask);

        std::vector<GPUTask*> CollectTasks();
        void ReadjustAttachmentPins(ImNodes::Node& node, std::size_t raster_index, bool value);
    };
} //namespace Node 
} //namespace EWE