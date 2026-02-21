#pragma once

#include "EWEngine/Preprocessor.h"
#include "EightWinds/VulkanHeader.h"

#include "EWEngine/Reflection/Reflect.h"

#include <meta>

#include <ranges>
#include <filesystem>
#include <typeinfo>

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

    template<MetaType MT>
    struct ImguiReflect_MT{

    };

    template<>
    struct ImguiReflect_MT<MetaType::Class> {
        template<std::meta::info T>
        static void Imgui() {
            
        }
    };

    template<std::meta::info T>
    void RenderTable(){
        using RefInfo = ReflectedInfo<T>;
        if(ImGui::BeginTable(RefInfo::name.data(), 8, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable)){
            ImGui::TableSetupColumn("name");
            ImGui::TableSetupColumn("meta type");
            ImGui::TableSetupColumn("type type");
            ImGui::TableSetupColumn("fundamental");
            ImGui::TableSetupColumn("parent");

            ImGui::TableSetupColumn("Function Type");
            ImGui::TableSetupColumn("Template Type");
            //ImGui::TableSetupColumn("alias");
            ImGui::TableSetupColumn("source location");
            ImGui::TableHeadersRow();

            ImGui::TableNextColumn();
            ImGui::Text("%s", RefInfo::name.data());

            ImGui::TableNextColumn();
            ImGui::Text("%s", enum_to_string(RefInfo::Props::meta_type).data());

            ImGui::TableNextColumn();
            bool temp_boolean = false;
            if constexpr(std::meta::is_type(T)){
                static constexpr bool static_boolean = std::meta::is_enum_type(T);
                temp_boolean = static_boolean;
            }
            std::string temp_string = CombineStrings("##enumtype", RefInfo::name);
            ImGui::Checkbox(temp_string.c_str(), &temp_boolean);

            ImGui::TableNextColumn();
            temp_boolean = std::meta::is_special_member_function(T);
            temp_string = CombineStrings("##specmemfunc", RefInfo::name);
            ImGui::Checkbox(temp_string.c_str(), &temp_boolean);

            ImGui::TableNextColumn();
            if constexpr(std::meta::has_parent(T)){
                ImGui::Text("%s", GetName<std::meta::parent_of(T)>().data());
            }
            else {
                //ImGui::Text(""); //is this necessary?
            }

            ImGui::TableNextColumn();
            ImGui::Text("%s", enum_to_string(RefInfo::Props::function_type).data());
            ImGui::TableNextColumn();
            ImGui::Text("%s", enum_to_string(RefInfo::Props::template_type).data());

            ImGui::TableNextColumn();
            static constexpr auto source_loc = std::meta::source_location_of(T);
            std::string file_name = Truncate_Path(source_loc.file_name());
            
            ImGui::Text("file[%s] line[%u]", file_name.c_str(), source_loc.line());

            
            ImGui::EndTable();
        };
    }

    template<std::meta::info T, std::meta::info... History>
    void ImguiReflect_History(){
        RenderTable<T>();

        if constexpr (reflection_imgui_is_visited<T, History...>){
            return;
        }
        using RefInfo = ReflectedInfo<T>;
        ImGui::Text("name : %s", RefInfo::name.data());

        //if constexpr(RefInfo::Props::meta_type == MetaType::Function){
        //                            i think you need to know the return type of std::meta::extract
        //    ImGui::Text("%s", typeid(decltype(&std::meta::extract(T))).name());
        //}
        //source_location
        std::string tree_name;

        if constexpr(RefInfo::Props::template_type != TemplateType::None){
            tree_name = "template args[" + std::to_string(RefInfo::template_arg_count) + "]##" + RefInfo::name;
            if(ImGui::TreeNode(tree_name.c_str())){
                template for(constexpr auto member : RefInfo::template_args) {
                    static constexpr std::string_view member_name = GetName<member>();
                    static constexpr auto member_meta_type = GetMetaType<member>();
                    static constexpr auto member_template_type = GetTemplateType<member, member_meta_type>();
                    
                    if constexpr(
                        (member_meta_type == MetaType::Type) 
                        || (member_meta_type == MetaType::Namespace) 
                        || (member_meta_type == MetaType::Class)
                    ){
                    
                        if(ImGui::TreeNode(member_name.data())){
                            //ImguiReflect_History<member, T, History...>();
                            ImGui::TreePop();
                        }
                    }
                }
                ImGui::TreePop();
            }
        }

        if constexpr(RefInfo::Props::meta_type == MetaType::Enumerator){
            tree_name = "enumerators[" + std::to_string(RefInfo::enumerator_count) + "]##" + RefInfo::name;
            if(ImGui::TreeNode(tree_name.c_str())){
                template for(constexpr auto member : RefInfo::enumerators){
                    static constexpr std::string_view member_name = GetName<member>();
                    static constexpr auto member_meta_type = GetMetaType<member>();
                    static constexpr auto member_template_type = GetTemplateType<member, member_meta_type>();

                    if constexpr(
                        (member_template_type != TemplateType::Incomplete) 
                        && (member_meta_type != MetaType::IncompleteType) 
                        && (member_meta_type != MetaType::IncompleteClass)
                    ){
                        if(ImGui::TreeNode(member_name.data())){
                            //ImguiReflect_History<member, T, History...>();
                            ImGui::TreePop();
                        }
                    }
                }
                ImGui::TreePop();
            }  
        } 

        tree_name = "members[" + std::to_string(RefInfo::members_count) + "]##" + RefInfo::name;
        if(ImGui::TreeNode(tree_name.c_str())){
            template for(constexpr auto member : RefInfo::members){
                static constexpr std::string_view member_name = GetName<member>();
                static constexpr auto member_meta_type = GetMetaType<member>();
                static constexpr auto member_template_type = GetTemplateType<member, member_meta_type>();

                if constexpr(member_meta_type != MetaType::ClassMember) { //this would be fine but im filtering for style)
                    if constexpr(
                        (
                            (member_meta_type == MetaType::Type) 
                            || (member_meta_type == MetaType::Namespace) 
                            || (member_meta_type == MetaType::Class)
                            || (member_template_type == TemplateType::None)
                            || (member_template_type == TemplateType::Variable)
                            || (member_template_type == TemplateType::Basic)
                        ) 
                        && (member_meta_type != MetaType::Invalid) //shouldnt be reflected
                        && (member_template_type != TemplateType::Incomplete) //shouldnt be reflected
                        && (member_template_type != TemplateType::Function) //symbol clashes
                    ) {
                        if(ImGui::TreeNode(member_name.data())){
                            ImguiReflect_History<member, T, History...>();
                            ImGui::TreePop();
                        }
                    }
                    else{
                        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%s : meta[%s] : template[%s]", member_name.data(), enum_to_string(member_meta_type).data(), enum_to_string(member_template_type).data());
                        //RenderTable<member>();
                    }
                }
            }
            ImGui::TreePop();
        }   

        if constexpr(RefInfo::Props::meta_type == MetaType::Class){
            tree_name = "static data members[" + std::to_string(RefInfo::static_member_count) + "]##" + RefInfo::name;
            if(ImGui::TreeNode(tree_name.c_str())){
                template for(constexpr auto member : RefInfo::static_members){
                    static constexpr std::string_view member_name = GetName<member>();
                    static constexpr auto member_meta_type = GetMetaType<member>();
                    static constexpr auto member_template_type = GetTemplateType<member, member_meta_type>();

                    if constexpr(
                        (member_meta_type == MetaType::Type) 
                        || (member_meta_type == MetaType::Namespace) 
                        || (member_meta_type == MetaType::Class)
                        || (member_template_type == TemplateType::None)
                        || (member_template_type == TemplateType::Variable)
                        || (member_template_type == TemplateType::Basic)
                    ){
                        if(ImGui::TreeNode(member_name.data())){
                            ImguiReflect_History<member, T, History...>();
                            ImGui::TreePop();
                        }
                    }
                    else if constexpr( 
                        //these types either shouldn't be iterated, or have clashing symbols within the compiler
                        (member_meta_type != MetaType::Invalid)
                        && (member_template_type != TemplateType::Incomplete)
                        && (member_template_type != TemplateType::Function)
                    ){
                        if(ImGui::TreeNode(member_name.data())){
                            ImguiReflect_History<member, T, History...>();
                            ImGui::TreePop();
                        }
                    }
                    else{
                        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%s : meta[%s] : template[%s]", member_name.data(), enum_to_string(member_meta_type).data(), enum_to_string(member_template_type).data());
                        //RenderTable<member>();
                    }
                }
                ImGui::TreePop();
            }   
        }
        if constexpr(RefInfo::Props::meta_type == MetaType::Class || RefInfo::Props::meta_type == MetaType::Union){
            tree_name = "nonstatic data members[" + std::to_string(RefInfo::nonstatic_member_count) + "]##" + RefInfo::name;
            if(ImGui::TreeNode(tree_name.c_str())){
                template for(constexpr auto member : RefInfo::nonstatic_members){
                    static constexpr std::string_view member_name = GetName<member>();
                    static constexpr auto member_meta_type = GetMetaType<member>();
                    static constexpr auto member_template_type = GetTemplateType<member, member_meta_type>();

                    if constexpr(
                        (member_meta_type == MetaType::Type) 
                        || (member_meta_type == MetaType::Namespace) 
                        || (member_meta_type == MetaType::Class)
                        || (member_template_type == TemplateType::None)
                        || (member_template_type == TemplateType::Variable)
                        || (member_template_type == TemplateType::Basic)
                    ){
                        if(ImGui::TreeNode(member_name.data())){
                            ImguiReflect_History<member, T, History...>();
                            ImGui::TreePop();
                        }
                    }
                    else if constexpr( 
                        //these types either shouldn't be iterated, or have clashing symbols within the compiler
                        (member_meta_type != MetaType::Invalid)
                        && (member_template_type != TemplateType::Incomplete)
                        && (member_template_type != TemplateType::Function)
                    ){
                        if(ImGui::TreeNode(member_name.data())){
                            ImguiReflect_History<member, T, History...>();
                            ImGui::TreePop();
                        }
                    }
                    else{
                        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%s : meta[%s] : template[%s]", member_name.data(), enum_to_string(member_meta_type).data(), enum_to_string(member_template_type).data());
                        //RenderTable<member>();
                    }
                }
                ImGui::TreePop();
            }
        }

        if constexpr(RefInfo::Props::meta_type == MetaType::Class){
            tree_name = "bases[" + std::to_string(RefInfo::base_count) + "]##" + RefInfo::name;
            if(ImGui::TreeNode(tree_name.c_str())){
                template for(constexpr auto member : RefInfo::bases){
                    static constexpr std::string_view member_name = GetName<member>();
                    static constexpr auto member_meta_type = GetMetaType<member>();
                    static constexpr auto member_template_type = GetTemplateType<member, member_meta_type>();

                    if constexpr(
                        (member_meta_type == MetaType::Type) 
                        || (member_meta_type == MetaType::Namespace) 
                        || (member_meta_type == MetaType::Class)
                        || (member_template_type == TemplateType::None)
                        || (member_template_type == TemplateType::Variable)
                        || (member_template_type == TemplateType::Basic)
                    ){
                        if(ImGui::TreeNode(member_name.data())){
                            ImguiReflect_History<member, T, History...>();
                            ImGui::TreePop();
                        }
                    }
                    else if constexpr( 
                        //these types either shouldn't be iterated, or have clashing symbols within the compiler
                        (member_meta_type != MetaType::Invalid)
                        && (member_template_type != TemplateType::Incomplete)
                        && (member_template_type != TemplateType::Function)
                    ){
                        if(ImGui::TreeNode(member_name.data())){
                            ImguiReflect_History<member, T, History...>();
                            ImGui::TreePop();
                        }
                    }
                    else{
                        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%s : meta[%s] : template[%s]", member_name.data(), enum_to_string(member_meta_type).data(), enum_to_string(member_template_type).data());
                        //RenderTable<member>();
                    }
                }
                ImGui::TreePop();
            }  
        }
        

    }

    template<std::meta::info T>
    void ImguiReflect(){
        ImguiReflect_History<T>();
    }

} //namespace IMGUI

#endif