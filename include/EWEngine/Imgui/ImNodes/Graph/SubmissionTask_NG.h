#pragma once

#include "EightWinds/Command/PackageRecord.h"
#include "EightWinds/Command/ParamPointerChain.h"

#include "EightWinds/RenderGraph/GPUTask.h"
#include "EightWinds/RenderGraph/SubmissionTask.h"

#include "EWEngine/Tools/ImguiFileExplorer.h"

#include "EWEngine/Imgui/ImNodes/imnodes_ewe.h"

namespace EWE{
namespace Node{

    struct SubmissionTask_NG : ImNodes::EWE::Editor {
        ExplorerContext explorer;
        ImNodes::EWE::Node* headNode;

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

        ImNodes::EWE::Node* CreateHeadNode();
        ImNodes::EWE::Node& CreateRGNode(Command::PackageRecord* record);
        ImNodes::EWE::Node& CreateRGNode(GPUTask* task);

        //void ImGuiNodeDebugPrint(ImNodes::EWE::Node& node) const override final;
        void OpenAddMenu() override final;
        bool RenderAddMenu() override final;
        void LinkEmptyDrop(ImNodes::EWE::Node& src_node, ImNodes::EWE::PinOffset pin_offset) override final;
        void LinkCreated(ImNodes::EWE::NodePair& link) override final;
        void LinkDestroyed(ImNodes::EWE::NodePair& link) override final;
        void RenderNode(ImNodes::EWE::Node& node) override final;
        void RenderPin(ImNodes::EWE::Node& node, ImNodes::EWE::PinOffset pin_index) override final;
        bool SaveFunc() override final;
        bool LoadFunc() override final;

        void PopulateFromGraph(SubmissionTask& subTask);
        void InitFromObject(SubmissionTask& subTask);

        std::vector<GPUTask*> CollectTasks();
        void ReadjustAttachmentPins(ImNodes::EWE::Node& node, std::size_t raster_index, bool value);
    };
} //namespace Node 
} //namespace EWE