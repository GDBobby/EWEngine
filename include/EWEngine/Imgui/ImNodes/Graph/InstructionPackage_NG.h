#pragma once


#include "EightWinds/Command/ParamPool.h"
#include "EightWinds/Command/InstructionPackage.h"
#include "EWEngine/Imgui/ImguiFileExplorer.h"

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

    struct InstructionPackage_NG : ImNodes::Editor {

        ExplorerContext explorer;
        ImNodes::Node* headNode;
        std::vector<Inst::Type> acceptable_add_instructions{};
        ImGuiTextFilter filter;

        Command::ParamPool paramPool;
        Command::InstructionPackage::Type previous_package_type = Command::InstructionPackage::Base;
        Command::InstructionPackage::Type packageType = Command::InstructionPackage::Base;
        void SetPackageType(Command::InstructionPackage::Type pkg_type);

        bool changing_package_type_allowed = false;
        void* package_payload;
        PushConstant object_push;

        [[nodiscard]] explicit InstructionPackage_NG();
        //this is just going to call InitFromPkg
        [[nodiscard]] explicit InstructionPackage_NG(Command::InstructionPackage& pkg);

        void RecreateLinks();

        ImNodes::Node* CreateHeadNode();
        static Inst::Type GetInstructionFromNode(ImNodes::Node& node);
        void PrintNode(ImNodes::Node& node) const;
        void RenderEditorTitle() override final;
        void RenderNodes() override final;
        ImNodes::Node& CreateRGNode(Inst::Type iType);
        std::vector<Inst::Type> CollectInstructionsUpTo(ImNodes::Node* limit_node) const;
        inline std::vector<Inst::Type> CollectInstructions() const{
            return CollectInstructionsUpTo(nullptr);
        }
        void InsertNodeToParamPool(ImNodes::Node* inserted_node);
        void UpdateNodeOffsets();
        bool AddInstructionButton(Inst::Type itype);

        bool InsertLink(ImNodes::Node& added_node, ImNodes::Node* node);

        void ImGuiNodeDebugPrint(ImNodes::Node& node) const override final;
        void OpenAddMenu() override final;
        bool RenderAddMenu() override final;
        void LinkEmptyDrop(ImNodes::Node& src_node, ImNodes::PinOffset pin_offset) override final;
        void LinkCreated(ImNodes::NodePair& link) override final;
        void LinkDestroyed(ImNodes::NodePair& link) override final;
        void RenderNode(ImNodes::Node& node) override final;
        void RenderPin(ImNodes::Node& node, ImNodes::PinOffset pin_index) override final;
        bool SaveFunc() override final;
        bool LoadFunc() override final;


        void InitFromFile(Command::ParamPool const& pp, Command::InstructionPackage::Type pkg_type);
        void InitFromObject(Command::InstructionPackage& pkg);
        void InitFromObject(void* payload) override final{
            InitFromObject(*reinterpret_cast<Command::InstructionPackage*>(payload));
        }
    };
} //namespace Node
} //namespace EWE