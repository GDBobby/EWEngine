#pragma once

#include "EWEngine/Imgui/ImNodes/imnodes_ewe.h"

#include "EightWinds/Data/KeyValueContainer.h"
#include "EightWinds/Reflect/Reflect.h"
#include "imgui.h"

#include <list>
#include <functional>

namespace EWE{
    struct NG_Manager{

        struct GraphIdentifier{
            ImNodes::EWE::Editor::Type type;
            std::string name;
        };
        
        struct NewPage{
            KeyValueContainer<GraphIdentifier, std::function<ImNodes::EWE::Editor*()>> potential_graphs;

            ImGuiTextFilter filter;

            uint32_t table_width = 4;

            uint32_t GetTableHeight() const;

            void Imgui();
        };

        struct TabItem{
            ImNodes::EWE::Editor* editor;
            bool closing_helper;
        };
        
        std::list<TabItem> editors{};

        //need a new page, to open arbitrary editors

        NewPage new_page;
        std::size_t new_page_index = 0;

        int force_selection = -1;

        //come back to this in a moment
        void AddPotentialGraph(ImNodes::EWE::Editor::Type type, std::function<ImNodes::EWE::Editor*()> creation_func);
        template<typename T>
        void AddPotentialDefaultGraph(ImNodes::EWE::Editor::Type type){
            auto default_creation_func = [&, type]() -> ImNodes::EWE::Editor* {
                std::string_view name = Reflect::Enum::ToString(type);
                T* temp = new T();
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
            AddPotentialGraph(type, default_creation_func);
        }


        struct OpenGraph_LayoverStorage{
            ImNodes::EWE::Editor::Type type;
            void* payload;
        };
        OpenGraph_LayoverStorage layover_storage;

        void OpenGraphLayover(ImNodes::EWE::Editor::Type type, void* payload);

        void DefaultFillGraphs();

        void Render();

        void OpenGraph(ImNodes::EWE::Editor::Type type, void* payload);
        void SetOpenGraphFunc();
    }; 
}