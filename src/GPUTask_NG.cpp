#include "EWEngine/Imgui/ImNodes/Graph/GPUTask_NG.h"

#include "EWEngine/Global.h"
#include "EWEngine/Imgui/DragDrop.h"
#include "EightWinds/Command/PackageRecord.h"
#include "imgui.h"
#include "imgui_internal.h"

namespace EWE{
namespace Node{
    GPUTask_NG::GPUTask_NG()
    : ImNodes::EWE::Editor{true, true},
        explorer{std::filesystem::current_path()},
        headNode{CreateHeadNode()}
    {
        explorer.acceptable_extensions.push_back(".egt");
    }

    ImNodes::EWE::Node* GPUTask_NG::CreateHeadNode(){
        auto& head = AddNode();
        head.snapToGrid = true;

        head.pos = node_editor_window_pos;
        head.pins.emplace_back(ImNodes::EWE::Pin{.local_pos{1.f, 0.5f}, .payload{nullptr}});
        return &head;
    } 
    
    void GPUTask_NG::RenderNodes() {
        ImNodes::EWE::Editor::RenderNodes();
        Command::PackageRecord* pkg;
        if(DragDropPtr::Target(pkg)) {
            auto& added_node = CreateRGNode(pkg);
            auto temp_mouse_pos =ImGui::GetIO().MousePos;
            added_node.pos = temp_mouse_pos;// - ImNodes::EditorContextGetPanning();// - (temp_min - window_pos);
        }
    }

    ImNodes::EWE::Node& GPUTask_NG::CreateRGNode(Command::PackageRecord* pkg) {
        auto& added_node = AddNode();
        added_node.payload = pkg;
        added_node.pos = menu_pos;
        added_node.pins.emplace_back(ImNodes::EWE::Pin{.local_pos{0.f, 0.5f}, .payload{nullptr}});
        added_node.pins.emplace_back(ImNodes::EWE::Pin{.local_pos{1.f, 0.5f}, .payload{nullptr}});

        return added_node;
    }

    void GPUTask_NG::OpenAddMenu() {
        add_menu_is_open = true;
        
    }

    bool GPUTask_NG::RenderAddMenu() {
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

    void GPUTask_NG::LinkEmptyDrop(ImNodes::EWE::Node& src_node, ImNodes::EWE::PinOffset pin_offset) {

        menu_pos = ImGui::GetMousePos();
        add_menu_is_open = true;
    }

    void GPUTask_NG::LinkCreated(ImNodes::EWE::NodePair& link) {
        //Logger::Print<Logger::Debug>("link created\n");
        link.start.node->pins[link.start.offset].payload = link.end.node;
        link.end.node->pins[link.end.offset].payload = link.start.node;
    }

    void GPUTask_NG::LinkDestroyed(ImNodes::EWE::NodePair& link) {
        //Logger::Print<Logger::Debug>("link destroyed\n");

        link.start.node->pins[link.start.offset].payload = nullptr;
        link.end.node->pins[link.end.offset].payload = nullptr;
        ImNodes::EWE::Editor::LinkDestroyed(link);
    }

    void GPUTask_NG::RenderNode(ImNodes::EWE::Node& node){
        ImNodes::BeginNodeTitleBar();
        if(node.payload == nullptr){
            EWE_ASSERT(headNode == &node);
            //ImGui::InputText("name of package record");
            ImGui::Text("head node");
            ImNodes::EndNodeTitleBar();
            ImGui::Text(" "); //filler text
            return;
        }
        auto* node_payload = reinterpret_cast<Command::InstructionPackage*>(node.payload);
        ImGui::Text(node_payload->name.c_str());

        ImNodes::EndNodeTitleBar();

        for(auto& inst : node_payload->paramPool.instructions){
            ImGui::BulletText(Reflect::Enum::ToString(inst).data());
        }
    }

    void GPUTask_NG::RenderPin(ImNodes::EWE::Node& node, ImNodes::EWE::PinOffset pin_index) {
        auto& pin = node.pins[pin_index];

        if (ImNodes::BeginPinAttribute(node.id + pin_index + 1, pin.local_pos)) {
            //ImGui::Text("pin");
        }
        ImNodes::EndPinAttribute();
    }

    bool GPUTask_NG::SaveFunc() {
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

    bool GPUTask_NG::LoadFunc() {
        explorer.enabled = load_open;
        explorer.state = ExplorerContext::State::Load;
        if(ImGui::Begin("file load")){
            explorer.Imgui();
            if(explorer.selected_file.has_value()){
                const std::filesystem::path load_path = *explorer.selected_file;
                //const std::filesystem::path temp_path = std::filesystem::proximate(load_path, Global::gpuTasks->files.root_directory);

                //auto& task = Global::gpuTasks->Get(temp_path);
                //LoadFromTask(task);


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


    void GPUTask_NG::LoadFromTask(GPUTask& task){
        nodes.Clear();
        links.clear();
        CreateHeadNode();

        ImNodes::EWE::Node* last_node = headNode;
        if(task.pkgRecord->packages.size() == 0){
            return;
        }
        /*
        {
            auto& node = CreateRGNode(subTask.tasks.front());
            headNode->pins[0].payload = &node;
            links.AddElement(
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

            links.AddElement(
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
            */
    }
    void GPUTask_NG::WriteIntoTask(GPUTask& task){

    }
    void GPUTask_NG::CreateTask(){

        /*
        Command::PackageRecord record{};
        record.name = name;
        record.queue = &Global::stcManager->renderQueue;
        record.packages;

        ImNodes::EWE::Node* current_node = headNode->pins[0].payload;
        while(current_node != nullptr){
            record.packages.push_back(current_node->payload);
            current_node = reinterpret_cast<ImNodes::EWE::Node*>(current_node.pins[1].payload);
        }

        Global::gpuTasks->Create(name, Global::gpuTasks->logicalDevice, record);
        */
    }
} //namespace Node
} //namespace EWE