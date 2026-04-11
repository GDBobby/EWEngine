#pragma once

#include "EWEngine/Imgui/ImNodes/imnodes_ewe.h"

#include <list>

namespace EWE{
    struct NG_Manager{
        std::list<ImNodes::EWE::Editor*> editors{};

        //need a new page, to open arbitrary editors

        void Render(){
            if(ImGui::BeginTabBar("node editors")){
                for(auto& editor : editors){
                    ImGui::BeginTabItem(editor->name);
                    editor->RenderNodes();
                    ImGui::EndTabItem();
                }

                ImGui::EndTabBar();
            }
        }
    }; 
}