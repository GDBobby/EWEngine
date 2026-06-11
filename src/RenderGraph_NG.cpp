#include "EWEngine/Imgui/ImNodes/Graph/RenderGraph_NG.h"

#include "EWEngine/EWEngine.h"
#include "EWEngine/Imgui/DragDrop.h"
#include "EWEngine/Imgui/ImNodes/imnodes.h"
#include "EightWinds/Backend/RenderInfo.h"
#include "EightWinds/Command/InstructionPackage.h"
#include "EightWinds/RenderGraph/SubmissionTask.h"

#include "EWEngine/Imgui/Objects.h"
#include "imgui.h"

#include "EWEngine/Imgui/ImNodes/imnodes_internal.h"

namespace EWE{
namespace Node{
    RenderGraph_NG::RenderGraph_NG()
        : ImNodes::Editor{},
        explorer{std::filesystem::current_path()},
        headNode{CreateHeadNode()}
    {
        name = "render graph ng";
    }

    ImNodes::Node* RenderGraph_NG::CreateHeadNode(){
        auto& head = AddNode();
        head.snapToGrid = true;

        head.pos = node_editor_window_pos;
        head.pins.emplace_back(ImNodes::Pin{.local_pos{1.f, 0.5f}, .payload{nullptr}});
        return &head;
    } 

    void RenderGraph_NG::RenderNodes() {
        ImNodes::Editor::RenderNodes();
        SubmissionTask* subTask;
        if(DragDropPtr::Target(subTask)) {
            auto& added_node = CreateRGNode(subTask);
            auto temp_mouse_pos =ImGui::GetIO().MousePos;
            added_node.pos = temp_mouse_pos;// - ImNodes::EditorContextGetPanning();// - (temp_min - window_pos);
        }
    }

    ImNodes::Node& RenderGraph_NG::CreateRGNode(FullRenderInfo* renderInfo){
        auto& added_node = AddNode();
        added_node.payload = new NodePayload{
            .type = NodeType::RenderInfo,
            .payload = renderInfo
        };
        added_node.pos = menu_pos;
        added_node.pins.emplace_back(ImNodes::Pin{.local_pos{0.f, 0.5f}, .payload{nullptr}});
        added_node.pins.emplace_back(ImNodes::Pin{.local_pos{1.f, 0.5f}, .payload{nullptr}});

        return added_node;
    }
    ImNodes::Node& RenderGraph_NG::CreateRGNode(SubmissionTask* subTask) {
        auto& added_node = AddNode();
        auto added_payload = new SubTaskPayload{
            .type = NodeType::TaskGroup,
            .sub_group = new std::vector<SubmissionTask*>{subTask}
        };
        added_payload->raster_pkg_open.resize(1);
        std::vector<std::vector<bool>>& back_task_group_raster_pkgs = added_payload->raster_pkg_open.back();
        back_task_group_raster_pkgs.resize(subTask->tasks.size());
        added_payload->task_meta_helpers.resize(1);

        for(std::size_t task_index = 0; task_index < subTask->tasks.size(); task_index++){
            std::size_t raster_index = 0;
            for(auto& pkg : subTask->tasks[task_index]->pkgRecord->packages){
                if(pkg->type == Command::InstructionPackage::Type::Raster){
                    raster_index++;
                }
            }

            back_task_group_raster_pkgs[task_index].resize(raster_index, false);
            added_payload->task_meta_helpers.back().emplace_back(*subTask->tasks[task_index], GPUTaskMeta_Helper::HelperType::RenderGraph);
        }

        added_node.payload = added_payload;

        added_node.pos = menu_pos;
        added_node.pos = menu_pos;
        added_node.pins.emplace_back(ImNodes::Pin{.local_pos{0.f, 0.5f}, .payload{nullptr}});
        added_node.pins.emplace_back(ImNodes::Pin{.local_pos{1.f, 0.5f}, .payload{nullptr}});

        return added_node;
    }

