#pragma once

#include "EWEngine/Preprocessor.h"
#include "EightWinds/VulkanHeader.h"

#include "EWEngine/Data/ReflectionInfo.h"

#include <meta>

#include <ranges>
#include <filesystem>

#ifdef EWE_IMGUI


    /*
    these don't work, potentially a skill issue

    template<std::size_t N, std::size_t... Is>
    consteval void UnpackSpan(const std::span<const std::meta::info, N> span, std::index_sequence<Is...>) {
        auto invoker = []<std::meta::info M>() {
            ImguiReflectProperties<M>();
        };

        (invoker.template operator()<span[Is]>(), ...);
    }

    template<std::meta::info T>
    consteval void ProcessMemberSpan() {
        constexpr std::size_t size = std::meta::members_of(T, std::meta::access_context::current()).size();
        static constexpr auto dynamic_members = std::define_static_array(std::meta::members_of(T, std::meta::access_context::current()));

        static constexpr std::span<const std::meta::info, size> members{ 
            dynamic_members.data(), 
            size 
        };
        
        UnpackSpan(members, std::make_index_sequence<size>{});
    }
    */

namespace Reflection{

    std::string Truncate_Path(std::filesystem::path const& full_path) {
        std::filesystem::path result;

        auto r_begin = std::make_reverse_iterator(full_path.end());
        auto r_end = std::make_reverse_iterator(full_path.begin());

        for (auto it = r_begin; it != r_end; ++it) {
            if (*it == "src" || *it == "include") {
                return result.string();
            }
            
            if (result.empty()) {
                result = *it;
            } else {
                result = *it / result;
            }
        }

        return full_path.string();
    }

    template <std::meta::info Search, std::meta::info... History>
    constexpr bool reflection_imgui_is_visited = ((Search == History) || ...);

    constexpr std::string CombineStrings(std::string_view lh, std::string_view rh){
        return std::string{std::string(lh) + std::string(rh)};
    }

    template<std::meta::info T>
    void ImguiReflectProperties(){

        /*
        */

    }

    template<Reflection::MetaType MT>
    struct ImguiReflect_MT{

    };

    template<>
    struct ImguiReflect_MT<MetaType::CompleteClass> {
        template<std::meta::info T>
        static void Imgui() {
            
        }
    };

    template<std::meta::info T>
    void RenderTable(){
        using RefInfo = Reflection::ReflectedInfo<T>;
        if(ImGui::BeginTable(RefInfo::Props::name.data(), 7, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable)){
            ImGui::TableSetupColumn("name");
            ImGui::TableSetupColumn("meta type");
            ImGui::TableSetupColumn("meta enumerable");
            ImGui::TableSetupColumn("enumerable");
            ImGui::TableSetupColumn("Function Type");
            ImGui::TableSetupColumn("Template Type");
            ImGui::TableSetupColumn("source location");
            ImGui::TableHeadersRow();

            ImGui::TableNextColumn();
            ImGui::Text("%s", RefInfo::Props::name.data());

            ImGui::TableNextColumn();
            ImGui::Text("%s", Reflection::enum_to_string(RefInfo::Props::meta_type).data());

            ImGui::TableNextColumn();
            bool temp_boolean = RefInfo::Props::meta_enumerable;
            std::string temp_string = CombineStrings("##enum", RefInfo::Props::name);
            ImGui::Checkbox(temp_string.c_str(), &temp_boolean);

            ImGui::TableNextColumn();
            temp_boolean = std::meta::is_enumerable_type(T);
            temp_string = CombineStrings("##enumerable", RefInfo::Props::name);
            ImGui::Checkbox(temp_string.c_str(), &temp_boolean);

            ImGui::TableNextColumn();
            ImGui::Text("%s", Reflection::enum_to_string(RefInfo::Props::function_type).data());
            ImGui::TableNextColumn();
            ImGui::Text("%s", Reflection::enum_to_string(RefInfo::Props::template_type).data());

            ImGui::TableNextColumn();
            static constexpr auto source_loc = std::meta::source_location_of(T);
            std::string file_name = Truncate_Path(source_loc.file_name());
            
            ImGui::Text("file[%s] line[%u]", file_name.c_str(), source_loc.line());

            
            ImGui::EndTable();
        };
    }

