#include "EWEngine/Imgui/ImNodes//NodeGraph_Manager.h"

#include "EWEngine/Imgui/ImNodes/Graph/RasterPackage_NG.h"
#include "EWEngine/Imgui/ImNodes/imnodes_ewe.h"

#include "EWEngine/Imgui/ImNodes/Graph/InstructionPackage_NG.h"
#include "EWEngine/Imgui/ImNodes/Graph/RenderGraph_NG.h"
#include "EWEngine/Imgui/ImNodes/Graph/InstructionPackage_NG.h"
#include "EWEngine/Imgui/ImNodes/Graph/PackageRecord_NG.h"
#include "EWEngine/Imgui/ImNodes/Graph/SubmissionTask_NG.h"
#include "EWEngine/Imgui/ImNodes/Graph/RasterPackage_NG.h"

#include <cmath>

#include "EightWinds/Command/InstructionPackage.h"
#include "imgui.h"
#include "imgui_internal.h"

namespace EWE{

    uint32_t NG_Manager::NewPage::GetTableHeight() const {
        return static_cast<uint32_t>(std::ceil(static_cast<float>(potential_graphs.size()) / 4.f));
    }

    void NG_Manager::AddPotentialGraph(ImNodes::EWE::Editor::Type type, std::function<ImNodes::EWE::Editor*()> creation_func){
        new_page.potential_graphs.push_back(
            GraphIdentifier{
                .type = type,
                .name{Reflect::Enum::ToString(type)}
            }, 
            creation_func
        );
    }

    void NG_Manager::NewPage::Imgui(){
        ImGui::Text("new graph");

        //make it possible to drag in an asset and expand it to a graph
        //need to make an invisible rectangle around some large area
        //maybe around the whole viewport?
        
        if (ImGui::IsWindowAppearing()) {
            ImGui::SetKeyboardFocusHere();
            filter.Clear();
        }
        ImGui::SetNextItemShortcut(ImGuiMod_Ctrl | ImGuiKey_F);
        filter.Draw("Filter", 300.f);

        if(ImGui::BeginTable("graphs", table_width)){

            for(uint32_t i = 0; i < table_width; i++){
                ImGui::TableSetupColumn("");
            }
            ImGui::TableNextRow();
            for(auto& graph : potential_graphs){
                ImGui::TableNextColumn();

                const bool passed_filter = filter.PassFilter(graph.key.name.c_str());

                if (!passed_filter) { 
                    auto style_color = ImGui::GetStyleColorVec4(ImGuiCol_Text);
                    style_color.x *= 0.5f;
                    style_color.y *= 0.5f;
                    style_color.z *= 0.5f;
                    style_color.w *= 0.5f;
                    ImGui::PushStyleColor(ImGuiCol_Text, style_color);
                }
                if(ImGui::Button(graph.key.name.c_str())){
                    graph.value();
                }
                if (!passed_filter) { 
                    ImGui::PopStyleColor();
                }
            }

            ImGui::EndTable();
        }
    }

    void NG_Manager::DefaultFillGraphs() {
        AddPotentialDefaultGraph<Node::InstructionPackage_NG>(ImNodes::EWE::Editor::Type::InstructionPackage);
        auto obj_graph_creation_func = [&]() -> ImNodes::EWE::Editor* {
            std::string_view name = Reflect::Enum::ToString(ImNodes::EWE::Editor::Type::ObjectPackage);
            Node::InstructionPackage_NG* temp = new Node::InstructionPackage_NG();
            temp->SetPackageType(Command::InstructionPackage::Object);
            temp->name = name;
            for(auto& editor : editors){
                if(editor.editor->name == temp->name){
                    delete temp;
                    return nullptr;
                }
            }
            editors.push_back(
                TabItem{
                    .editor = static_cast<ImNodes::EWE::Editor*>(temp),
                    .closing_helper = true
                }
            );
            return temp;
        };
        AddPotentialGraph(ImNodes::EWE::Editor::Type::ObjectPackage, obj_graph_creation_func);


        AddPotentialDefaultGraph<Node::PackageRecord_NG>(ImNodes::EWE::Editor::Type::PackageRecord);
        AddPotentialDefaultGraph<Node::SubmissionTask_NG>(ImNodes::EWE::Editor::Type::SubmissionTask);
        AddPotentialDefaultGraph<Node::RenderGraph_NG>(ImNodes::EWE::Editor::Type::RenderGraph);
        AddPotentialDefaultGraph<Node::RasterPackage_NG>(ImNodes::EWE::Editor::Type::RasterPackage);
    }