    void RenderGraph_NG::RenderEditorTitle() {

        ImNodes::Editor::RenderEditorTitle();

        /*
        if(ImGui::Button("set from current")){
            for(auto iter = nodes.begin(); iter != nodes.end(); ++iter){
                nodes.DestroyElement(iter);
            }
            links.clear();

            float horizontalPos = node_editor_window_pos.x;
            for(auto& sub_group : renderGraph->execution_order){
                float verticalPos = node_editor_window_pos.y;

                for(auto& ind_sub : sub_group){
                    auto& added_node = AddNode();
                    added_node.payload = ind_sub;
                    added_node.pos.x = horizontalPos;
                    added_node.pos.y = verticalPos;
                    verticalPos += 60.f;

                    float pin_starting = 0.3f;
                    for(auto& wait : ind_sub->submitInfo[0].waitSemaphores){
                        //currently a mem leak
                        added_node.pins.emplace_back(ImNodes::Pin{.local_pos{0.f, pin_starting}, .payload = wait});
                        pin_starting += 0.1f;
                    }
                    pin_starting = 0.3f;
                    for(auto& signal : ind_sub->submitInfo[0].signalSemaphores){
                        //currently a mem leak
                        added_node.pins.emplace_back(ImNodes::Pin{.local_pos{1.f, pin_starting}, .payload = &signal});
                        pin_starting += 0.1f;
                    }
                }
                horizontalPos += 200.f;
            }
        }
        */
        
    }

    bool RenderGraph_NG::RenderAddMenu(){

        bool wantsClose = false;

        ImGui::SetNextWindowPos(menu_pos);
        if(ImGui::Begin("add menu")){
            
            //if(ImGui::Button("add render info")){
                //CreateRGNode(new FullRenderInfo("unnamed", engine->logicalDevice, engine->renderQueue, AttachmentSetInfo{}));
                //wantsClose = true;
            //}

            //ImGui::Text("%d", ImGui::IsWindowHovered());
            /*
            for(auto& submission : renderGraph->submissions){
                if(ImGui::Button(submission.name.c_str())){
                    CreateRGNode(&submission);
                    wantsClose = true;
                }
            }
            */
            if(!ImGui::IsWindowFocused()){
                wantsClose = true;
            }

        }
        ImGui::End();
        /*
            in the render graph node graph, submissions are added
            a submission is most likely going to be a record. there is also the imgui submission
            there will potentially be other specialized submissions like the imgui submission
        */
        return wantsClose;
    }

