#pragma once

#include "EWEngine/Preprocessor.h"
#include "EightWinds/VulkanHeader.h"

#include "EWEngine/Data/ReflectionInfo.h"

#include <meta>

#include <ranges>

#ifdef EWE_IMGUI

namespace EWE{


    template<std::meta::info T>
    void ImguiReflect(){
        using RefInfo = ReflectedInfo<T>;

        ImGui::Text("name : %s", RefInfo::name.data());
        //source_location
        ImGui::Text("template argument count - %zu", RefInfo::template_arguments_count);
        ImGui::Text("enumerator count : %zu", RefInfo::enumerators_count);
        ImGui::Text("member count : %zu", RefInfo::members_count);

        try{
            const std::string combined_name_tag = std::string("members##") + RefInfo::name;
            if(ImGui::TreeNode(combined_name_tag.c_str())) {
                template for(constexpr auto member : std::define_static_array(std::meta::members_of(T, std::meta::access_context::current()))){
                    constexpr auto member_name = SafeAttemptName<member>();
                    if constexpr(member_name == "null_str_v"){
                        ImGui::TextColored(ImVec4(1.f, 0.f, 0.f, 1.f), "%s", member_name.data());
                    }
                    else{
                        ImGui::Text("%s", member_name.data());
                    }
                }
                ImGui::TreePop();
            }
        }
        catch(...){

        }

        ImGui::Text("static data member count : %zu", RefInfo::static_members_count);
        ImGui::Text("nonstatic data member count : %zu", RefInfo::nonstatic_members_count);
        
        ImGui::Text("base count : %zu", RefInfo::bases_count);
        

    }

} //namespace IMGUI

#endif