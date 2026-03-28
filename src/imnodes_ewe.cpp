#include "EWEngine/Imgui/ImNodes/imnodes_ewe.h"
#include "EWEngine/Imgui/ImNodes/imnodes.h"
#include "imgui.h"

#include "EWEngine/Global.h"
#include "EightWinds/Window.h"

#include "LAB/Support/Generic.h"


namespace ImNodes{
    namespace EWE{

        int Node::CheckPin(PinID globalPinID){
            
            int ret = globalPinID - id - 1;

            if((ret >= pins.size())){
                return -1;
            }
            return ret;
        }

        bool context_created = false;

        Editor::Editor()
        : context{ImNodes::EditorContextCreate()}
        {
		    if(!context_created){
                context_created = true;

        		ImNodes::CreateContext();
            }
            ImNodes::PushAttributeFlag(ImNodes::ImNodesAttributeFlags_EnableLinkDetachWithDragClick);
            
            //initialize window data
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO();

            
            io.DisplaySize = ImVec2(::EWE::Global::window->screenDimensions.width, ::EWE::Global::window->screenDimensions.height); 
            io.DeltaTime = 1.0f;

            ImGui::NewFrame();
            ImNodes::EditorContextSet(context);
            ImNodes::BeginNodeEditor();

            node_editor_window_pos = ImGui::GetWindowPos();
            node_editor_window_size = ImGui::GetWindowSize();
            ImNodes::EndNodeEditor();
            ImGui::EndFrame();
        }


        Editor::Editor(bool saveFunc, bool loadFunc) : Editor{}{
            has_save_func = saveFunc;
            has_load_func = loadFunc;
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

        void Editor::ImGuiNodeDebugPrint(Node& node) const {
            if(node.id != 0){
                auto temp_pos = ImNodes::GetNodeScreenSpacePos(node.id);
                ImGui::Text("node : %d - pos : %.2f:%.2f", node.id, temp_pos.x, temp_pos.y);
            }
        }

        void Editor::RenderEditorTitle(){
            ImGui::Text("node count : %zu", nodes.Size());

            ImGui::Text("window pos  : %.2f:%.2f", node_editor_window_pos.x, node_editor_window_pos.y);
            ImGui::Text("window size  : %.2f:%.2f", node_editor_window_size.x, node_editor_window_size.y);
            //ImGui::Text("mouse in editor coords : %.2f:%.2f", ImNodes::Get)
            
            if(ImGui::TreeNode("node data (debugging)")){

                for(auto& node : nodes){
                    ImGuiNodeDebugPrint(node);
                }
                ImGui::TreePop();
            }
                

            auto editor_pan_pos = ImNodes::EditorContextGetPanning();
            if(ImGui::SliderFloat2("pan pos", &editor_pan_pos.x, -10000.f, 10000.f)){
                ImNodes::EditorContextResetPanning(editor_pan_pos);
            }
        }

        void Editor::RenderNodes() {
            ImNodes::EditorContextSet(context);

            RenderEditorTitle();

            //title bar here, save and whatever else
            if(has_save_func){
                if(ImGui::Button("Save")){
                    save_open = true;
                }
                if(save_open){
                    if(SaveFunc()){
                        save_open = false;
                    }
                }
            }
            if(has_load_func){
                if(has_save_func){
                    ImGui::SameLine();
                }
                if(ImGui::Button("Load")){
                    load_open = true;
                }
                if(load_open){
                    if(LoadFunc()){
                        load_open = false;
                    }
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
                    menu_pos = ImGui::GetMousePos();
                    OpenAddMenu();
                    submissionReady = false;
                }

                if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) && ImGui::IsKeyReleased(ImGuiKey_S)) {
                    attemptingToSave = true;
                }
            }

            int current_index = INT32_MIN + 1;
            for (auto& node : nodes) {
                if(node.id != current_index){
                    ImNodes::SetNodeScreenSpacePos(current_index, node.pos);
                }
                node.id = current_index++;
                ImGui::SetNextWindowSizeConstraints(ImVec2(50, -1), ImVec2(400, -1));
                ImNodes::BeginNode(node.id);

                if(node.snapToGrid){
                    ImNodes::SnapNodeToGrid(node.id);
                }
                node.pos = ImNodes::GetNodeScreenSpacePos(node.id);
                RenderNode(node);

                current_index += node.pins.size();
                for (PinOffset i = 0; i < node.pins.size(); i++) {
                    RenderPin(node, i);
                }
                
                
                ImNodes::EndNode();
            }

            for (auto& link : links) {
                link.id = current_index++;
                ImNodes::Link(link.id, link.start->id + link.start_offset + 1, link.end->id + link.end_offset + 1);
            }

            int returned_deleted_node = ImNodes::EndNodeEditor();

            { //link start and drop
                PinID link_pin_id = -1;
                if(ImNodes::IsLinkStarted(&link_pin_id)){
                    //this is holding a link
                }
                if(ImNodes::IsLinkDropped(&link_pin_id)){

                    Node* starting_node = nullptr;

                    for(auto& node : nodes){
                        int pin_offset = node.CheckPin(link_pin_id);
                        if(pin_offset >= 0){
                            starting_node = &node;
                            break;
                        }
                    }
                    EWE_ASSERT(starting_node != nullptr);

                    //this is releasing a pin
                    LinkEmptyDrop(*starting_node, (link_pin_id - starting_node->id) - 1);
                }
                if(ImNodes::IsLinkDestroyed(&link_pin_id)){
                    for(auto link_iter = links.begin(); link_iter != links.end(); link_iter++){
                        if(link_iter->id == link_pin_id){
                            links.erase(link_iter);
                            break;
                        }
                    }
                }
            }
            {
                PinID link_created_lh;
                PinID link_created_rh;
                if(ImNodes::IsLinkCreated(&link_created_lh, &link_created_rh)){

                    NodePair& pair = links.emplace_back(
                        NodePair{
                            .start = nullptr,
                            .end = nullptr
                        }
                    );

                    for(auto & node : nodes){
                        int lh_pin_offset = node.CheckPin(link_created_lh);
                        int rh_pin_offset = node.CheckPin(link_created_rh);
                        if(lh_pin_offset >= 0){
                            pair.start = &node;
                            pair.start_offset = lh_pin_offset;

                            if(pair.end != nullptr){
                                break;
                            }
                        }
                        else if(rh_pin_offset >= 0) {
                            pair.end = &node;
                            pair.end_offset = rh_pin_offset;

                            if(pair.start != nullptr){
                                break;
                            }
                        }
                    }
                    
                    LinkCreated(pair);
                }
            }

            if(add_menu_is_open){
                add_menu_is_open = !RenderAddMenu();
            }

            bool delete_pressed = ImGui::IsKeyPressed(ImGuiKey_Delete);
            if(delete_pressed){

                int hovered_id;
                if(ImNodes::IsLinkHovered(&hovered_id)){
                    for(auto& link : links){
                        if(hovered_id == link.id){
                            DestroyPressedOnLink(link);
                            break;
                        }
                    }
                }
                if(ImNodes::IsPinHovered(&hovered_id)){
                    for(auto& node : nodes){
                        int pin_check = node.CheckPin(hovered_id);
                        if(pin_check >= 0){
                            DestroyPressedOnPin(node, pin_check);
                            break;
                        }
                    }
                }
                else if (ImNodes::IsNodeHovered(&hovered_id)){
                    for(auto& node : nodes){
                        if(node.id == hovered_id){
                            DestroyPressedOnNode(node);
                            break;
                        }
                    }
                }
            }
        }
    } //namespace EWE
} //namespace ImNodes