#pragma once


#include "EWEngine/Reflect/Enum.h"
#include "EightWinds/RenderGraph/Command/Record.h"

#include "EWEngine/Tools/ImguiFileExplorer.h"

#include "EWEngine/Imgui/ImNodes/imnodes_ewe.h"
#include "imgui.h"

#include <fstream>

namespace EWE{
    namespace Node{

        struct RecordNodeGraph : ImNodes::EWE::Editor {
            std::vector<Instruction::Type> instructions_data;
            ExplorerContext explorer;

            ImNodes::EWE::Node& headNode;

            std::vector<Instruction::Type> acceptable_add_instructions{};

            [[nodiscard]] explicit RecordNodeGraph()
            : ImNodes::EWE::Editor{true, true},
                explorer{std::filesystem::current_path()},
                headNode{AddNode()}
            {
                explorer.acceptable_extensions.push_back(".ewrg");
                headNode.snapToGrid = true;
                headNode.payload = reinterpret_cast<void*>(UINT64_MAX); //std::Size_t_max would be better but i dont want to include type_traits
                headNode.pos = node_editor_window_pos;
                headNode.pins.emplace_back(new ImNodes::EWE::Pin{.local_pos{1.f, 0.5f}, .payload{nullptr}});
            }

            std::vector<Instruction::Type> CollectInstructionsUpTo(ImNodes::EWE::Node* limit_node) const{
                std::vector<Instruction::Type> ret{};
                
                ImNodes::EWE::Node* current_node = nullptr;
                //the pin payload is going to be a pointer to the node, unless nullptr
                if(headNode.pins[0]->payload != nullptr){
                    ImNodes::EWE::Node* current_node = reinterpret_cast<ImNodes::EWE::Node*>(headNode.pins[0]->payload);

                    if(current_node == limit_node){
                        return ret;
                    }
                    ret.push_back(static_cast<Instruction::Type>(reinterpret_cast<std::size_t>(current_node->payload)));
                    if(limit_node == nullptr){
                        while(current_node->pins[1]->payload != nullptr){
                            current_node = reinterpret_cast<ImNodes::EWE::Node*>(current_node->pins[1]->payload);
                            ret.push_back(static_cast<Instruction::Type>(reinterpret_cast<std::size_t>(current_node->payload)));
                        }
                    }
                    else{
                        while(current_node->pins[1]->payload != nullptr){
                            current_node = reinterpret_cast<ImNodes::EWE::Node*>(current_node->pins[1]->payload);
                            if(current_node == limit_node){
                                return ret;
                            }
                            ret.push_back(static_cast<Instruction::Type>(reinterpret_cast<std::size_t>(current_node->payload)));
                        }
                    }
                }

                if(limit_node != nullptr){
                    //if the limit_node wasn't in the chain, an empty vector will be returned
                    //if it was in the chain, it was the last node
                    if(current_node != limit_node){
                        return {};
                    }
                }

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
                    const std::size_t record_index = instructions_data.size();
                    
                    instructions_data.push_back(itype);
                    auto& added_node = AddNode();
                    added_node.payload = reinterpret_cast<void*>(record_index);//&emp;
                    added_node.pos = menu_pos;
                    added_node.pins.emplace_back(new ImNodes::EWE::Pin{
                                                    .local_pos{0.f, 0.5f},
                                                    .payload{nullptr}}
                                                );
                    added_node.pins.emplace_back(new ImNodes::EWE::Pin{.local_pos{1.f, 0.5f}, .payload{nullptr}});

                    return true;
                }
                return false;
            }

            bool RenderAddMenu() override final{
                bool wantsClose = false;

                ImGui::SetNextWindowPos(menu_pos);
                if(ImGui::Begin("add menu")){
                    if(!ImGui::IsWindowFocused()){
                        return true;
                    }
                    for(auto const& inst : acceptable_add_instructions){
                        wantsClose |= AddInstructionButton(inst);
                    }
                }
                ImGui::End();
                return wantsClose;
            }

            void LinkEmptyDrop(ImNodes::EWE::Node& src_node, ImNodes::EWE::PinOffset pin_offset) override {
                auto const& instructions = CollectInstructionsUpTo(&src_node);

                acceptable_add_instructions = Instruction::GetValidInstructionsAtBackOf(instructions);

                printf("link empty dropped : %d : %d\n", src_node.id, pin_offset);

                menu_pos = ImGui::GetMousePos();
                add_menu_is_open = true;
            }

            void RenderNode(ImNodes::EWE::Node& node) override final{

                const std::size_t record_index = reinterpret_cast<std::size_t>(node.payload);

                ImNodes::BeginNodeTitleBar();

                if(record_index == UINT64_MAX){
                    ImGui::TextColored(ImVec4(0.2f, 0.f, 0.f, 1.f), "head node");
                }
                else{
                    ImGui::Text("%s", Reflect::enum_to_string(instructions_data[record_index]).data());
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
                        Command::Record::WriteInstructions(saved_path.string(), collected_instructions);
                    }
                    if(!ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows)){
                        //auto imwin = ImGui::GetCurrentWindow();
                        explorer.enabled = false;
                    }
                }
                ImGui::End();
                return !explorer.enabled;
            }

            bool LoadFunc() override final{
                explorer.enabled = load_open;
                explorer.state = ExplorerContext::State::Load;
                if(ImGui::Begin("file load")){
                    explorer.Imgui();
                    if(explorer.selected_file.has_value()){
                        const std::filesystem::path load_path = *explorer.selected_file;
                        //put the record somewhere
                        auto const& instructions = Command::Record::ReadInstructions(load_path.string());
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