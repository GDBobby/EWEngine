#pragma once


#include "EightWinds/Reflect/Enum.h"
#include "EightWinds/Command/PackageRecord.h"

#include "EWEngine/Tools/ImguiFileExplorer.h"

#include "EWEngine/Imgui/ImNodes/imnodes_ewe.h"
#include "imgui.h"

namespace EWE{
namespace Node{

    struct PackageRecord_NG : ImNodes::EWE::Editor {
        ExplorerContext explorer;
        ImNodes::EWE::Node* headNode;

        Queue::Type queue_type = Queue::Graphics;

        [[nodiscard]] explicit PackageRecord_NG();

        void RenderNodes() override final;

        ImNodes::EWE::Node* CreateHeadNode();
        ImNodes::EWE::Node& CreateRGNode(Command::InstructionPackage* record);

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

        void InitFromObject(Command::PackageRecord& record);
    };
} //namespace Node 
} //namespace EWE