#pragma once


#include "EightWinds/Reflect/Enum.h"
#include "EightWinds/Command/PackageRecord.h"

#include "EWEngine/Imgui/ImguiFileExplorer.h"

#include "EWEngine/Imgui/ImNodes/imnodes_ewe.h"
#include "imgui.h"

namespace EWE{
namespace Node{

    struct PackageRecord_NG : ImNodes::Editor {
        ExplorerContext explorer;
        ImNodes::Node* headNode;

        Queue::Type queue_type = Queue::Graphics;

        [[nodiscard]] explicit PackageRecord_NG();

        void RenderNodes() override final;

        ImNodes::Node* CreateHeadNode();
        ImNodes::Node& CreateRGNode(Command::InstructionPackage* record);

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

        void InitFromObject(Command::PackageRecord& record);        
        void InitFromObject(void* payload) override final{
            InitFromObject(*reinterpret_cast<Command::PackageRecord*>(payload));
        }
    };
} //namespace Node 
} //namespace EWE