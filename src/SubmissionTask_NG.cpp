#include "EWEngine/NodeGraph/SubmissionTask_NG.h"

#include "EWEngine/Imgui/DragDrop.h"
#include "EWEngine/Global.h"
#include "EightWinds/Command/PackageRecord.h"
#include "imgui.h"
#include "imgui_internal.h"
#include <filesystem>

namespace EWE{
namespace Node{
    SubmissionTask_NG::SubmissionTask_NG()
    : ImNodes::EWE::Editor{true, true},
        explorer{std::filesystem::current_path()},
        headNode{CreateHeadNode()}
    {
        explorer.acceptable_extensions.push_back(".ewrg");
    }

    ImNodes::EWE::Node* SubmissionTask_NG::CreateHeadNode(){
        auto& head = AddNode();
        head.snapToGrid = true;

        head.pos = node_editor_window_pos;
        head.pins.emplace_back(ImNodes::EWE::Pin{.local_pos{1.f, 0.5f}, .payload{nullptr}});
        return &head;
    } 
    
    void SubmissionTask_NG::RenderNodes() {
        ImNodes::EWE::Editor::RenderNodes();
        Command::PackageRecord** pkg;
        if(DragDropPtr::Target(pkg)) {
            Logger::Print("dropping in sub task\n");
            auto temp_min = ImGui::GetItemRectMin();
            auto temp_max =ImGui::GetItemRectMax();
            GPUTask* task = new GPUTask((*pkg)->name, *Global::logicalDevice, **pkg);
            auto& added_node = CreateRGNode(task);
            auto temp_mouse_pos =ImGui::GetIO().MousePos;
            auto window_pos =  ImGui::GetWindowPos();
            added_node.pos = temp_mouse_pos;// - ImNodes::EditorContextGetPanning();// - (temp_min - window_pos);
        }
    }

    ImNodes::EWE::Node& SubmissionTask_NG::CreateRGNode(GPUTask* task) {
        auto& added_node = AddNode();
        added_node.payload = task;
        added_node.pos = menu_pos;
        added_node.pins.emplace_back(ImNodes::EWE::Pin{.local_pos{0.f, 0.5f}, .payload{nullptr}});
        added_node.pins.emplace_back(ImNodes::EWE::Pin{.local_pos{1.f, 0.5f}, .payload{nullptr}});

        return added_node;
    }

    void SubmissionTask_NG::OpenAddMenu() {
        add_menu_is_open = true;
        
    }

    bool SubmissionTask_NG::RenderAddMenu() {
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

    void SubmissionTask_NG::LinkEmptyDrop(ImNodes::EWE::Node& src_node, ImNodes::EWE::PinOffset pin_offset) {

        menu_pos = ImGui::GetMousePos();
        add_menu_is_open = true;
    }

    void SubmissionTask_NG::LinkCreated(ImNodes::EWE::NodePair& link) {
        //Logger::Print<Logger::Debug>("link created\n");
        link.start.node->pins[link.start.offset].payload = link.end.node;
        link.end.node->pins[link.end.offset].payload = link.start.node;
    }

    void SubmissionTask_NG::LinkDestroyed(ImNodes::EWE::NodePair& link) {
        //Logger::Print<Logger::Debug>("link destroyed\n");

        link.start.node->pins[link.start.offset].payload = nullptr;
        link.end.node->pins[link.end.offset].payload = nullptr;
        ImNodes::EWE::Editor::LinkDestroyed(link);
    }

    void SubmissionTask_NG::RenderNode(ImNodes::EWE::Node& node){
        ImNodes::BeginNodeTitleBar();
        if(node.payload == nullptr){
            EWE_ASSERT(headNode == &node);
            //ImGui::InputText("name of package record");
            ImGui::Text("head node");
            ImNodes::EndNodeTitleBar();
            ImGui::Text(""); //filler text
            return;
        }
        auto* node_payload = reinterpret_cast<GPUTask*>(node.payload);
        ImGui::Text(node_payload->name.c_str());

        ImNodes::EndNodeTitleBar();

        if(node_payload->pkgRecord->packages.size() == 0){
            ImGui::Text("no packages");
        }

        for(auto& pkg : node_payload->pkgRecord->packages){
            ImGui::BulletText(pkg->name.c_str());
        }
    }

    void SubmissionTask_NG::RenderPin(ImNodes::EWE::Node& node, ImNodes::EWE::PinOffset pin_index) {
        auto& pin = node.pins[pin_index];

        if (ImNodes::BeginPinAttribute(node.id + pin_index + 1, pin.local_pos)) {
            //ImGui::Text("pin");
        }
        ImNodes::EndPinAttribute();
    }

    bool SubmissionTask_NG::SaveFunc() {
        explorer.enabled = save_open;
        explorer.state = ExplorerContext::State::Save;
        if(ImGui::Begin("file save")){
            explorer.Imgui();
            if(explorer.selected_file.has_value()){
                const std::filesystem::path saved_path = *explorer.selected_file;



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

    bool SubmissionTask_NG::LoadFunc() {
        explorer.enabled = load_open;
        explorer.state = ExplorerContext::State::Load;
        if(ImGui::Begin("file load")){
            explorer.Imgui();
            if(explorer.selected_file.has_value()){
                const std::filesystem::path load_path = *explorer.selected_file;

                const std::filesystem::path temp = std::filesystem::proximate(load_path, Global::subTasks->files.root_directory);

                auto& subTask = Global::subTasks->Get(temp);
                LoadFromTask(subTask);

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

    void SubmissionTask_NG::CreateFromGraph(SubmissionTask& subTask){
        subTask.tasks.clear();
        ImNodes::EWE::Node* current_node = reinterpret_cast<ImNodes::EWE::Node*>(headNode->pins[0].payload);
        while(current_node != nullptr){
            subTask.tasks.push_back(reinterpret_cast<GPUTask*>(current_node->payload));
            current_node = reinterpret_cast<ImNodes::EWE::Node*>(current_node->pins[1].payload);
        }
        subTask.CollectTaskWorkloads();
    }

    void SubmissionTask_NG::LoadFromTask(SubmissionTask& subTask){
        nodes.Clear();
        CreateHeadNode();
        links.clear();
        ImNodes::EWE::Node* last_node = headNode;

        if(subTask.tasks.size() == 0){
            return;
        }
        {
            auto& node = CreateRGNode(subTask.tasks.front());
            headNode->pins[0].payload = &node;
            links.push_back(
                ImNodes::EWE::NodePair{
                    .start{
                        .node = last_node,
                        .offset = 0,
                    },
                    .end{
                        .node = &node,
                        .offset = 0
                    }
                }
            );
            node.pins[0].payload = headNode;
        }

        for(std::size_t i = 1; i < subTask.tasks.size(); i++){
            auto& node = CreateRGNode(subTask.tasks[i]);

            links.push_back(
                ImNodes::EWE::NodePair{
                    .start{
                        .node = last_node,
                        .offset = 1,
                    },
                    .end{
                        .node = &node,
                        .offset = 0
                    }
                }
            );
            last_node->pins[1].payload = &node;
            node.pins[0].payload = last_node;
        }
    }
} //namespace Node
} //namespace EWE