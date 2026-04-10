#include "EWEngine/NodeGraph/RenderGraph_NG.h"

#include "EWEngine/Imgui/DragDrop.h"

namespace EWE{
namespace Node{
    RenderGraphNodeGraph::RenderGraphNodeGraph(RenderGraph& _renderGraph)
        : ImNodes::EWE::Editor{},
        renderGraph{_renderGraph}
    {
        name = "render graph ng";
    }


    void RenderGraphNodeGraph::RenderNodes() {
        ImNodes::EWE::Editor::RenderNodes();
        SubmissionTask** subTask;
        if(DragDropPtr::Target(subTask)) {
            auto temp_min = ImGui::GetItemRectMin();
            auto temp_max =ImGui::GetItemRectMax();
            auto& added_node = CreateRGNode(*subTask);
            auto temp_mouse_pos =ImGui::GetIO().MousePos;
            auto window_pos =  ImGui::GetWindowPos();
            added_node.pos = temp_mouse_pos;// - ImNodes::EditorContextGetPanning();// - (temp_min - window_pos);
        }
    }

    ImNodes::EWE::Node& RenderGraphNodeGraph::CreateRGNode(SubmissionTask* subTask) {
        auto& added_node = AddNode();
        added_node.payload = subTask;
        added_node.pos = menu_pos;
        added_node.pins.emplace_back(ImNodes::EWE::Pin{.local_pos{0.f, 0.5f}, .payload{nullptr}});
        added_node.pins.emplace_back(ImNodes::EWE::Pin{.local_pos{1.f, 0.5f}, .payload{nullptr}});

        return added_node;
    }

    void RenderGraphNodeGraph::RenderEditorTitle() {

        ImNodes::EWE::Editor::RenderEditorTitle();

        if(ImGui::Button("set from current")){
            for(auto iter = nodes.begin(); iter != nodes.end(); ++iter){
                nodes.DestroyElement(iter);
            }
            links.clear();

            float horizontalPos = node_editor_window_pos.x;
            for(auto& sub_group : renderGraph.execution_order){
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

    bool RenderGraphNodeGraph::RenderAddMenu(){

        bool wantsClose = false;

        ImGui::SetNextWindowPos(menu_pos);
        if(ImGui::Begin("add menu")){
            
            //ImGui::Text("%d", ImGui::IsWindowHovered());
            for(auto& submission : renderGraph.submissions){
                if(ImGui::Button(submission.name.c_str())){
                    wantsClose = true;
                }
            }
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

    void RenderGraphNodeGraph::RenderNode(ImNodes::EWE::Node& node) {
        EWE::SubmissionTask* payload = reinterpret_cast<EWE::SubmissionTask*>(node.payload);

        ImNodes::BeginNodeTitleBar();
        ImGui::TextUnformatted(payload->name.c_str());
        ImNodes::EndNodeTitleBar();

        ImGui::Text("queue family index : %u", payload->queue->FamilyIndex());
    }

    void RenderGraphNodeGraph::RenderPin(ImNodes::EWE::Node& node, ImNodes::EWE::PinOffset pin_index) {
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

} //namespace Node
} //namespace EWE