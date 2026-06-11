#include "EWEngine/Imgui/ImNodes/Graph/SubmissionTask_NG.h"

#include "EWEngine/Assets/GPUTasks.h"
#include "EWEngine/Assets/Hash.h"
#include "EWEngine/Imgui/DragDrop.h"
#include "EWEngine/EWEngine.h"
//#include "EWEngine/Imgui/ImNodes/Graph/PackageRecord_NG.h"
#include "EightWinds/Backend/RenderInfo.h"

#include "EightWinds/Command/InstructionPackage.h"
#include "EightWinds/Command/PackageRecord.h"

#include "LAB/Support/Generic.h"

#include "imgui.h"
#include "imgui_internal.h"
#include <filesystem>

namespace EWE{
namespace Node{

    GPUTask* GetTask(Command::PackageRecord* record){ 
        EWE_ASSERT(record != nullptr);
        std::filesystem::path task_path = record->name;
        task_path.replace_extension("egt");
        GPUTask* ret = Global::assetManager->gpuTask.Get(task_path);
        if(ret == nullptr) {
            return &Global::assetManager->gpuTask.ConstructInto(record->name, engine->logicalDevice, *record, false);
        }
        return ret;
    }

    void SubmissionTask_NG::ReadjustAttachmentPins(ImNodes::Node& node, std::size_t raster_index, bool value){
        TaskMetaPayload& taskPayload = *reinterpret_cast<TaskMetaPayload*>(node.payload);
        GPUTask& task = *taskPayload.task;
        RasterPackage* raster_pkg = nullptr;
        {
            std::size_t current_raster_index = 0;
            for(auto* pkg : task.pkgRecord->packages){
                if(pkg->type == Command::InstructionPackage::Type::Raster){
                    if(current_raster_index == raster_index){
                        raster_pkg = static_cast<RasterPackage*>(pkg);
                        break;
                    }
                    current_raster_index++;
                }
            }
            EWE_ASSERT(raster_pkg != nullptr);
        }
        if(value){ //the pins are being added
            AttachmentSetInfo const& att_set_info = raster_pkg->task_config.attachment_info;
            float current_y = 0.2f;
            for(std::size_t i = 0; i < att_set_info.colors.Size(); i++) {
                node.pins.emplace_back(ImNodes::Pin{.local_pos{0.1f, current_y}, .payload{nullptr}});
                node.pins.emplace_back(ImNodes::Pin{.local_pos{0.9f, current_y}, .payload{nullptr}});
                current_y += 0.1f;
            }
            if(att_set_info.using_depth){
                node.pins.emplace_back(ImNodes::Pin{.local_pos{0.1f, current_y}, .payload{nullptr}});
                node.pins.emplace_back(ImNodes::Pin{.local_pos{0.9f, current_y}, .payload{nullptr}});
            }
        }
        else{
            node.pins.erase(node.pins.begin() + 2, node.pins.end());
        }
    }

    SubmissionTask_NG::TaskMetaPayload::TaskMetaPayload(Command::PackageRecord* record)
    : task{GetTask(record)},
        meta_helper{*task, GPUTaskMeta_Helper::HelperType::SubTask}   
    {
        for(auto* pkg : task->pkgRecord->packages){
            if(pkg->type == Command::InstructionPackage::Type::Raster){
                raster_open.push_back(false);
            }
        }
    }

    SubmissionTask_NG::TaskMetaPayload::TaskMetaPayload(GPUTask* _task)
    : task{_task},
        meta_helper{*task, GPUTaskMeta_Helper::HelperType::SubTask}   
    {
        for(auto* pkg : task->pkgRecord->packages){
            if(pkg->type == Command::InstructionPackage::Type::Raster){
                raster_open.push_back(false);
            }
        }
    }


    SubmissionTask_NG::SubmissionTask_NG()
    : ImNodes::Editor{true, true},
        explorer{std::filesystem::current_path()},
        headNode{CreateHeadNode()}
    {
        explorer.acceptable_extensions = Global::assetManager->subTask.files.acceptable_extensions;
    }

    ImNodes::Node* SubmissionTask_NG::CreateHeadNode(){
        auto& head = AddNode();
        head.snapToGrid = true;

        head.pos = node_editor_window_pos;
        head.pins.emplace_back(ImNodes::Pin{.local_pos{1.f, 0.5f}, .payload{nullptr}});
        return &head;
    } 
    
