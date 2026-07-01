#pragma once

#include "EWEngine/Imgui/ImNodes/imnodes_ewe.h"

#include "EightWinds/Data/KeyValueContainer.h"
#include "EightWinds/Reflect/Reflect.h"
#include "imgui.h"

#include <list>
#include <functional>

namespace EWE{
    struct NG_Manager{

        struct GraphData{
            std::function<ImNodes::Editor*()> constructor;
            std::function<ImNodes::Editor*(void* payload)> init_from_obj;
        };
        
        struct NewPage{
            KeyValueContainer<std::string, GraphData> potential_graphs;
            ImGuiTextFilter filter;
            uint32_t table_width = 4;

            uint32_t GetTableHeight() const;

            void Imgui();
        };

        struct TabItem{
            ImNodes::Editor* editor;
            bool closing_helper;
        };
        
        std::list<TabItem> editors{};

        //need a new page, to open arbitrary editors

        NewPage new_page;
        std::size_t new_page_index = 0;

        int force_selection = -1;

        //come back to this in a moment
        void AddPotentialGraph(std::string const& name, GraphData data);
        template<typename T>
        void AddPotentialDefaultGraph(std::string_view name){
            auto default_creation_func = [&]() -> ImNodes::Editor* {
                for(auto& editor : editors){
                    if(editor.editor->name == name){
                        return nullptr;
                    }
                }
                T* temp = new T();
                temp->name = std::meta::identifier_of(^^T);
                editors.push_back(
                    TabItem{
                        .editor = static_cast<ImNodes::Editor*>(temp),
                        .closing_helper = true
                    }
                );
                return temp;
            };
            auto default_init_func = [&](void* payload)-> ImNodes::Editor*{
                T* temp = static_cast<T*>(default_creation_func());
                if(temp != nullptr){
                    //i can use Function_Traits to deduce the type of the first argument
                    temp->InitFromObject(payload);
                }
                else{
                    Log::Debug("failed to init graph from object : %s\n", std::meta::identifier_of(^^T).data());
                }
                return temp;
                
            };
            AddPotentialGraph(
                name.data(), 
                GraphData{
                    default_creation_func,
                    default_init_func
                }
            );
        }


        struct OpenGraph_LayoverStorage{
            std::string name;
            void* payload;
        };
        OpenGraph_LayoverStorage layover_storage;

        void OpenGraphLayover(std::string_view name, void* payload);

        void DefaultFillGraphs();

        void Render();

        void OpenGraph(std::string_view name, void* payload);
        void SetOpenGraphFunc();
    }; 
}