    void NG_Manager::Render(){
        if(ImGui::BeginTabBar("node editors", ImGuiTabBarFlags_Reorderable)){


            if(ImGui::BeginTabItem("new*", nullptr, ImGuiTabItemFlags_NoReorder)){
                new_page.Imgui();
                ImGui::EndTabItem();
            }
            
            int current_iteration = 0;
            for(auto iter = editors.begin(); iter != editors.end();){
                ImGuiTabItemFlags tab_flags = ImGuiTabItemFlags_NoPushId;
                if(current_iteration == force_selection){
                    tab_flags |= ImGuiTabItemFlags_SetSelected;
                }

                //ImGuiTabItemFlags_UnsavedDocument
                ImNodes::EWE::Editor* editor = iter->editor;
                bool temp_bool = true;
                ImGui::PushID(current_iteration);
                if(ImGui::BeginTabItem(editor->name.c_str(), &temp_bool, tab_flags)){
                    editor->RenderNodes();
                    ImGui::EndTabItem();
                }
                ImGui::PopID();
                if(!temp_bool){
                    iter->closing_helper = false;
                }
                if(!iter->closing_helper){
                    Logger::Print("attempted to close\n");
                    ImGui::SetNextWindowFocus();
                    if(ImGui::Begin("Closing")){
                        if(ImGui::Button("close without saving")){
                            iter = editors.erase(iter);
                            ImGui::End();
                            continue;
                        }
                        ImGui::SameLine();
                        if(ImGui::Button("save and close")){

                            iter->closing_helper = true;
                        }
                        ImGui::SameLine();
                        if(ImGui::Button("cancel")){
                            //close the popup and do nothing?
                            iter->closing_helper = true;
                        }
                        ImGui::End();
                    }
                }
                iter++;
                current_iteration++;
            }

            ImGui::EndTabBar();
        }
        if(layover_storage.payload != nullptr){
            OpenGraph(layover_storage.type, layover_storage.payload);

            layover_storage.payload = nullptr;
        }
    }

    void NG_Manager::OpenGraph(ImNodes::EWE::Editor::Type type, void* payload){
        for(auto& pot_graph : new_page.potential_graphs){
            if(pot_graph.key.type == type){
                ImNodes::EWE::Editor* created = pot_graph.value();
                switch(type){   
                    case ImNodes::EWE::Editor::Type::InstructionPackage: {
                        Node::InstructionPackage_NG* temp = static_cast<Node::InstructionPackage_NG*>(created);
                        temp->packageType = Command::InstructionPackage::Type::Base;
                        temp->InitFromObject(*reinterpret_cast<Command::InstructionPackage*>(payload));
                        break;
                    }
                    case ImNodes::EWE::Editor::Type::ObjectPackage: {
                        Node::InstructionPackage_NG* temp = static_cast<Node::InstructionPackage_NG*>(created);
                        temp->packageType = Command::InstructionPackage::Type::Object;
                        temp->InitFromObject(*reinterpret_cast<Command::ObjectPackage*>(payload));
                        break;
                    }
                    case ImNodes::EWE::Editor::Type::PackageRecord:{
                        Node::PackageRecord_NG* temp = static_cast<Node::PackageRecord_NG*>(created);
                        temp->InitFromObject(*reinterpret_cast<Command::PackageRecord*>(payload));
                        break;
                    }
                    case ImNodes::EWE::Editor::Type::SubmissionTask: {
                        Node::SubmissionTask_NG* temp = static_cast<Node::SubmissionTask_NG*>(created);
                        temp->InitFromObject(*reinterpret_cast<SubmissionTask*>(payload));
                        break;
                    }
                    case ImNodes::EWE::Editor::Type::RenderGraph: {
                        Node::RenderGraph_NG* temp = static_cast<Node::RenderGraph_NG*>(created);
                        temp->InitFromObject(*reinterpret_cast<RenderGraph*>(payload));
                        break;
                    }
                    default: EWE_UNREACHABLE;
                }
                break;
            }
        }
    }

    void NG_Manager::OpenGraphLayover(ImNodes::EWE::Editor::Type type, void* payload){
        layover_storage.type = type;
        layover_storage.payload = payload;
    }

    void NG_Manager::SetOpenGraphFunc() {
        ImNodes::EWE::Editor::OpenGraph = [&](ImNodes::EWE::Editor::Type type, void* payload){
            this->OpenGraphLayover(type, payload);
        };
        //using OpenGraphFunc = void(*)(std::string_view name, void* payload);
    }

} //namespace EWE