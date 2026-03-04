#pragma once

#include "EWEngine/Reflect/Enum.h"
#include "EightWinds/RenderGraph/RenderGraph.h"

#include "EWEngine/Imgui/ImNodes/imnodes_ewe.h"
#include "imgui.h"

namespace EWE{
    namespace Node{
        struct RenderGraphNodeGraph{
            RenderGraph& renderGraph;
            ImNodes::EWE::Editor editor;

            [[nodiscard]] explicit RenderGraphNodeGraph(RenderGraph& renderGraph)
                : renderGraph{renderGraph},
                editor{}
            {
                editor.name = "render graph";
                SetFunctionHooks();
            }

            void TitleExtension(){
                if(ImGui::Button("set from current")){
                    for(auto iter = editor.nodes.begin(); iter != editor.nodes.end(); ++iter){
                        editor.nodes.DestroyElement(iter);
                    }
                    editor.links.clear();

                    float horizontalPos = editor.node_editor_window_pos.x;
                    for(auto& sub_group : renderGraph.execution_order){
                        float verticalPos = editor.node_editor_window_pos.y;

                        for(auto& ind_sub : sub_group){
                            auto& added_node = editor.AddNode();
                            added_node.payload = ind_sub;
                            added_node.nodePos.x = horizontalPos;
                            added_node.nodePos.y = verticalPos;
                            verticalPos += 60.f;

                            float pin_starting = 0.3f;
                            for(auto& wait : ind_sub->submitInfo[0].waitSemaphores){
                                added_node.pins.emplace_back(new ImNodes::EWE::Pin{.local_pos{0.f, pin_starting}, .payload = wait});
                                pin_starting += 0.1f;
                            }
                            pin_starting = 0.3f;
                            for(auto& signal : ind_sub->submitInfo[0].signalSemaphores){
                                added_node.pins.emplace_back(new ImNodes::EWE::Pin{.local_pos{1.f, pin_starting}, .payload = &signal});
                                pin_starting += 0.1f;
                            }
                        }
                        horizontalPos += 200.f;
                    }
                }
                
            }

            bool AddNodeMenu(ImVec2 menu_pos){
                //printf("iterating add node menu\n");

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

            void RenderNode(ImNodes::EWE::Node& node){
                EWE::SubmissionTask* payload = reinterpret_cast<EWE::SubmissionTask*>(node.payload);

                ImNodes::BeginNodeTitleBar();
                ImGui::TextUnformatted(payload->name.c_str());
                ImNodes::EndNodeTitleBar();

                ImGui::Text("queue family index : %u", payload->queue.FamilyIndex());
            }

            void PinRender(ImNodes::EWE::Node& node, ImNodes::EWE::PinOffset pin_index){
                auto& pin = node.pins[pin_index];
                auto* payload = reinterpret_cast<VkSemaphoreSubmitInfo*>(pin->payload);

                if (ImNodes::BeginPinAttribute(node.id + pin_index, pin->local_pos)) {
                    ImGui::TextColored(ImVec4(1.f, 0.f, 0.f, 1.f), "%zu", payload->stageMask);
                }
                else {
                    ImGui::Text("%zu", static_cast<uint64_t>(payload->stageMask));
                }
                ImNodes::EndPinAttribute();
            }

            void SetFunctionHooks(){
                editor.render_add_menu = [&](ImVec2 menu_pos){
                    return AddNodeMenu(menu_pos);
                };
                editor.title_extension = [&]{
                    TitleExtension();
                };
                editor.node_renderer = [&](ImNodes::EWE::Node& node){
                    RenderNode(node);
                };
                editor.pin_renderer = [&](ImNodes::EWE::Node& node, ImNodes::EWE::PinOffset pin_index){
                    PinRender(node, pin_index);
                };
            }

        };
    }
}