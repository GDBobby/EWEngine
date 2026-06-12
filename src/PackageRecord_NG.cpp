#include "EWEngine/Imgui/ImNodes/Graph/PackageRecord_NG.h"

#include "EWEngine/EWEngine.h"
#include "EWEngine/Imgui/DragDrop.h"
#include "EightWinds/Command/InstructionPackage.h"
#include "EightWinds/Command/ObjectInstructionPackage.h"
#include "EightWinds/Command/PackageRecord.h"
#include "EightWinds/Reflect/Enum.h"
#include "imgui.h"
#include "imgui_internal.h"

namespace EWE{
namespace Node{
    PackageRecord_NG::PackageRecord_NG()
    : ImNodes::Editor{true, true},
        explorer{std::filesystem::current_path()},
        headNode{CreateHeadNode()}
    {
        explorer.acceptable_extensions = Global::assetManager->pkgRecord.files.acceptable_extensions;
    }

    ImNodes::Node* PackageRecord_NG::CreateHeadNode(){
        auto& head = AddNode();
        head.snapToGrid = true;

        head.pos = node_editor_window_pos;
        head.pins.emplace_back(ImNodes::Pin{.local_pos{1.f, 0.5f}, .payload{nullptr}});
        return &head;
    } 
    
    void PackageRecord_NG::RenderNodes() {
        ImNodes::Editor::RenderNodes();
        {
            Command::InstructionPackage* pkg;
            if(DragDropPtr::Target(pkg)) {
                auto& added_node = CreateRGNode(pkg);
                auto temp_mouse_pos =ImGui::GetIO().MousePos;
                added_node.pos = temp_mouse_pos;// - ImNodes::EditorContextGetPanning();// - (temp_min - window_pos);
            }
        }
        {
            RasterPackage* pkg;
            if(DragDropPtr::Target(pkg)) {
                auto& added_node = CreateRGNode(reinterpret_cast<Command::InstructionPackage*>(pkg));
                auto temp_mouse_pos =ImGui::GetIO().MousePos;
                added_node.pos = temp_mouse_pos;// - ImNodes::EditorContextGetPanning();// - (temp_min - window_pos);
            }
        }
        {
            Command::ObjectPackage* pkg;
            if(DragDropPtr::Target(pkg)) {
                auto& added_node = CreateRGNode(reinterpret_cast<Command::InstructionPackage*>(pkg));
                auto temp_mouse_pos =ImGui::GetIO().MousePos;
                added_node.pos = temp_mouse_pos;// - ImNodes::EditorContextGetPanning();// - (temp_min - window_pos);
            }
        }
    }

    ImNodes::Node& PackageRecord_NG::CreateRGNode(Command::InstructionPackage* pkg) {
        auto& added_node = AddNode();
        added_node.payload = pkg;
        added_node.pos = menu_pos;
        added_node.pins.emplace_back(ImNodes::Pin{.local_pos{0.f, 0.5f}, .payload{nullptr}});
        added_node.pins.emplace_back(ImNodes::Pin{.local_pos{1.f, 0.5f}, .payload{nullptr}});

        return added_node;
    }

    void PackageRecord_NG::OpenAddMenu() {
        add_menu_is_open = true;
        
    }

    bool PackageRecord_NG::RenderAddMenu() {
        bool wantsClose = false;

        bool window_not_focused;

        ImGui::SetNextWindowPos(menu_pos);
        if(ImGui::Begin("add menu")){

            if(ImGui::BeginListBox("##lb record")){

                window_not_focused = !ImGui::IsWindowFocused();

                ImGui::EndListBox();
            }
        }
        ImGui::End();
        return wantsClose | window_not_focused;
    }

    void PackageRecord_NG::LinkEmptyDrop(ImNodes::Node& src_node, ImNodes::PinOffset pin_offset) {

        menu_pos = ImGui::GetMousePos();
        add_menu_is_open = true;
    }

    void PackageRecord_NG::LinkCreated(ImNodes::NodePair& link) {
        //Log::Debug("link created\n");
        link.start.node->pins[link.start.offset].payload = link.end.node;
        link.end.node->pins[link.end.offset].payload = link.start.node;
    }

    void PackageRecord_NG::LinkDestroyed(ImNodes::NodePair& link) {
        //Log::Debug("link destroyed\n");

        link.start.node->pins[link.start.offset].payload = nullptr;
        link.end.node->pins[link.end.offset].payload = nullptr;
        ImNodes::Editor::LinkDestroyed(link);
    }

