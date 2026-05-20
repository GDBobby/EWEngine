#include "EWEngine/Imgui/ImNodes/Graph/RenderGraph_NG.h"

#include "EWEngine/Global.h"
#include "EWEngine/Imgui/DragDrop.h"
#include "EWEngine/Imgui/ImNodes/imnodes.h"
#include "EightWinds/Backend/RenderInfo.h"
#include "EightWinds/RenderGraph/SubmissionTask.h"

#include "EWEngine/Imgui/Objects.h"
#include "imgui.h"

#include "EWEngine/Imgui/ImNodes/imnodes_internal.h"

namespace EWE{
namespace Node{
    RenderGraph_NG::RenderGraph_NG()
        : ImNodes::EWE::Editor{},
        explorer{std::filesystem::current_path()},
        headNode{CreateHeadNode()}
    {
        name = "render graph ng";
    }

    ImNodes::EWE::Node* RenderGraph_NG::CreateHeadNode(){
        auto& head = AddNode();
        head.snapToGrid = true;

        head.pos = node_editor_window_pos;
        head.pins.emplace_back(ImNodes::EWE::Pin{.local_pos{1.f, 0.5f}, .payload{nullptr}});
        return &head;
    } 

    void RenderGraph_NG::RenderNodes() {
        ImNodes::EWE::Editor::RenderNodes();
        SubmissionTask* subTask;
        if(DragDropPtr::Target(subTask)) {
            auto& added_node = CreateRGNode(subTask);
            auto temp_mouse_pos =ImGui::GetIO().MousePos;
            added_node.pos = temp_mouse_pos;// - ImNodes::EditorContextGetPanning();// - (temp_min - window_pos);
        }
    }

    ImNodes::EWE::Node& RenderGraph_NG::CreateRGNode(FullRenderInfo* renderInfo){
        auto& added_node = AddNode();
        added_node.payload = new NodePayload{
            .type = NodeType::RenderInfo,
            .payload = renderInfo
        };
        added_node.pos = menu_pos;
        added_node.pins.emplace_back(ImNodes::EWE::Pin{.local_pos{0.f, 0.5f}, .payload{nullptr}});
        added_node.pins.emplace_back(ImNodes::EWE::Pin{.local_pos{1.f, 0.5f}, .payload{nullptr}});

        return added_node;
    }
    ImNodes::EWE::Node& RenderGraph_NG::CreateRGNode(SubmissionTask* subTask) {
        auto& added_node = AddNode();
        added_node.payload = new NodePayload{
            .type = NodeType::TaskGroup,
            .payload = new std::vector<SubmissionTask*>{subTask}
        };
        added_node.pos = menu_pos;
        added_node.pos = menu_pos;
        added_node.pins.emplace_back(ImNodes::EWE::Pin{.local_pos{0.f, 0.5f}, .payload{nullptr}});
        added_node.pins.emplace_back(ImNodes::EWE::Pin{.local_pos{1.f, 0.5f}, .payload{nullptr}});

        return added_node;
    }

