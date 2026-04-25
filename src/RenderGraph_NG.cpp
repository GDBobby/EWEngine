#include "EWEngine/Imgui/ImNodes/Graph/RenderGraph_NG.h"

#include "EWEngine/Global.h"
#include "EWEngine/Imgui/DragDrop.h"
#include "EightWinds/Backend/RenderInfo.h"
#include "EightWinds/RenderGraph/SubmissionTask.h"

#include "EWEngine/Imgui/Objects.h"

namespace EWE{
namespace Node{
    RenderGraph_NG::RenderGraph_NG()
        : ImNodes::EWE::Editor{},
        renderGraph{nullptr}
    {
        name = "render graph ng";
    }
    RenderGraph_NG::RenderGraph_NG(RenderGraph& _renderGraph)
        : ImNodes::EWE::Editor{},
        renderGraph{&_renderGraph}
    {
        name = "render graph ng";
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
            .type = NodeType::RenderInfo,
            .payload = subTask
        };
        added_node.pos = menu_pos;
        added_node.pos = menu_pos;
        added_node.pins.emplace_back(ImNodes::EWE::Pin{.local_pos{0.f, 0.5f}, .payload{nullptr}});
        added_node.pins.emplace_back(ImNodes::EWE::Pin{.local_pos{1.f, 0.5f}, .payload{nullptr}});

        return added_node;
    }

    void RenderGraph_NG::RenderEditorTitle() {

        ImNodes::EWE::Editor::RenderEditorTitle();

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
        
    }

    bool RenderGraph_NG::RenderAddMenu(){

        bool wantsClose = false;

        ImGui::SetNextWindowPos(menu_pos);
        if(ImGui::Begin("add menu")){
            
            if(ImGui::Button("add render info")){
                CreateRGNode(new FullRenderInfo("unnamed", *Global::logicalDevice, Global::stcManager->renderQueue, AttachmentSetInfo{}));
                    wantsClose = true;
            }

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
        auto* payload = reinterpret_cast<NodePayload*>(node.payload);

        ImNodes::BeginNodeTitleBar();
        if(payload->type == NodeType::Task){
            ImGui::TextUnformatted(reinterpret_cast<SubmissionTask*>(payload->payload)->name.c_str());
        }
        else{
            ImGui::Text("render info");
        }
        ImNodes::EndNodeTitleBar();

        if(payload->type == NodeType::Task){
            ImguiExtension::Imgui(*reinterpret_cast<SubmissionTask*>(payload->payload));
        }
        else{
            ImguiExtension::Imgui(reinterpret_cast<FullRenderInfo*>(payload->payload)->full);
        }
    }

    void RenderGraph_NG::RenderPin(ImNodes::EWE::Node& node, ImNodes::EWE::PinOffset pin_index) {
        auto& pin = node.pins[pin_index];
        auto* payload = reinterpret_cast<VkSemaphoreSubmitInfo*>(pin.payload);

        if (ImNodes::BeginPinAttribute(node.id + pin_index, pin.local_pos)) {
            ImGui::TextColored(ImVec4(1.f, 0.f, 0.f, 1.f), "%zu", payload->stageMask);
        }
        else {
            ImGui::Text("%zu", static_cast<uint64_t>(payload->stageMask));
        }
        ImNodes::EndPinAttribute();
    }


    void RenderGraph_NG::InitFromObject(RenderGraph& _renderGraph){

    }

} //namespace Node
} //namespace EWE