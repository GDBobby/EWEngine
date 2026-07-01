#include "EWEngine/Imgui/ImNodes//NodeGraph_Manager.h"

#include "EWEngine/Imgui/ImNodes/imnodes_ewe.h"

#include "EWEngine/Imgui/ImNodes/Graph/RasterPackage_NG.h"
#include "EWEngine/Imgui/ImNodes/Graph/InstructionPackage_NG.h"
#include "EWEngine/Imgui/ImNodes/Graph/RenderGraph_NG.h"
#include "EWEngine/Imgui/ImNodes/Graph/InstructionPackage_NG.h"
#include "EWEngine/Imgui/ImNodes/Graph/PackageRecord_NG.h"
#include "EWEngine/Imgui/ImNodes/Graph/SubmissionTask_NG.h"
#include "EWEngine/Imgui/ImNodes/Graph/RasterPackage_NG.h"

#include "EightWinds/Command/InstructionPackage.h"
#include "imgui.h"
#include "imgui_internal.h"

#include <cmath>

namespace EWE{

    uint32_t NG_Manager::NewPage::GetTableHeight() const {
        return static_cast<uint32_t>(std::ceil(static_cast<float>(potential_graphs.size()) / 4.f));
    }

    void NG_Manager::AddPotentialGraph(std::string const& name, GraphData data){
        new_page.potential_graphs.push_back(
            name,
            data
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

                const bool passed_filter = filter.PassFilter(graph.key.c_str());

                if (!passed_filter) { 
                    auto style_color = ImGui::GetStyleColorVec4(ImGuiCol_Text);
                    style_color.x *= 0.5f;
                    style_color.y *= 0.5f;
                    style_color.z *= 0.5f;
                    style_color.w *= 0.5f;
                    ImGui::PushStyleColor(ImGuiCol_Text, style_color);
                }
                if(ImGui::Button(graph.key.c_str())){
                    graph.value.constructor();
                }
                if (!passed_filter) { 
                    ImGui::PopStyleColor();
                }
            }

            ImGui::EndTable();
        }
    }

    void NG_Manager::DefaultFillGraphs() {
        AddPotentialDefaultGraph<Node::InstructionPackage_NG>("InstructionPackage");
        auto obj_graph_creation_func = [&]() -> ImNodes::Editor* {
            for(auto& editor : editors){
                if(editor.editor->name == "Object Package"){
                    return nullptr;
                }
            }
            Node::InstructionPackage_NG* temp = new Node::InstructionPackage_NG();
            temp->SetPackageType(Command::InstructionPackage::Object);
            temp->name = "Object Package";

            editors.push_back(
                TabItem{
                    .editor = static_cast<ImNodes::Editor*>(temp),
                    .closing_helper = true
                }
            );
            return temp;
        };
        AddPotentialGraph(
            "ObjectPackage",
            GraphData{
                obj_graph_creation_func,
            
                [&](void* payload)-> ImNodes::Editor*{
                    Node::InstructionPackage_NG* temp = static_cast<Node::InstructionPackage_NG*>(obj_graph_creation_func());
                    if(temp != nullptr){
                        //i can use Function_Traits to deduce the type of the first argument
                        temp->InitFromObject(*reinterpret_cast<Command::ObjectPackage*>(payload));
                    }
                    else{
                        Log::Debug("failed to init graph from object : object package\n");
                    }
                    return temp;
                    
                }
            }
        );


        AddPotentialDefaultGraph<Node::PackageRecord_NG>("PackageRecord");
        AddPotentialDefaultGraph<Node::SubmissionTask_NG>("SubmissionTask");
        AddPotentialDefaultGraph<Node::RenderGraph_NG>("RenderGraph");
        AddPotentialDefaultGraph<Node::RasterPackage_NG>("RasterPackage");
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
                ImNodes::Editor* editor = iter->editor;
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
                    Log::Debug("attempted to close\n");
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
            OpenGraph(layover_storage.name, layover_storage.payload);

            layover_storage.payload = nullptr;
        }
    }

    void NG_Manager::OpenGraph(std::string_view name, void* payload){
        for(auto& pot_graph : new_page.potential_graphs){
            if(pot_graph.key == name){
                ImNodes::Editor* created = pot_graph.value.init_from_obj(payload);
                return;
            }
        }
    }

    void NG_Manager::OpenGraphLayover(std::string_view name, void* payload){
        layover_storage.name = name;
        layover_storage.payload = payload;
    }

    void NG_Manager::SetOpenGraphFunc() {
        ImNodes::Editor::OpenGraph = [&](std::string_view name, void* payload){
            this->OpenGraphLayover(name, payload);
        };
        //using OpenGraphFunc = void(*)(std::string_view name, void* payload);
    }

} //namespace EWE