    void SubmissionTask_NG::RenderNodes() {
        ImNodes::Editor::RenderNodes();
        {
            Command::PackageRecord* pkg;
            if(DragDropPtr::Target(pkg)) {
                Log::Debug("dropping in sub task\n");


                auto const task_queue_type = engine->GetQueueType(*pkg->queue);
                bool adding_allowed = false;
                if(current_queue_type != task_queue_type){
                    if(headNode->pins[0].payload == nullptr){
                        current_queue_type = task_queue_type;
                        adding_allowed = true;
                    }
                }
                else{
                    adding_allowed = true;
                }

                if(adding_allowed){
                    auto& added_node = CreateRGNode(pkg);
                    auto temp_mouse_pos =ImGui::GetIO().MousePos;
                    added_node.pos = temp_mouse_pos;// - ImNodes::EditorContextGetPanning();// - (temp_min - window_pos);
                }
            }
        }
    }   

    ImNodes::Node& SubmissionTask_NG::CreateRGNode(Command::PackageRecord* record) {
        auto& added_node = AddNode();
        added_node.payload = new TaskMetaPayload{record};
        added_node.pos = menu_pos;
        added_node.pins.emplace_back(ImNodes::Pin{.local_pos{0.f, 0.5f}, .payload{nullptr}});
        added_node.pins.emplace_back(ImNodes::Pin{.local_pos{1.f, 0.5f}, .payload{nullptr}});

        return added_node;
    }
    ImNodes::Node& SubmissionTask_NG::CreateRGNode(GPUTask* task) {
        auto& added_node = AddNode();
        added_node.payload = new TaskMetaPayload{task};
        added_node.pos = menu_pos;
        added_node.pins.emplace_back(ImNodes::Pin{.local_pos{0.f, 0.5f}, .payload{nullptr}});
        added_node.pins.emplace_back(ImNodes::Pin{.local_pos{1.f, 0.5f}, .payload{nullptr}});

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

    void SubmissionTask_NG::LinkEmptyDrop(ImNodes::Node& src_node, ImNodes::PinOffset pin_offset) {

        menu_pos = ImGui::GetMousePos();
        add_menu_is_open = true;
    }

    void SubmissionTask_NG::LinkCreated(ImNodes::NodePair& link) {
        //Log::Debug("link created\n");
        link.start.node->pins[link.start.offset].payload = link.end.node;
        link.end.node->pins[link.end.offset].payload = link.start.node;
    }

    void SubmissionTask_NG::LinkDestroyed(ImNodes::NodePair& link) {
        //Log::Debug("link destroyed\n");

        link.start.node->pins[link.start.offset].payload = nullptr;
        link.end.node->pins[link.end.offset].payload = nullptr;
        ImNodes::Editor::LinkDestroyed(link);
    }

    void SubmissionTask_NG::RenderNode(ImNodes::Node& node){
        ImNodes::BeginNodeTitleBar();

        //if node == &headNode
        if(node.payload == nullptr){
            EWE_ASSERT(headNode == &node);
            //ImGui::InputText("name of package record");
            ImGui::Text("head node");
            ImNodes::EndNodeTitleBar();
            ImGui::SetNextItemWidth(150.f);
            if(ImGui::InputText("name", explorer.file_save_buf, explorer.path_length, ImGuiInputTextFlags_CallbackCharFilter, ImguiInputFilters::File)){
                //name = name_buffer;
            }
            return;
        }
        auto* task_payload = reinterpret_cast<TaskMetaPayload*>(node.payload);
        auto* task = task_payload->task;
        ImGui::Text(task->name.c_str());

        ImNodes::EndNodeTitleBar();
        if(ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && ImGui::IsWindowHovered()){
            //open the graph for the package
            OpenGraph(Type::PackageRecord, task->pkgRecord);
        }

        if(task->pkgRecord->packages.size() == 0){
            ImGui::Text("no packages");
            return;
        }
        std::size_t raster_index = 0;
        for(auto* pkg : task->pkgRecord->packages){
            if(pkg->type == Command::InstructionPackage::Type::Raster){
                auto& raster_pkg = *reinterpret_cast<RasterPackage*>(pkg);
                const std::string raster_tree_name = std::string("raster pkg attachments : ") + raster_pkg.name.string();
                const bool raster_tree_open = ImGui::TreeNode(raster_tree_name.c_str());
                if(task_payload->raster_open[raster_index] != raster_tree_open){
                    ReadjustAttachmentPins(node, raster_index, raster_tree_open);
                    task_payload->raster_open[raster_index] = raster_tree_open;
                }
                if(raster_tree_open){
                    for(std::size_t a_i = 0; a_i < raster_pkg.attachmentMeta.Size(); a_i++){
                        if(a_i < raster_pkg.task_config.attachment_info.colors.Size()){
                            ImGui::Text("color[%zu]", a_i);
                        }
                        else{
                            ImGui::Text("depth");
                        }
                        
                    }

                    ImGui::TreePop();
                }
                raster_index++;
            }
        }

        GPUTaskMeta_Helper& meta_helper = task_payload->meta_helper;
        for(auto& push : meta_helper.pushes){
            auto& record = *push.pointer_chain.base;
            EWE_ASSERT(push.pointer_chain.base == task_payload->task->pkgRecord);
            auto& raster_pkg = *reinterpret_cast<RasterPackage*>(record.packages[push.pointer_chain.package_iter]);
            auto& obj_pkg = *raster_pkg.objectPackages[push.pointer_chain.pointer_into[0]];
            /* 
            does this even matter? what am i going to use it for here?
            
            then the last pointer is the instruciton index? or param offset?
            PerFlight<ParamPack<Inst::Push>*> push_packs;

            //memory is std::byte*
            push_packs[0] = reinterpret_cast<ParamPack<Inst::Push>*>(obj_pkg.paramPool.params[0].memory + push.pointer_chain.pointer_into[1]);
            push_packs[1] = reinterpret_cast<ParamPack<Inst::Push>*>(obj_pkg.paramPool.params[1].memory + push.pointer_chain.pointer_into[1]);
            */
            ImGui::Text("toggle to expose for barrier");
            const std::string tree_obj_name = "object : " + obj_pkg.name.string();
            if(ImGui::TreeNode(tree_obj_name.c_str())){
                if(ImGui::Button("open object")){
                    OpenGraph(Type::ObjectPackage, &obj_pkg);
                }
                const std::string raster_button_name = std::string("raster pkg") + raster_pkg.name.string();
                if(ImGui::Button(raster_button_name.c_str())){
                    OpenGraph(Type::RasterPackage, &raster_pkg);
                }
                const std::string record_button_name = std::string("record") + record.name.string();
                if(ImGui::Button(record_button_name.c_str())){
                    OpenGraph(Type::PackageRecord, &record);
                }

                if(ImGui::BeginTable("push expose", 2, ImGuiTableFlags_Borders)){
                    ImGui::TableSetupColumn("buffer");
                    ImGui::TableSetupColumn("texture");
                    ImGui::TableHeadersRow();

                    const std::size_t max_iter = lab::Max(push.buffer_active.size(), push.texture_active.size());

                    bool buffer_bool;
                    for(std::size_t resource_iter = 0; resource_iter < max_iter; resource_iter++){
                        ImGui::TableNextColumn();
                        if(resource_iter < push.buffer_active.size()){
                            buffer_bool = push.buffer_active[resource_iter];
                            if(ImGui::Checkbox(push.push.buffers[resource_iter].name.c_str(), &buffer_bool)){
                                push.buffer_active[resource_iter] = buffer_bool;
                                meta_helper.ToggleResource(push.buffer_active[resource_iter], push.pointer_chain, resource_iter);
                            }
                        }
                        
                        ImGui::TableNextColumn();
                        if(resource_iter < push.texture_active.size()){
                            buffer_bool = push.texture_active[resource_iter];
                            if(ImGui::Checkbox(push.push.textures[resource_iter].name.c_str(), &buffer_bool)){
                                push.texture_active[resource_iter] = buffer_bool;
                                meta_helper.ToggleResource(push.texture_active[resource_iter], push.pointer_chain, resource_iter + push.buffer_active.size());
                            }
                        }
                    }

                    ImGui::EndTable();
                }
                ImGui::TreePop();
            }
        }
    }

    void SubmissionTask_NG::RenderPin(ImNodes::Node& node, ImNodes::PinOffset pin_index) {
        auto& pin = node.pins[pin_index];

        if (ImNodes::BeginPinAttribute(node.id + pin_index + 1, pin.local_pos)) {
            //ImGui::Text("pin");
        }
        ImNodes::EndPinAttribute();
    }

    std::vector<GPUTask*> SubmissionTask_NG::CollectTasks(){
        std::vector<GPUTask*> ret{};
        
        auto GetTaskFromNode = [](ImNodes::Node* node) -> GPUTask*{
            return reinterpret_cast<TaskMetaPayload*>(node->payload)->task;
        };

        ImNodes::Node* current_node = nullptr;
        //the pin payload is going to be a pointer to the node, unless nullptr
        if(headNode->pins[0].payload != nullptr){
            current_node = reinterpret_cast<ImNodes::Node*>(headNode->pins[0].payload);

            ret.push_back(GetTaskFromNode(current_node));
            if(current_node == nullptr){
                return ret;
            }

            while(current_node->pins[1].payload != nullptr){

                current_node = reinterpret_cast<ImNodes::Node*>(current_node->pins[1].payload);
                auto current_task = GetTaskFromNode(current_node);
                ret.push_back(current_task);
            }
        }
        return ret;
    }

    bool SubmissionTask_NG::SaveFunc() {
        explorer.enabled = save_open;
        explorer.state = ExplorerContext::State::Save;
        if(ImGui::Begin("file save")){
            explorer.Imgui();
            if(explorer.selected_file.has_value()){
                const std::filesystem::path saved_path = *explorer.selected_file;

                const auto temp_path = std::filesystem::proximate(saved_path, Global::assetManager->subTask.files.root_directory);
                name = temp_path;

                SubmissionTask& written = Global::assetManager->subTask.ConstructInto(name, engine->logicalDevice, engine->renderQueue);
                
                auto collected_tasks = CollectTasks();
                
                for(auto& task : collected_tasks){
                    written.tasks.push_back(task);
                }

                Asset::WriteAssetToFile(written, Global::assetManager->subTask.files.root_directory, temp_path);

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

                const std::filesystem::path temp = std::filesystem::proximate(load_path, Global::assetManager->subTask.files.root_directory);

                auto* subTask = Global::assetManager->subTask.Get(temp);
                InitFromObject(*subTask);

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

    void SubmissionTask_NG::PopulateFromGraph(SubmissionTask& subTask){
        subTask.tasks.clear();
        ImNodes::Node* current_node = reinterpret_cast<ImNodes::Node*>(headNode->pins[0].payload);
        while(current_node != nullptr){
            subTask.tasks.push_back(reinterpret_cast<GPUTask*>(current_node->payload));
            current_node = reinterpret_cast<ImNodes::Node*>(current_node->pins[1].payload);
        }
        subTask.CollectTaskWorkloads();
    }

    void SubmissionTask_NG::InitFromObject(SubmissionTask& subTask){
        nodes.Clear();
        CreateHeadNode();
        links.clear();
        name = subTask.name;
        const std::string string_name = subTask.name.string();
        if(string_name.size() > ExplorerContext::path_length) {
            memcpy(explorer.file_save_buf, string_name.substr(0, ExplorerContext::path_length - 1).c_str(), ExplorerContext::path_length);
        }
        else{
            memcpy(explorer.file_save_buf, string_name.c_str(), string_name.size() + 1);
        }
        ImNodes::Node* last_node = headNode;

        if(subTask.tasks.size() == 0){
            return;
        }

        if(subTask.queue != nullptr){
            current_queue_type = engine->GetQueueType(*subTask.queue);
        }

        {
            auto& node = CreateRGNode(subTask.tasks.front());
            headNode->pins[0].payload = &node;
            links.push_back(
                ImNodes::NodePair{
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
            for(auto* pkg : subTask.tasks.front()->pkgRecord->packages){
                if(pkg->type == Command::InstructionPackage::Type::Raster){
                    auto* raster_pkg = reinterpret_cast<RasterPackage*>(pkg);
                    //handle images
                }
            }
        }

        for(std::size_t i = 1; i < subTask.tasks.size(); i++){
            auto& node = CreateRGNode(subTask.tasks[i]);

            links.push_back(
                ImNodes::NodePair{
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