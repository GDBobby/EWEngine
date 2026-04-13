#include "EWEngine/Imgui/ImNodes/Graph/RasterPackage_NG.h"

#include "EWEngine/Global.h"
#include "EWEngine/Imgui/DragDrop.h"
#include "EightWinds/Command/InstructionPackage.h"
#include "EightWinds/Command/ObjectInstructionPackage.h"
#include "EightWinds/Reflect/Enum.h"
#include "imgui.h"
#include "imgui_internal.h"


namespace EWE{
namespace Node{
    RasterPackage_NG::RasterPackage_NG()
    : ImNodes::EWE::Editor{true, true},
        explorer{std::filesystem::current_path()},
        headNode{CreateHeadNode()}
    {
        explorer.acceptable_extensions = Global::assetManager->pkgRecord.files.acceptable_extensions;
    }

    ImNodes::EWE::Node* RasterPackage_NG::CreateHeadNode(){
        auto& head = AddNode();
        head.snapToGrid = true;

        head.pos = node_editor_window_pos;
        head.pins.emplace_back(ImNodes::EWE::Pin{.local_pos{1.f, 0.5f}, .payload{nullptr}});
        return &head;
    } 
    
    void RasterPackage_NG::RenderNodes() {
        ImNodes::EWE::Editor::RenderNodes();
        Command::ObjectPackage* pkg;
        if(DragDropPtr::Target(pkg)) {
            if(pkg->type == Command::InstructionPackage::Type::Object){
                auto& added_node = CreateRGNode(pkg);
                auto temp_mouse_pos =ImGui::GetIO().MousePos;
                added_node.pos = temp_mouse_pos;// - ImNodes::EditorContextGetPanning();// - (temp_min - window_pos);
            }
        }
    }

    ImNodes::EWE::Node& RasterPackage_NG::CreateRGNode(Command::ObjectPackage* pkg) {
        auto& added_node = AddNode();
        added_node.payload = pkg;
        added_node.pos = menu_pos;
        added_node.pins.emplace_back(ImNodes::EWE::Pin{.local_pos{0.f, 0.5f}, .payload{nullptr}});
        added_node.pins.emplace_back(ImNodes::EWE::Pin{.local_pos{1.f, 0.5f}, .payload{nullptr}});

        return added_node;
    }

    void RasterPackage_NG::OpenAddMenu() {
        add_menu_is_open = true;
        
    }

    bool RasterPackage_NG::RenderAddMenu() {
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

    void RasterPackage_NG::LinkEmptyDrop(ImNodes::EWE::Node& src_node, ImNodes::EWE::PinOffset pin_offset) {

        menu_pos = ImGui::GetMousePos();
        add_menu_is_open = true;
    }

    void RasterPackage_NG::LinkCreated(ImNodes::EWE::NodePair& link) {
        //Logger::Print<Logger::Debug>("link created\n");
        link.start.node->pins[link.start.offset].payload = link.end.node;
        link.end.node->pins[link.end.offset].payload = link.start.node;
    }

    void RasterPackage_NG::LinkDestroyed(ImNodes::EWE::NodePair& link) {
        //Logger::Print<Logger::Debug>("link destroyed\n");

        link.start.node->pins[link.start.offset].payload = nullptr;
        link.end.node->pins[link.end.offset].payload = nullptr;
        ImNodes::EWE::Editor::LinkDestroyed(link);
    }

    void RasterPackage_NG::RenderNode(ImNodes::EWE::Node& node){
        ImNodes::BeginNodeTitleBar();
        if(node.payload == nullptr){
            EWE_ASSERT(headNode == &node);
            //ImGui::InputText("name of package record");
            ImGui::Text("head node");
            ImNodes::EndNodeTitleBar();
            ImGui::Text(" ");

            return;
        }
        auto* node_payload = reinterpret_cast<Command::ObjectPackage*>(node.payload);
        ImGui::Text(node_payload->name.c_str());

        ImNodes::EndNodeTitleBar();
        if(ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && ImGui::IsWindowHovered()){
            //open the graph for the package
            OpenGraph(Type::InstructionPackage, node_payload);
        }

        for(auto& inst : node_payload->paramPool.instructions){
            ImGui::BulletText(Reflect::Enum::ToString(inst).data());
        }
    }

    void RasterPackage_NG::RenderPin(ImNodes::EWE::Node& node, ImNodes::EWE::PinOffset pin_index) {
        auto& pin = node.pins[pin_index];

        if (ImNodes::BeginPinAttribute(node.id + pin_index + 1, pin.local_pos)) {
            //ImGui::Text("pin");
        }
        ImNodes::EndPinAttribute();
    }

    bool RasterPackage_NG::SaveFunc() {
        explorer.enabled = save_open;
        explorer.state = ExplorerContext::State::Save;
        if(ImGui::Begin("file save")){
            explorer.Imgui();
            if(explorer.selected_file.has_value()){
                const std::filesystem::path saved_path = *explorer.selected_file;

                RasterPackage rt{saved_path.string(), *Global::logicalDevice, Global::stcManager->renderQueue, task_config, nullptr};
                ImNodes::EWE::Node* current_node = reinterpret_cast<ImNodes::EWE::Node*>(headNode->pins[0].payload);
                while(current_node != nullptr){
                    rt.objectPackages.push_back(reinterpret_cast<Command::ObjectPackage*>(current_node->payload));
                    current_node = reinterpret_cast<ImNodes::EWE::Node*>(current_node->pins[1].payload);
                }
                EWE::Global::assetManager->rasterTask.WriteToFile(rt);

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

    bool RasterPackage_NG::LoadFunc() {
        explorer.enabled = load_open;
        explorer.state = ExplorerContext::State::Load;
        if(ImGui::Begin("file load")){
            explorer.Imgui();
            if(explorer.selected_file.has_value()){
                const std::filesystem::path load_path = *explorer.selected_file;
                
                const auto localized_path = std::filesystem::proximate(load_path, Global::assetManager->rasterTask.files.root_directory);
                auto& rt = Global::assetManager->rasterTask.Get(localized_path);
                InitFromObject(rt);

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

    void RasterPackage_NG::InitFromObject(RasterPackage& record){
        nodes.Clear();
        CreateHeadNode();
        links.clear();

        if(record.objectPackages.size() == 0){
            return;
        }
        ImNodes::EWE::Node* last_node = nullptr;
        {
            auto& node = CreateRGNode(record.objectPackages.front());
            last_node = &node;
            node.pins[0].payload = headNode;
            headNode->pins[0].payload = &node;
        }

        links.push_back(
            ImNodes::EWE::NodePair{
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
        for(std::size_t i = 1; i < record.objectPackages.size(); i++){
            auto& node = CreateRGNode(record.objectPackages[i]);
            last_node->pins[1].payload = &node;
            node.pins[0].payload = last_node;

            links.push_back(
                ImNodes::EWE::NodePair{
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