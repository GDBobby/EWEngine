#pragma once


#include "EWEngine/Reflect/Enum.h"
#include "EightWinds/RenderGraph/Command/Record.h"

#include "EWEngine/Tools/ImguiFileExplorer.h"

#include "EWEngine/Imgui/ImNodes/imnodes_ewe.h"
#include "imgui.h"

#include <fstream>

namespace EWE{
    namespace Node{

        /*
            i can stop links from being made if it's not a valid instruction (binding descriptor out of a pipeline)

            i can only allow certain nodes to be made at a certain point, using link drop

            im putting that ^^^^ off for now
        */

        struct RecordNodeGraph : ImNodes::EWE::Editor {
            ExplorerContext explorer;

            ImNodes::EWE::Node* headNode;

            std::vector<Instruction::Type> acceptable_add_instructions{};

            ImNodes::EWE::Node* link_empty_drop_srcNode = nullptr;

            [[nodiscard]] explicit RecordNodeGraph()
            : ImNodes::EWE::Editor{true, true},
                explorer{std::filesystem::current_path()},
                headNode{CreateHeadNode()}
            {
                explorer.acceptable_extensions.push_back(".ewrg");
            }

            ImNodes::EWE::Node* CreateHeadNode(){
                auto& head = AddNode();
                head.snapToGrid = true;
                head.payload = reinterpret_cast<void*>(UINT64_MAX);
                head.pos = node_editor_window_pos;
                head.pins.emplace_back(new ImNodes::EWE::Pin{.local_pos{1.f, 0.5f}, .payload{nullptr}});
                return &head;
            } 

            static Instruction::Type GetInstructionFromNode(ImNodes::EWE::Node& node){
                return static_cast<Instruction::Type>(reinterpret_cast<std::size_t>(node.payload));
            }

            void ImGuiNodeDebugPrint(ImNodes::EWE::Node& node) const override final{
                if(node.id != 0){
                    auto temp_pos = ImNodes::GetNodeScreenSpacePos(node.id);
                    ImGui::Text("node.id[%d] : inst[%s] - pos[%.2f:%.2f]", node.id, Reflect::enum_to_string(GetInstructionFromNode(node)).data(), temp_pos.x, temp_pos.y);
                }
            }

            void PrintNode(ImNodes::EWE::Node& node) const{
                printf("node.id[%d] - type[%s] - node.pin[0].addr[%zu] - node.pin[1].addr[%zu]\n", 
                    node.id, 
                    Reflect::enum_to_string(GetInstructionFromNode(node)).data(), 
                    node.pins[0]->payload, 
                    node.pins[1]->payload
                );
            }

            ImNodes::EWE::Node& CreateRGNode(int inst_index) {
                auto& added_node = AddNode();
                added_node.payload = reinterpret_cast<void*>(inst_index);//&emp;
                added_node.pos = menu_pos;
                added_node.pins.emplace_back(new ImNodes::EWE::Pin{.local_pos{0.f, 0.5f}, .payload{nullptr}});
                added_node.pins.emplace_back(new ImNodes::EWE::Pin{.local_pos{1.f, 0.5f}, .payload{nullptr}});

                return added_node;
            }

            std::vector<Instruction::Type> CollectInstructionsUpTo(ImNodes::EWE::Node* limit_node) const{
                std::vector<Instruction::Type> ret{};
                
                ImNodes::EWE::Node* current_node = nullptr;
                //the pin payload is going to be a pointer to the node, unless nullptr
                if(headNode->pins[0]->payload != nullptr){
                    ImNodes::EWE::Node* current_node = reinterpret_cast<ImNodes::EWE::Node*>(headNode->pins[0]->payload);
                    PrintNode(*current_node);
                    ret.push_back(GetInstructionFromNode(*current_node));
                    if(current_node == limit_node){
                        //printf("early early return - %s\n", __FUNCTION__);
                        return ret;
                    }

                    if(limit_node == nullptr){
                        while(current_node->pins[1]->payload != nullptr){
                            PrintNode(*current_node);

                            current_node = reinterpret_cast<ImNodes::EWE::Node*>(current_node->pins[1]->payload);
                            auto const current_inst = GetInstructionFromNode(*current_node);
                            ret.push_back(current_inst);
                        }
                    }
                    else{
                        while(current_node->pins[1]->payload != nullptr){
                            current_node = reinterpret_cast<ImNodes::EWE::Node*>(current_node->pins[1]->payload);
                            auto const current_inst = GetInstructionFromNode(*current_node);
                            ret.push_back(current_inst);
                            if(current_node == limit_node){
                                //printf("early limit return - %zu :  %s\n", ret.size(), __FUNCTION__);
                                //for(auto& inst : ret){
                                //    printf("\t%s\n", Reflect::enum_to_string(inst).data());
                                //}
                                return ret;
                            }
                        }
                    }
                }

                if(limit_node != nullptr){
                    //if the limit_node wasn't in the chain, an empty vector will be returned
                    //if it was in the chain, it was the last node
                    if(current_node != limit_node){
                        //printf("empty return, current_node not equal to limit_node in %s\n", __FUNCTION__);
                        return {};
                    }
                }

                //printf("ret - %zu : %s\n", ret.size(), __FUNCTION__);
                //for(auto& inst : ret){
                //    printf("\t%s\n", Reflect::enum_to_string(inst).data());
                //}
                return ret;
            }

            std::vector<Instruction::Type> CollectInstructions() const{
                return CollectInstructionsUpTo(nullptr);
            }

            void OpenAddMenu() override final{
                add_menu_is_open = true;
                static constexpr std::size_t type_count = std::meta::enumerators_of(^^Instruction::Type).size();
                if(acceptable_add_instructions.size() != type_count){
                    acceptable_add_instructions.clear();
                    acceptable_add_instructions.reserve(type_count);
                    template for(constexpr auto inst : std::define_static_array(std::meta::enumerators_of(^^Instruction::Type))){
                        acceptable_add_instructions.push_back([:inst:]);
                    }
                }
            }