    void RenderGraph_NG::RenderNode(ImNodes::Node& node) {

        ImNodes::ImNodeData& imnodes_node = context->Nodes.Pool[ImNodes::GetCurrentContext()->CurrentNodeIdx];
        if(&node == headNode){

            ImNodes::BeginNodeTitleBar();
            ImGui::Text("head node - {%.2f:%.2f} : {%.2f:%.2f}", imnodes_node.Rect.Min.x, imnodes_node.Rect.Min.y, imnodes_node.Rect.Max.x, imnodes_node.Rect.Max.y);
            ImGui::Text("head");
            ImNodes::EndNodeTitleBar();
            ImGui::Text("filler");

            return;
        }

        auto* payload = reinterpret_cast<NodePayload*>(node.payload);

        ImNodes::BeginNodeTitleBar();
        if(payload->type == NodeType::TaskGroup){
            ImGui::Text("task group");// - {%.2f:%.2f} : {%.2f:%.2f}", imnodes_node.Rect.Min.x, imnodes_node.Rect.Min.y, imnodes_node.Rect.Max.x, imnodes_node.Rect.Max.y);
        }
        else{
            ImGui::Text("render info");
        }
        ImNodes::EndNodeTitleBar();

        uint16_t current_pin = 0;

        if(payload->type == NodeType::TaskGroup){
            auto* sub_task_payload = reinterpret_cast<SubTaskPayload*>(node.payload);
            auto* sub_group = sub_task_payload->sub_group;

            for(std::size_t sub_task_index = 0; sub_task_index < sub_group->size(); sub_task_index++){
                auto* subTask = sub_group->at(sub_task_index);
                bool sub_task_tree_open = ImGui::TreeNode(subTask->name.string().c_str());
                if(ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)){

                }
                if(sub_task_tree_open){
                    if(!subTask->specializedSubmission){
                    //expose the render infos
                        for(std::size_t task_index = 0; task_index < subTask->tasks.size(); task_index++){
                            auto* task = subTask->tasks[task_index];

                            bool task_meta_node = ImGui::TreeNode(task->name.string().c_str());

                            if(task_meta_node){
                                std::size_t raster_index = 0;
                                for(auto* pkg : task->pkgRecord->packages){
                                    if(pkg->type == Command::InstructionPackage::Type::Raster){
                                        auto& raster_pkg = *reinterpret_cast<RasterPackage*>(pkg);
                                        const std::string raster_tree_name = std::string("raster pkg attachments : ") + raster_pkg.name.string();
                                        const bool raster_tree_open = ImGui::TreeNode(raster_tree_name.c_str());
                                        if(sub_task_payload->raster_pkg_open[sub_task_index][task_index][raster_index] != raster_tree_open){
                                            //ReadjustAttachmentPins(node, raster_index, raster_tree_open);
                                            sub_task_payload->raster_pkg_open[sub_task_index][task_index][raster_index] = raster_tree_open;
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

                                for(auto& push_helper : sub_task_payload->task_meta_helpers[sub_task_index][task_index].pushes){
                                    auto& m_rp = push_helper.pointer_chain;
                                    Command::PackageRecord& pkgRecord = *m_rp.base;
                                    RasterPackage& rasterPkg = *reinterpret_cast<RasterPackage*>(pkgRecord.packages[m_rp.package_iter]);
                                    auto& obj_pkg = *rasterPkg.objectPackages[m_rp.pointer_into[0]];
                                    //std::size_t instruction_offset = m_rp.pointer_into[1];
                                    std::size_t resource_index = m_rp.pointer_into[2];

                                    PushConstant& push = push_helper.push;
                                    if(resource_index < push.buffers.size()){
                                        ImGui::Button(push.buffers[resource_index].name.c_str());
                                    }
                                    else{
                                        resource_index -= push.buffers.size();
                                        ImGui::Button(push.textures[resource_index].name.c_str());
                                    }
                                }

                                ImGui::TreePop();
                            }
                        }
                    }

                    ImGui::TreePop();
                }
            }
        }
        else{
            ImguiExtension::Imgui(reinterpret_cast<FullRenderInfo*>(payload->payload)->full);
        }
    }

    void RenderGraph_NG::RenderPin(ImNodes::Node& node, ImNodes::PinOffset pin_index) {
        auto& pin = node.pins[pin_index];

        if (ImNodes::BeginPinAttribute(node.id + pin_index + 1, pin.local_pos)) {
            //ImGui::TextColored(ImVec4(1.f, 0.f, 0.f, 1.f), "%zu", payload->stageMask);
        }
        else {
            //ImGui::Text("%zu", static_cast<uint64_t>(payload->stageMask));
        }
        ImNodes::EndPinAttribute();
    }
    bool RenderGraph_NG::SaveFunc() {
        explorer.enabled = save_open;
        explorer.state = ExplorerContext::State::Save;
        if(ImGui::Begin("file save")){
            explorer.Imgui();
            if(explorer.selected_file.has_value()){
                const std::filesystem::path saved_path = *explorer.selected_file;

                const auto temp_path = std::filesystem::proximate(saved_path, Global::assetManager->subTask.files.root_directory);
                name = temp_path;

                Log::Error("saving rendergraph isn't setup yet\n");
                //RenderGraph& written = Global::assetManager->renderGraph.ConstructInto(name, engine->logicalDevice);
                
                //auto collected_tasks = CollectTasks();
                
                //for(auto& task : collected_tasks){
                //    written.tasks.push_back(task);
                //}

                //Asset::WriteAssetToFile(written, Global::assetManager->renderGraph.files.root_directory, temp_path);

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

    bool RenderGraph_NG::LoadFunc() {
        explorer.enabled = load_open;
        explorer.state = ExplorerContext::State::Load;
        if(ImGui::Begin("file load")){
            explorer.Imgui();
            if(explorer.selected_file.has_value()){
                const std::filesystem::path load_path = *explorer.selected_file;

                const std::filesystem::path temp = std::filesystem::proximate(load_path, Global::assetManager->subTask.files.root_directory);

                auto* graph = Global::assetManager->renderGraph.Get(temp);
                InitFromObject(*graph);

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


    void RenderGraph_NG::InitFromObject(RenderGraph& _renderGraph){

    }

} //namespace Node
} //namespace EWE