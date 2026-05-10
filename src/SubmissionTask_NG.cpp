#include "EWEngine/Imgui/ImNodes/Graph/SubmissionTask_NG.h"

#include "EWEngine/Assets/GPUTasks.h"
#include "EWEngine/Assets/Hash.h"
#include "EWEngine/Imgui/DragDrop.h"
#include "EWEngine/Global.h"
//#include "EWEngine/Imgui/ImNodes/Graph/PackageRecord_NG.h"
#include "EightWinds/Backend/RenderInfo.h"
#include "EightWinds/Command/InstructionPackage.h"
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
        explorer.acceptable_extensions = Global::assetManager->subTask.files.acceptable_extensions;
        renderInfo = nullptr;
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
        {
            Command::PackageRecord* pkg;
            if(DragDropPtr::Target(pkg)) {
                Logger::Print("dropping in sub task\n");


                auto const task_queue_type = Global::stcManager->GetQueueType(*pkg->queue);
                bool adding_allowed = false;
                if(current_queue_type != task_queue_type){
                    if(headNode->pins[0].payload == nullptr){
                        current_queue_type = task_queue_type;
                        adding_allowed = true;
                        if(current_queue_type == Queue::Type::Graphics){

                            AttachmentSetInfo defaultInfo{};

                            renderInfo = new FullRenderInfo(
                                "default render info", 
                                *Global::logicalDevice, *pkg->queue,
                                defaultInfo
                            );
                        }
                        else if (renderInfo != nullptr){
                            delete renderInfo;
                        }
                    }
                }
                else{
                    adding_allowed = true;
                }

                if(adding_allowed){
                    GPUTask* task = new GPUTask(pkg->name, *Global::logicalDevice, *pkg, false);
                    auto& added_node = CreateRGNode(task);
                    auto temp_mouse_pos =ImGui::GetIO().MousePos;
                    added_node.pos = temp_mouse_pos;// - ImNodes::EditorContextGetPanning();// - (temp_min - window_pos);
                }
            }
        }
    }   

    ImNodes::EWE::Node& SubmissionTask_NG::CreateRGNode(GPUTask* task) {
        auto& added_node = AddNode();
        added_node.payload = new NodePayload{
            .type = NodePayload::Type::Task,
            .payload = task
        };
        added_node.pos = menu_pos;
        added_node.pins.emplace_back(ImNodes::EWE::Pin{.local_pos{0.f, 0.5f}, .payload{nullptr}});
        added_node.pins.emplace_back(ImNodes::EWE::Pin{.local_pos{1.f, 0.5f}, .payload{nullptr}});

        added_node.pins.emplace_back(ImNodes::EWE::Pin{.local_pos{0.9f, 0.1f}, .payload{nullptr}});
        /*
        for(auto* pkg : task->pkgRecord->packages){
            if(pkg->type == Command::InstructionPackage::Raster){
                added_node.pins.emplace_back(ImNodes::EWE::Pin{.local_pos{1.f, 0.5f}, .payload{nullptr}});
                break;
            }
        }
        */
        

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
            ImGui::SetNextItemWidth(150.f);
            if(ImGui::InputText("name", explorer.file_save_buf, explorer.path_length, ImGuiInputTextFlags_CallbackCharFilter, ImguiInputFilters::File)){
                //name = name_buffer;
            }
            
            if(current_queue_type == Queue::Type::Graphics){
                EWE_ASSERT(renderInfo != nullptr);
                ImguiExtension::Imgui(renderInfo->full.setInfo);
                ImGui::Separator();

                if(ImGui::BeginTable("images", 2, ImGuiTableFlags_Borders)){
                    ImGui::TableSetupColumn("frame[0]");
                    ImGui::TableSetupColumn("frame[1]");
                    ImGui::TableHeadersRow();
                    
                    auto image_color_cell = [&](ImageView*& view, uint8_t index){
                        ImGui::TableNextColumn();
                        if(view == nullptr){
                            const std::string color_name = "color[" + std::to_string(index) + " : null";
                            ImGui::Button(color_name.c_str());
                        }
                        else{
                            const std::string color_name = "color["+ std::to_string(index) + "] : " + view->image.name.string();
                            ImGui::Button(color_name.c_str());
                        }
                        ImageView* dd_ptr = nullptr;
                        if(DragDropPtr::Target(dd_ptr)){
                            view = dd_ptr;
                        }
                    };
                    auto img_depth_cell = [&](ImageView*& view, uint8_t frame){
                        ImGui::TableNextColumn();
                        if(view == nullptr){
                            const std::string img_name = "depth : null##" + std::to_string(frame);
                            ImGui::Button(img_name.c_str());
                        }
                        else{
                            const std::string img_name = "depth : " + view->image.name.string();
                            ImGui::Button(img_name.c_str());
                        }
                        ImageView* dd_ptr = nullptr;
                        if(DragDropPtr::Target(dd_ptr)){
                            view = dd_ptr;
                        }
                    };

                    for(std::size_t i = 0; i < renderInfo->full.color_views.Size(); i++){
                        image_color_cell(renderInfo->full.color_views[i][0], i);
                        image_color_cell(renderInfo->full.color_views[i][1], i);
                    }
                    if(renderInfo->full.setInfo.using_depth){
                        img_depth_cell(renderInfo->full.depth_views[0], 0);
                        img_depth_cell(renderInfo->full.depth_views[1], 1);
                    }
                    ImGui::EndTable();
                }
                if(ImGui::TreeNode("attachment set info for generation")){
                    
                    ImguiExtension::Imgui(generate_attachment_info);

                    if(ImGui::Button("generate color")){

                        std::vector<AttachmentInfo> old_info{renderInfo->full.setInfo.colors.Size()};
                        memcpy(old_info.data(), renderInfo->full.setInfo.colors.Data(), old_info.size() * sizeof(AttachmentInfo));
                        renderInfo->full.setInfo.colors.ClearAndResize(old_info.size() + 1);
                        memcpy(renderInfo->full.setInfo.colors.Data(), old_info.data(), old_info.size() * sizeof(AttachmentInfo));
                        renderInfo->full.setInfo.colors[old_info.size()] = generate_attachment_info;

                        PerFlight<Image*> img_con_ptr{};
                        img_con_ptr[0] = Global::assetManager->image.data_arena.GetCell();
                        img_con_ptr[1] = Global::assetManager->image.data_arena.GetCell();
                        PerFlight<ImageView*> view_con_ptr{};
                        view_con_ptr[0] = Global::assetManager->imageView.data_arena.GetCell();
                        view_con_ptr[1] = Global::assetManager->imageView.data_arena.GetCell();

                        std::vector<PerFlight<ImageView*>> old_views{renderInfo->full.color_views.Size()};
                        for(std::size_t i = 0; i < old_views.size(); i++){
                            for_each_frame{
                                old_views[i][frame] = renderInfo->full.color_views[i][frame];
                            }
                        }
                        renderInfo->full.color_views.ClearAndResize(old_views.size() + 1);
                        renderInfo->full.GenerateImage(
                            img_con_ptr, view_con_ptr,
                            Global::window->screenDimensions.width, Global::window->screenDimensions.height, 
                            old_views.size()
                        );
                        for(std::size_t i = 0; i < old_views.size(); i++){
                            for_each_frame{
                                renderInfo->full.color_views[i][frame] = old_views[i][frame];
                            }
                        }
                    }
                    if(ImGui::Button("generate depth")){

                        renderInfo->full.setInfo.using_depth = true;
                        renderInfo->full.setInfo.depth = generate_attachment_info;
                        PerFlight<Image*> img_con_ptr{};
                        img_con_ptr[0] = Global::assetManager->image.data_arena.GetCell();
                        img_con_ptr[1] = Global::assetManager->image.data_arena.GetCell();
                        PerFlight<ImageView*> view_con_ptr{};
                        view_con_ptr[0] = Global::assetManager->imageView.data_arena.GetCell();
                        view_con_ptr[1] = Global::assetManager->imageView.data_arena.GetCell();
                        renderInfo->full.GenerateImage(
                            img_con_ptr, view_con_ptr,
                            Global::window->screenDimensions.width, Global::window->screenDimensions.height, 
                            -1
                        );
                    }

                    ImGui::TreePop();
                }


            }
            return;
        }
        auto* node_payload = reinterpret_cast<NodePayload*>(node.payload);
        if(node_payload->type == NodePayload::Type::Task){
            auto* task_payload = reinterpret_cast<GPUTask*>(node_payload->payload);
            ImGui::Text(task_payload->name.c_str());

            ImNodes::EndNodeTitleBar();
            if(ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && ImGui::IsWindowHovered()){
                //open the graph for the package
                OpenGraph(Type::PackageRecord, task_payload->pkgRecord);
            }

            if(task_payload->pkgRecord->packages.size() == 0){
                ImGui::Text("no packages");
            }

            for(auto& pkg : task_payload->pkgRecord->packages){
                if(pkg->type == Command::InstructionPackage::Raster){
                    const bool tree_open = ImGui::TreeNode(pkg->name.c_str());
                    if(tree_open != node_payload->current_open_status){

                    }
                    if(tree_open){
                        auto* raster_pkg = reinterpret_cast<RasterPackage*>(pkg);

                        for(auto& obj_pkg : raster_pkg->objectPackages){
                            if(ImGui::TreeNode(obj_pkg->name.string().c_str())){
                                for(auto& inst : obj_pkg->paramPool.instructions){
                                    ImGui::BulletText(Reflect::Enum::ToString(inst).data());
                                }
                                ImGui::TreePop();
                            }
                        }
                        ImGui::TreePop();
                    }
                    /*
                    for(auto& obj_pkg : pkg->packages){
                        for(std::size_t inst_index = 0; inst_index < obj_pkg->paramPool.instructions.size(); inst_index++){
                            if(obj_pkg->paramPool.instructions[inst_index] == Inst::Push){
                                auto const param_index = obj_pkg->paramPool.GetParamOffset(inst_index);
                                auto* push_data = obj_pkg->paramPool.param_data[param_index].CastTo<Inst::Push>();
                            }
                        }
                    }
                    */
                }
                else{
                    ImGui::BulletText("%s : %s", Reflect::Enum::ToString(pkg->type).data(), pkg->name.c_str());
                }
            }
        }
        else{
            auto* ri_payload = reinterpret_cast<FullRenderInfo*>(node_payload->payload);
            ImGui::Text(ri_payload->name.string().c_str());

            ImNodes::EndNodeTitleBar();

            ImguiExtension::Imgui(*ri_payload);
            ImGui::Text("filler");
        }
    }

    void SubmissionTask_NG::RenderPin(ImNodes::EWE::Node& node, ImNodes::EWE::PinOffset pin_index) {
        auto& pin = node.pins[pin_index];

        if (ImNodes::BeginPinAttribute(node.id + pin_index + 1, pin.local_pos)) {
            //ImGui::Text("pin");
        }
        ImNodes::EndPinAttribute();
    }

    std::vector<GPUTask*> SubmissionTask_NG::CollectTasks(){
        std::vector<GPUTask*> ret{};
        
        auto GetTaskFromNode = [](ImNodes::EWE::Node* node) -> GPUTask*{
            NodePayload* node_payload = reinterpret_cast<NodePayload*>(node->payload);
            EWE_ASSERT(node_payload->type == NodePayload::Type::Task);
            GPUTask* task = reinterpret_cast<GPUTask*>(node_payload->payload);
            return task;
        };

        ImNodes::EWE::Node* current_node = nullptr;
        //the pin payload is going to be a pointer to the node, unless nullptr
        if(headNode->pins[0].payload != nullptr){
            current_node = reinterpret_cast<ImNodes::EWE::Node*>(headNode->pins[0].payload);

            ret.push_back(GetTaskFromNode(current_node));
            if(current_node == nullptr){
                return ret;
            }

            while(current_node->pins[1].payload != nullptr){

                current_node = reinterpret_cast<ImNodes::EWE::Node*>(current_node->pins[1].payload);
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

                SubmissionTask& written = Global::assetManager->subTask.ConstructInto(name, *Global::logicalDevice, Global::stcManager->renderQueue);
                
                auto collected_tasks = CollectTasks();
                
                for(auto& task : collected_tasks){
                    written.tasks.push_back(task);
                }
                written.renderInfo = renderInfo;

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
        ImNodes::EWE::Node* current_node = reinterpret_cast<ImNodes::EWE::Node*>(headNode->pins[0].payload);
        while(current_node != nullptr){
            subTask.tasks.push_back(reinterpret_cast<GPUTask*>(current_node->payload));
            current_node = reinterpret_cast<ImNodes::EWE::Node*>(current_node->pins[1].payload);
        }
        subTask.CollectTaskWorkloads();
    }

    void SubmissionTask_NG::InitFromObject(SubmissionTask& subTask){
        nodes.Clear();
        CreateHeadNode();
        links.clear();
        ImNodes::EWE::Node* last_node = headNode;

        if(subTask.tasks.size() == 0){
            return;
        }

        if(subTask.queue != nullptr){
            current_queue_type = Global::stcManager->GetQueueType(*subTask.queue);
            if(current_queue_type == Queue::Type::Graphics){
                renderInfo = subTask.renderInfo;
            }
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