            bool AddInstructionButton(Instruction::Type itype){
                if(ImGui::Button(Reflect::enum_to_string(itype).data())){
                    auto& added_node = CreateRGNode(itype);
                    if(link_empty_drop_srcNode != nullptr){
                        const uint16_t start_offset = link_empty_drop_srcNode == headNode ? 0 : 1;
                        links.emplace_back(
                            ImNodes::EWE::NodePair{
                                .start = link_empty_drop_srcNode,
                                .start_offset = start_offset,
                                .end = &added_node,
                                .end_offset = 0
                            }
                        );
                    }
                    return true;
                }
                return false;
            }

            bool RenderAddMenu() override final{
                bool wantsClose = false;

                bool window_not_focused;

                ImGui::SetNextWindowPos(menu_pos);
                if(ImGui::Begin("add menu")){
                    window_not_focused = !ImGui::IsWindowFocused();
                    for(auto const& inst : acceptable_add_instructions){
                        wantsClose |= AddInstructionButton(inst);
                    }
                }
                ImGui::End();
                if(wantsClose | window_not_focused){
                    link_empty_drop_srcNode = nullptr;
                }
                return wantsClose | window_not_focused;
            }

            void LinkEmptyDrop(ImNodes::EWE::Node& src_node, ImNodes::EWE::PinOffset pin_offset) override {
                auto const& instructions = CollectInstructionsUpTo(&src_node);

                acceptable_add_instructions = Instruction::GetValidInstructionsAtBackOf(instructions);

                //printf("link empty dropped : %d : %d\n", src_node.id, pin_offset);

                menu_pos = ImGui::GetMousePos();
                add_menu_is_open = true;
                link_empty_drop_srcNode = &src_node;
            }
            void LinkCreated(ImNodes::EWE::NodePair& link) override final{
                link.start->pins[link.start_offset]->payload = link.end;
                link.end->pins[link.end_offset]->payload = link.start;
            }

            void RenderNode(ImNodes::EWE::Node& node) override final{

                const std::size_t inst_index = reinterpret_cast<std::size_t>(node.payload);

                ImNodes::BeginNodeTitleBar();

                if(inst_index == UINT64_MAX){
                    ImGui::TextColored(ImVec4(1.f, 0.f, 0.f, 1.f), "head node");
                }
                else{
                    ImGui::Text("%s", Reflect::enum_to_string(static_cast<Instruction::Type>(inst_index)).data());
                }
                ImNodes::EndNodeTitleBar();

                //ImGui::DebugLog("");
                ImGui::Text(""); //empty text just to populate this
            }

            void RenderPin(ImNodes::EWE::Node& node, ImNodes::EWE::PinOffset pin_index) override final{
                auto& pin = node.pins[pin_index];

                if (ImNodes::BeginPinAttribute(node.id + pin_index + 1, pin->local_pos)) {
                    //ImGui::Text("pin");
                }
                ImNodes::EndPinAttribute();
            }

            bool SaveFunc() override final{
                explorer.enabled = save_open;
                explorer.state = ExplorerContext::State::Save;
                if(ImGui::Begin("file save")){
                    explorer.Imgui();
                    if(explorer.selected_file.has_value()){
                        const std::filesystem::path saved_path = *explorer.selected_file;
                        auto const& collected_instructions = CollectInstructions();
                        printf("collected inst count : %zu\n", collected_instructions.size());
                        Command::Record::WriteInstructions(saved_path.string(), collected_instructions);
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

            void CreateFromInstructions(std::span<const Instruction::Type> create_instructions){
                nodes.Clear();
                links.clear();
                if(create_instructions.size() == 0){
                    return;
                }

                headNode = CreateHeadNode();
                auto& init_node = CreateRGNode(create_instructions[0]);
                headNode->pins[0]->payload = reinterpret_cast<ImNodes::EWE::Node*>(&init_node);
                init_node.pos.x = headNode->pos.x + 100.f;
                init_node.pos.y = headNode->pos.y + 30.f;


                auto* lastNode = &init_node;
                init_node.pins[0]->payload = headNode;

                links.emplace_back(
                    ImNodes::EWE::NodePair{
                        .start = headNode,
                        .start_offset = 0,
                        .end = &init_node,
                        .end_offset = 0
                    }
                );

                for(std::size_t i = 1; i < create_instructions.size(); i++){
                    auto& node = CreateRGNode(create_instructions[i]);
                    node.pos = lastNode->pos;
                    node.pos.x += 100.f;
                    node.pos.y += 30.f;

                    node.pins[0]->payload = lastNode;
                    lastNode->pins[1]->payload = &node;


                    links.emplace_back(
                        ImNodes::EWE::NodePair{
                            .start = lastNode,
                            .start_offset = 1,
                            .end = &node,
                            .end_offset = 0
                        }
                    );

                    lastNode = &node;
                }

                nodes.ShrinkToFit();
            }

            bool LoadFunc() override final{
                explorer.enabled = load_open;
                explorer.state = ExplorerContext::State::Load;
                if(ImGui::Begin("file load")){
                    explorer.Imgui();
                    if(explorer.selected_file.has_value()){
                        const std::filesystem::path load_path = *explorer.selected_file;
                        //put the record somewhere
                        auto const& loaded_instructions = Command::Record::ReadInstructions(load_path.string());

                        CreateFromInstructions(std::span{loaded_instructions.Data(), loaded_instructions.Size()});

                        printf("loaded instructions size : %zu\n", loaded_instructions.Size());
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
        };
    }
}