#include "EWEngine/Imgui/ImNodes/imnodes_ewe.h"
#include "EWEngine/Imgui/ImNodes/imnodes.h"
#include "imgui.h"
#include "imgui_internal.h"



namespace ImNodes{
    namespace EWE{

        bool context_created = false;

        Editor::Editor()
        : context{ImNodes::EditorContextCreate()}
        {
		    if(!context_created){
                context_created = true;

        		ImNodes::CreateContext();
            }
            ImNodes::PushAttributeFlag(ImNodes::ImNodesAttributeFlags_EnableLinkDetachWithDragClick);
            
        }

        /*
        Editor::NodeAndPinOffset Editor::GetOtherNodeFromLinks(int rawPinID) {
            for (auto& link : links) {
                if (link.start_attr == rawPinID) {
                    return NodeAndPinOffset(nodes, link.end_attr);
                }
                if (link.end_attr == rawPinID) {
                    return NodeAndPinOffset(nodes, link.start_attr);
                }
            }
            return {};
        }

        NodePair Editor::GetBothNodesFromLink(Link const& link) {

            int startingNodeID = link.start_attr >> pin_shift;
            int endingNodeID = link.end_attr >> pin_shift;

            NodePair ret;
            ret.start = &nodes.at(startingNodeID);
            ret.end = &nodes.at(endingNodeID);

            return ret;
        }

        Editor::NodeAndPinOffset::NodeAndPinOffset(std::unordered_map<int, Node>& nodes, int rawPinID) : rawPinID{ rawPinID } {
            exists = true;
            nodeID = rawPinID >> pin_shift;
            pinOffset = rawPinID - (nodeID << pin_shift);
    #if EWE_DEBUG
            if (!nodes.contains(nodeID)) {
                exists = false;
            }
    #endif
        }
        */

        void Editor::RenderNodes(){
            ImNodes::EditorContextSet(context);

            int deletedNode;
            if(ImGui::Begin(name.c_str())){

                ImGui::Text("node count : %zu", nodes.Size());

                ImGui::Text("window pos  : %.2f:%.2f", node_editor_window_pos.x, node_editor_window_pos.y);
                ImGui::Text("window size  : %.2f:%.2f", node_editor_window_size.x, node_editor_window_size.y);
                //ImGui::Text("mouse in editor coords : %.2f:%.2f", ImNodes::Get)
                
                for(auto& node : nodes){
                    auto temp_pos = ImNodes::GetNodeScreenSpacePos(node.id);
                    ImGui::Text("node : %d - pos : %.2f:%.2f", node.id, temp_pos.x, temp_pos.y);
                    //ImGui::PushID(node.id);
                    //ImGui::SliderFloat2("node pos", &node.nodePos.x, -1000.f, 1000.f);
                    //ImGui::PopID();
                }
                    

                auto editor_pan_pos = ImNodes::EditorContextGetPanning();
                if(ImGui::SliderFloat2("pan pos", &editor_pan_pos.x, -10000.f, 10000.f)){
                    ImNodes::EditorContextResetPanning(editor_pan_pos);
                }

                if(title_extension){
                    title_extension();
                }

                if(ImGui::BeginTabBar("tab bar")){
                    if(ImGui::BeginTabItem("tab 1")){
                        if(ImGui::Button("Save")){
                            if(save){
                                save();
                            }
                        }
					    ImNodes::BeginNodeEditor();

                        node_editor_window_pos = ImGui::GetWindowPos();
                        node_editor_window_size = ImGui::GetWindowSize();

                        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && ImNodes::IsEditorHovered()) {



                            if (ImGui::IsKeyPressed(ImGuiKey_A) && !submissionReady) {
                                submissionReady = true;
                            }
                            if(ImGui::IsKeyReleased(ImGuiKey_A) && submissionReady){
                                add_menu_is_open = true;
                                if(nodes.Size() > 0){
                                    auto first_node_iter = nodes.begin();
                                    first_node_iter->nodePos = ImGui::GetMousePos();
                                }
                                menu_pos = ImGui::GetMousePos();
                                /*
                                if(init_node != nullptr){
                                    Node& creation_node = AddNode();
                                    creation_node.nodePos = ImGui::GetMousePos();
                                    init_node(creation_node);
                                }
                                */
                                submissionReady = false;
                            }

                            if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) && ImGui::IsKeyReleased(ImGuiKey_S)) {
                                attemptingToSave = true;
                            }
                        }

                        int current_index = INT32_MIN + 1;
                        for (auto& node : nodes) {
                            if(node.id != current_index){
                                ImNodes::SetNodeScreenSpacePos(current_index, node.nodePos);
                            }
                            node.id = current_index++;
                            ImNodes::BeginNode(node.id);
                            //ImGui::Dummy(ImVec2(1.0f, 1.0f));

                            node.nodePos = ImNodes::GetNodeScreenSpacePos(node.id);
                            if(node.snapToGrid){
                                ImNodes::SnapNodeToGrid(node.id);
                                node.nodePos = ImNodes::GetNodeScreenSpacePos(node.id);
                            }
                            //ImGui::Dummy(ImVec2(1.0f, 1.0f));

                            if(node_title_renderer != nullptr){
                                node_title_renderer(node);
                            }
                            //ImNodes::BeginNodeTitleBar();
                            //ImGui::TextUnformatted(node.levelName.c_str());
                            //ImNodes::EndNodeTitleBar();

                            if(node_renderer){
                                node_renderer(node);
                            }
                            if(pin_renderer){

                                current_index += node.pins.size();
                                for (PinOffset i = 0; i < node.pins.size(); i++) {

                                    //globalPinID can be calculated from node.id + i
                                    pin_renderer(node, i);
                                    
                                    //ImNodes::BeginPinAttribute();
                                    /*
                                    if (ImNodes::BeginPinAttribute(pinIndex, node.exits[i].positionOnMap)) {
                                        //ImGui::TextColored(ImVec4(1.f, 0.f, 0.f, 1.f), node.exits[i].destination.c_str());
                                        //node.exits[i].linking = false;
                                    }
                                    else {
                                        //ImGui::TextUnformatted(node.exits[i].destination.c_str());
                                    }
                                    ImNodes::EndPinAttribute();
                                    */
                                }
                            }
                            
                            ImNodes::EndNode();
                        }

                        int link_id = INT32_MIN + 1;
                        for (auto const& link : links) {
                            ImNodes::Link(link_id++, link.start->id + link.start_offset, link.end->id + link.end_offset);
                        }

                        deletedNode = ImNodes::EndNodeEditor();
                        ImGui::EndTabItem();
                    }
                    ImGui::EndTabBar();

                    if(add_menu_is_open){
                        if(render_add_menu != nullptr){
                            if(render_add_menu(menu_pos)){
                                printf("closing add menu by request\n");
                                add_menu_is_open = false;
                            }
                        }
                        else{
                            add_menu_is_open = false;
                            printf("closing add menu, no func\n");
                        }
                    }
                }
            }
            ImGui::End();
        }
    } //namespace EWE
} //namespace ImNodes