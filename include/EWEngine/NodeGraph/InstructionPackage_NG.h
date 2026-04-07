#pragma once


#include "EightWinds/Command/ParamPool.h"
#include "EightWinds/Reflect/Enum.h"
#include "EightWinds/Command/Record.h"
#include "EightWinds/Command/InstructionPackage.h"
#include "EightWinds/Command/RasterInstructionPackage.h"

#include "EWEngine/Tools/ImguiFileExplorer.h"

#include "EWEngine/Imgui/ImNodes/imnodes_ewe.h"

namespace EWE{
namespace Node{
    /*
        i can stop links from being made if it's not a valid instruction (binding descriptor out of a pipeline)

        i can only allow certain nodes to be made at a certain point, using link drop

        im putting that ^^^^ off for now
    */
    
    struct InstNodePayload{
        enum Type{
            Head,
            Instruction,
            Package //maybe only a record can contain a package? idk
        };
        Type type;
        Inst::Type iType;

        //if this is negative, the node is not connected to head
        int distanceFromHead = -1; 
    };

    struct InstructionPackageNodeGraph : ImNodes::EWE::Editor {

        ExplorerContext explorer;
        ImNodes::EWE::Node* headNode;
        std::vector<Inst::Type> acceptable_add_instructions{};
        ImGuiTextFilter filter;

        Command::ParamPool paramPool;

        Command::InstructionPackage::Type packageType = Command::InstructionPackage::Base;
        bool changing_package_type_allowed = true;

        [[nodiscard]] explicit InstructionPackageNodeGraph();

        ImNodes::EWE::Node* CreateHeadNode();
        static Inst::Type GetInstructionFromNode(ImNodes::EWE::Node& node);
        void PrintNode(ImNodes::EWE::Node& node) const;
        void RenderNodes();
        ImNodes::EWE::Node& CreateRGNode(Inst::Type iType);
        std::vector<Inst::Type> CollectInstructionsUpTo(ImNodes::EWE::Node* limit_node) const;
        inline std::vector<Inst::Type> CollectInstructions() const{
            return CollectInstructionsUpTo(nullptr);
        }
        void InsertNodeToParamPool(ImNodes::EWE::Node* inserted_node);
        void UpdateNodeOffsets();
        bool AddInstructionButton(Inst::Type itype);
        void CreateFromInstructions(std::span<const Inst::Type> create_instructions);

        bool InsertLink(ImNodes::EWE::Node& added_node, ImNodes::EWE::Node* node);

        void ImGuiNodeDebugPrint(ImNodes::EWE::Node& node) const override final;
        void OpenAddMenu() override final;
        bool RenderAddMenu() override final;
        void LinkEmptyDrop(ImNodes::EWE::Node& src_node, ImNodes::EWE::PinOffset pin_offset) override final;
        void LinkCreated(ImNodes::EWE::NodePair& link) override final;
        void LinkDestroyed(ImNodes::EWE::NodePair& link) override final;
        void RenderNode(ImNodes::EWE::Node& node) override final;
        void RenderPin(ImNodes::EWE::Node& node, ImNodes::EWE::PinOffset pin_index) override final;
        bool SaveFunc() override final;
        bool LoadFunc() override final;
    };
} //namespace Node
} //namespace EWE