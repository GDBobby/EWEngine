#pragma once


#include "EightWinds/RenderGraph/RasterPackage.h"
#include "EWEngine/Imgui/ImguiFileExplorer.h"
#include "EWEngine/Imgui/ImNodes/imnodes_ewe.h"

namespace EWE{
namespace Node{

    struct RasterPackage_NG : ImNodes::Editor {
        ExplorerContext explorer;
        ImNodes::Node* headNode;

        TaskRasterConfig task_config;
        AssetHash render_info_hash = Asset::INVALID_HASH;

        [[nodiscard]] explicit RasterPackage_NG();

        void RenderNodes() override final;

        ImNodes::Node* CreateHeadNode();
        ImNodes::Node& CreateRGNode(Command::ObjectPackage* pkg);

        void RenderEditorTitle() override final;
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

        void InitFromObject(RasterPackage& rt);
        void InitFromObject(void* payload) override final{
            InitFromObject(*reinterpret_cast<RasterPackage*>(payload));
        }
    };
} //namespace Node 
} //namespace EWE