    void PackageRecord_NG::RenderNode(ImNodes::Node& node){
        ::ImNodes::BeginNodeTitleBar();
        if(node.payload == nullptr){
            EWE_ASSERT(headNode == &node);
            //ImGui::InputText("name of package record");
            ImGui::Text("head node");
            ::ImNodes::EndNodeTitleBar();

            ImGui::SetNextItemWidth(150.f);
            Reflect::Enum::Imgui_Combo_Selectable("Queue type", queue_type);
            return;
        }
        auto* node_payload = reinterpret_cast<Command::InstructionPackage*>(node.payload);
        ImGui::Text(node_payload->name.c_str());

        ::ImNodes::EndNodeTitleBar();
        if(ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && ImGui::IsWindowHovered()){
            //open the graph for the package
            switch(node_payload->type){
                case Command::InstructionPackage::Type::Base:
                    OpenGraph(Type::InstructionPackage, node_payload);
                    break;
                case Command::InstructionPackage::Type::Object:
                    OpenGraph(Type::ObjectPackage, node_payload);
                    break;
                case Command::InstructionPackage::Type::Raster:
                    OpenGraph(Type::RasterPackage, node_payload);
                    break;
                default: EWE_UNREACHABLE;
            }
        }
        ImGui::Text("node type : %s\n", Reflect::Enum::ToString(node_payload->type).data());
        switch(node_payload->type){
            case Command::InstructionPackage::Type::Base:
            case Command::InstructionPackage::Type::Object:{
                for(auto& inst : node_payload->paramPool.instructions){
                    ImGui::BulletText(Reflect::Enum::ToString(inst).data());
                }
                break;
            }
            case Command::InstructionPackage::Type::Raster:{
                for(auto& obj_pkg : reinterpret_cast<RasterPackage*>(node_payload)->objectPackages){
                    if(ImGui::TreeNode(obj_pkg->name.string().c_str())){
                        for(auto& inst : obj_pkg->paramPool.instructions){
                            ImGui::BulletText(Reflect::Enum::ToString(inst).data());
                        }
                        ImGui::TreePop();
                    }
                }
                break;
            }
            default: EWE_UNREACHABLE;
        }
    }

    void PackageRecord_NG::RenderPin(ImNodes::Node& node, ImNodes::PinOffset pin_index) {
        auto& pin = node.pins[pin_index];

        if (::ImNodes::BeginPinAttribute(node.id + pin_index + 1, pin.local_pos)) {
            //ImGui::Text("pin");
        }
        ::ImNodes::EndPinAttribute();
    }

    bool PackageRecord_NG::SaveFunc() {
        explorer.enabled = save_open;
        explorer.state = ExplorerContext::State::Save;
        if(ImGui::Begin("file save")){
            explorer.Imgui();
            if(explorer.selected_file.has_value()){
                const std::filesystem::path saved_path = *explorer.selected_file;

                Command::PackageRecord record{};
                record.queue = &engine->GetQueue(queue_type);
                record.name = saved_path;
                ImNodes::Node* current_node = reinterpret_cast<ImNodes::Node*>(headNode->pins[0].payload);
                while(current_node != nullptr){
                    record.packages.push_back(reinterpret_cast<Command::InstructionPackage*>(current_node->payload));
                    current_node = reinterpret_cast<ImNodes::Node*>(current_node->pins[1].payload);
                }
                const auto proximate_path = std::filesystem::proximate(
                    saved_path,
                    EWE::Global::assetManager->pkgRecord.files.root_directory
                );
                Log::Debug("writing package record to file : %s / %s\n",
                    EWE::Global::assetManager->pkgRecord.files.root_directory.string().c_str(), 
                    proximate_path.string().c_str()
                );
                Asset::WriteAssetToFile(
                    record, 
                    EWE::Global::assetManager->pkgRecord.files.root_directory, 
                    proximate_path
                );

                explorer.enabled = false;
                explorer.selected_file.reset();
            }
            if(!ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows)){
                //auto imwin = ImGui::GetCurrentWindow();
                explorer.enabled = false;
            }
        }
        ImGui::End();
        return !explorer.enabled;
    }

    bool PackageRecord_NG::LoadFunc() {
        explorer.enabled = load_open;
        explorer.state = ExplorerContext::State::Load;
        if(ImGui::Begin("file load")){
            explorer.Imgui();
            if(explorer.selected_file.has_value()){
                const std::filesystem::path load_path = *explorer.selected_file;
                
                const auto localized_path = std::filesystem::proximate(load_path, Global::assetManager->pkgRecord.files.root_directory);
                auto* record = Global::assetManager->pkgRecord.Get(localized_path);
                if(record == nullptr){
                    Log::Warning("failed to read record from file : %s\n", localized_path.string().c_str());
                }
                else{
                    InitFromObject(*record);
                }

                explorer.enabled = false;
                explorer.selected_file.reset();
            }
            if(!ImGui::IsWindowFocused()){
                //auto imwin = ImGui::GetCurrentWindow();
                explorer.enabled = false;
            }
        }
        ImGui::End();
        return !explorer.enabled;
    }

    void PackageRecord_NG::InitFromObject(Command::PackageRecord& record){
        nodes.Clear();
        CreateHeadNode();
        links.clear();

        if(record.packages.size() == 0){
            return;
        }
        ImNodes::Node* last_node = nullptr;
        {
            auto& node = CreateRGNode(record.packages.front());
            last_node = &node;
            node.pins[0].payload = headNode;
            headNode->pins[0].payload = &node;
        }

        links.push_back(
            ImNodes::NodePair{
                .start{
                    .node = headNode,
                    .offset = 0
                },
                .end{
                    .node = last_node,
                    .offset = 0
                }
            }
        );
        for(std::size_t i = 1; i < record.packages.size(); i++){
            auto& node = CreateRGNode(record.packages[i]);
            last_node->pins[1].payload = &node;
            node.pins[0].payload = last_node;

            links.push_back(
                ImNodes::NodePair{
                    .start{
                        .node = last_node,
                        .offset = 1
                    },
                    .end{
                        .node = &node,
                        .offset = 0
                    }
                }
            );
            last_node = &node;
        }

    }
} //namespace Node
} //namespace EWE