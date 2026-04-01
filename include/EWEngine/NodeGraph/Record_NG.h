#pragma once


#include "EightWinds/Reflect/Enum.h"
#include "EightWinds/Command/Record.h"

#include "EWEngine/Tools/ImguiFileExplorer.h"

#include "EWEngine/Imgui/ImNodes/imnodes_ewe.h"
#include "imgui.h"

#include <fstream>

namespace EWE{
namespace Node{

    /*
        i can stop links from being made if it's not a valid instruction (binding descriptor out of a pipeline)

        i can only allow certain nodes to be made at a certain point, using link drop

        im putting that ^^^^ off for now
    */

    struct RecordNodeGraph : ImNodes::EWE::Editor {
        ExplorerContext explorer;
        ImNodes::EWE::Node* headNode;
        std::vector<Inst::Type> acceptable_add_instructions{};
        ImNodes::EWE::Node* link_empty_drop_srcNode = nullptr;
        ImGuiTextFilter filter;

        [[nodiscard]] explicit RecordNodeGraph();

        void RenderNodes() override final;

        ImNodes::EWE::Node* CreateHeadNode();
        static Inst::Type GetInstructionFromNode(ImNodes::EWE::Node& node);
        void PrintNode(ImNodes::EWE::Node& node) const;
        ImNodes::EWE::Node& CreateRGNode(int inst_index);
        std::vector<Inst::Type> CollectInstructionsUpTo(ImNodes::EWE::Node* limit_node) const;

        inline std::vector<Inst::Type> CollectInstructions() const{
            return CollectInstructionsUpTo(nullptr);
        }
        void CreateFromInstructions(std::span<const Inst::Type> create_instructions);
        bool AddInstructionButton(Inst::Type itype);

        void ImGuiNodeDebugPrint(ImNodes::EWE::Node& node) const override final;
        void OpenAddMenu() override final;
        bool RenderAddMenu() override final;
        void LinkEmptyDrop(ImNodes::EWE::Node& src_node, ImNodes::EWE::PinOffset pin_offset) override final;
        void LinkCreated(ImNodes::EWE::NodePair& link) override final;
        void RenderNode(ImNodes::EWE::Node& node) override final;
        void RenderPin(ImNodes::EWE::Node& node, ImNodes::EWE::PinOffset pin_index) override final;
        bool SaveFunc() override final;
        bool LoadFunc() override final;
    };
} //namespace Node 
} //namespace EWE