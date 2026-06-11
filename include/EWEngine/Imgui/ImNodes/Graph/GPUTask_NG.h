#pragma once

#include "EightWinds/RenderGraph/GPUTask.h"
#include "EWEngine/Imgui/ImguiFileExplorer.h"

#include "EWEngine/Imgui/ImNodes/imnodes_ewe.h"

/*
    i can skip this and build this directly from the packagerecord
*/


namespace EWE{
namespace Node{

    struct GPUTask_NG : ImNodes::Editor {
        ExplorerContext explorer;
        ImNodes::Node* headNode;

        std::string name{"default"};

        [[nodiscard]] explicit GPUTask_NG();

        void RenderNodes() override final;

        ImNodes::Node* CreateHeadNode();
        ImNodes::Node& CreateRGNode(Command::PackageRecord* rec);

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

        void LoadFromTask(GPUTask& task);
        void WriteIntoTask(GPUTask& task);

        void CreateTask();
    };
} //namespace Node 
} //namespace EWE