    template<std::meta::info T, std::meta::info... History>
    void ImguiReflect_History(){
        if constexpr (reflection_imgui_is_visited<T, History...>){
            return;
        }

        using RefInfo = Reflection::ReflectedInfo<T>;

        RenderTable<T>();
        

        ImGui::Text("name : %s", RefInfo::name.data());
        //source_location
        std::string tree_name = "template args[" + std::to_string(RefInfo::template_arg_count) + "]##" + RefInfo::name;
        if(ImGui::TreeNode(tree_name.c_str())){
            template for(constexpr auto member : RefInfo::template_args) {
                static constexpr auto member_meta_type = Reflection::GetMetaType<member>();

                //specifically, overloaded functions and templates can't be used within a template for loop
                //^it currently causes a symbol error in the compiler
                if constexpr(member_meta_type == (MetaType::CompleteType) || (member_meta_type == MetaType::Namespace) || (member_meta_type == MetaType::CompleteClass)){
                    static constexpr std::string_view member_name = Reflection::SafeAttemptName<member>();
                    
                    if(ImGui::TreeNode(member_name.data())){
                        ImguiReflect_History<member, T, History...>();
                        ImGui::TreePop();
                    }
                }
                else{
                    static constexpr std::string_view member_name = Reflection::SafeAttemptName<member>();
                    ImGui::Text("%s : metatype[%s]", member_name.data(), Reflection::enum_to_string(member_meta_type).data());
                }
            }
            ImGui::TreePop();
        }
        tree_name = "enumerators[" + std::to_string(RefInfo::enumerator_count) + "]##" + RefInfo::name;
        if(ImGui::TreeNode(tree_name.c_str())){
            template for(constexpr auto member : RefInfo::enumerators){
                static constexpr auto member_meta_type = Reflection::GetMetaType<member>();

                //specifically, overloaded functions and templates can't be used within a template for loop
                //^it currently causes a symbol error in the compiler
                if constexpr(member_meta_type == (MetaType::CompleteType) || (member_meta_type == MetaType::Namespace) || (member_meta_type == MetaType::CompleteClass)){
                    static constexpr std::string_view member_name = Reflection::SafeAttemptName<member>();
                    
                    if(ImGui::TreeNode(member_name.data())){
                        ImguiReflect_History<member, T, History...>();
                        ImGui::TreePop();
                    }
                }
                else{
                    static constexpr std::string_view member_name = Reflection::SafeAttemptName<member>();
                    ImGui::Text("%s : metatype[%s]", member_name.data(), Reflection::enum_to_string(member_meta_type).data());
                }
            }
            ImGui::TreePop();
        }   

        tree_name = "members[" + std::to_string(RefInfo::members_count) + "]##" + RefInfo::name;
        if(ImGui::TreeNode(tree_name.c_str())){
            template for(constexpr auto member : RefInfo::members){
                static constexpr std::string_view member_name = Reflection::SafeAttemptName<member>();
                static constexpr auto member_meta_type = Reflection::GetMetaType<member>();
                static constexpr auto member_template_type = Reflection::GetTemplateType<member>();

                //specifically, overloaded functions and templates can't be used within a template for loop
                //^it currently causes a symbol error in the compiler
                if constexpr(member_meta_type == (MetaType::CompleteType) || (member_meta_type == MetaType::Namespace) || (member_meta_type == MetaType::CompleteClass)){
                    if(ImGui::TreeNode(member_name.data())){
                        ImguiReflect_History<member, T, History...>();
                        ImGui::TreePop();
                    }
                }
                else if constexpr(member_meta_type == MetaType::Function){
                    if(ImGui::TreeNode(member_name.data())){
                        ImguiReflect_History<member, T, History...>();
                        ImGui::TreePop();
                    }
                }
                else if constexpr(member_template_type != TemplateType::None){
                    //static constexpr std::size_t template_arg_count = std::meta::template_arguments_of(member).size();
                    ImGui::Text("%s : meta_type[%s] : template_type[%s] : arg_count[%zu]", member_name.data(), Reflection::enum_to_string(member_meta_type).data(), Reflection::enum_to_string(member_template_type).data(), 0);
                
                }
                else{
                    ImGui::Text("%s : meta_type[%s]", member_name.data(), Reflection::enum_to_string(member_meta_type).data());
                }
            }
            ImGui::TreePop();
        }   

        ImGui::Text("static data member count : %zu", RefInfo::static_member_count);
        ImGui::Text("nonstatic data member count : %zu", RefInfo::nonstatic_member_count);
        
        ImGui::Text("base count : %zu", RefInfo::base_count);
        

    }

    template<std::meta::info T>
    void ImguiReflect(){
        ImguiReflect_History<T>();
    }

} //namespace IMGUI

#endif