    void RenderGraph_NG::RenderEditorTitle() {

        ImNodes::EWE::Editor::RenderEditorTitle();

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
                        added_node.pins.emplace_back(ImNodes::EWE::Pin{.local_pos{0.f, pin_starting}, .payload = wait});
                        pin_starting += 0.1f;
                    }
                    pin_starting = 0.3f;
                    for(auto& signal : ind_sub->submitInfo[0].signalSemaphores){
                        //currently a mem leak
                        added_node.pins.emplace_back(ImNodes::EWE::Pin{.local_pos{1.f, pin_starting}, .payload = &signal});
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
                //CreateRGNode(new FullRenderInfo("unnamed", *Global::logicalDevice, Global::stcManager->renderQueue, AttachmentSetInfo{}));
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

    void RenderGraph_NG::RenderNode(ImNodes::EWE::Node& node) {

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
            auto* sub_group = reinterpret_cast<std::vector<SubmissionTask*>*>(payload->payload);
            if(ImGui::BeginTable("sub group", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable)){
                ImGui::TableSetupColumn("name");
                ImGui::TableSetupColumn("render info");

                ImGui::TableHeadersRow();

                for(auto* subTask : *sub_group){
                    ImGui::TableNextColumn();
                    if(ImGui::TreeNode(subTask->name.string().c_str())){
                        ImguiExtension::Imgui(*subTask);
                        for(auto& task : subTask->tasks) {
                            for(auto& pkg : task->pkgRecord->packages){
                                switch(pkg->type){
                                    case Command::InstructionPackage::Type::Raster:{
                                        auto& rasterPkg = *reinterpret_cast<RasterPackage*>(pkg);
                                        for(auto& obj : rasterPkg.objectPackages){
                                            std::size_t instruction_offset = 0;
                                            for(auto const& inst : obj->paramPool.instructions){
                                                if(inst == Inst::Push){
                                                
                                                    for(auto& shader : obj->payload.shaders){
                                                        if(shader != nullptr){
                                                            for(std::size_t buf_index = 0; buf_index < shader->meta.buffer_written_to.Size(); buf_index++){

                                                                uint32_t color = (0xFF << 24) + 0xFF;
                                                                
                                                                if(shader->meta.buffer_written_to[buf_index]){
                                                                    ImNodes::PushColorStyle(ImNodes::ImNodesCol_Pin, color);
                                                                }

                                                                //ImNodes::BeginPinAttribute(node.id + current_pin + 1, ImVec2{0.0f, 0.3f});
                                                                //ImGui::Text(shader->pushRange.buffers[buf_index].name.c_str());
                                                                //current_pin++;
                                                                //ImNodes::EndPinAttribute();

                                                                if(shader->meta.buffer_written_to[buf_index]){
                                                                    ImNodes::PopColorStyle();
                                                                }
                                                            }
                                                            for(std::size_t img_index = 0; img_index < shader->meta.texture_written_to.Size(); img_index++){

                                                                uint32_t color = (0xFF << 24) + 0xFF;
                                                                
                                                                if(shader->meta.texture_written_to[img_index]){
                                                                    ImNodes::PushColorStyle(ImNodes::ImNodesCol_Pin, color);
                                                                }

                                                                //ImNodes::BeginPinAttribute(node.id + current_pin + 1, ImVec2{0.0f, 0.3f});
                                                                //ImGui::Text(shader->pushRange.textures[img_index].name.c_str());
                                                                //current_pin++;
                                                                //ImNodes::EndPinAttribute();

                                                                if(shader->meta.texture_written_to[img_index]){
                                                                    ImNodes::PopColorStyle();
                                                                }
                                                            }
                                                        }
                                                    }
                                                }
                                                instruction_offset += Inst::GetParamSize(inst);
                                            }
                                        }
                                        break;
                                    }
                                    default: break;
                                }
                            }
                        }
                        ImGui::TreePop();
                    }
                    ImGui::TableNextColumn();
                    ImGui::Text("sub task's render info");
                }
                ImGui::TableNextColumn();
                ImGui::Button("task drag point");
                SubmissionTask* dragTask;
                if(DragDropPtr::Target(dragTask)){
                    sub_group->push_back(dragTask);
                }
                ImGui::TableNextColumn();
                ImGui::EndTable();
            }
            //ImGui::Text("filler");   
        }
        else{
            ImguiExtension::Imgui(reinterpret_cast<FullRenderInfo*>(payload->payload)->full);
        }
    }

    void RenderGraph_NG::RenderPin(ImNodes::EWE::Node& node, ImNodes::EWE::PinOffset pin_index) {
        auto& pin = node.pins[pin_index];

        if (ImNodes::BeginPinAttribute(node.id + pin_index + 1, pin.local_pos)) {
            //ImGui::TextColored(ImVec4(1.f, 0.f, 0.f, 1.f), "%zu", payload->stageMask);
        }
        else {
            //ImGui::Text("%zu", static_cast<uint64_t>(payload->stageMask));
        }
        ImNodes::EndPinAttribute();
    }


    void RenderGraph_NG::InitFromObject(RenderGraph& _renderGraph){

    }

} //namespace Node
} //namespace EWE