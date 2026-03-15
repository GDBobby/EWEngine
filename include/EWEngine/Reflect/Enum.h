#pragma once

#include <meta>

#include "EWEngine/Preprocessor.h"


#if EWE_IMGUI
#include "imgui.h"
#endif

namespace Reflect {

    template<typename T>
    concept IsEnum = std::is_enum_v<T>;

    template <IsEnum E>
    constexpr std::string_view enum_to_string(E value) {
        template for (constexpr auto e : std::define_static_array(std::meta::enumerators_of(^^E))) {
            if (value == [:e:]) {
                return std::meta::identifier_of(e);
            }
        }

        return "<unnamed>";
    }


    template<IsEnum E>
    struct EnumData {
        using UnderlyingType = std::underlying_type_t<E>;
        std::string_view name;
        E value;

        UnderlyingType GetUnderlyingValue() const {
            return static_cast<UnderlyingType>(value);
        }
    };

    template<IsEnum E>
    consteval auto BuildEnumData() {
        constexpr auto members = std::define_static_array(std::meta::enumerators_of(^^E));
        constexpr auto member_count = std::meta::enumerators_of(^^E).size();
        std::array<EnumData<E>, member_count> result;
        
        template for (constexpr auto i : std::views::iota(0u, member_count)) {
            result[i] = {
                .name = std::meta::identifier_of(members[i]),
                .value = std::meta::extract<E>(members[i])
            };
        }

        return result;
    }

    template<IsEnum E>
    constexpr std::array<EnumData<E>, std::meta::enumerators_of(^^E).size()> enum_data = BuildEnumData<E>();

    template<IsEnum E>
    std::size_t Enum_Members_Index(E val){
        for(std::size_t i = 0; i < enum_data<E>.size(); i++){
            if(enum_data<E>[i].value == val){
                return i;
            }
        }
        std::unreachable();
    }

#if EWE_IMGUI

    template <IsEnum E>
    void ImguiEnum_Combo(std::string_view name, E& value) {

        int index = Enum_Members_Index(value);

        const char* preview = enum_data<E>[index].name.data();
        if(ImGui::BeginCombo(name.data(), preview)) {
            for(std::size_t i = 0; i < enum_data<E>.size(); i++){
                const bool selected = index == i;
                if(ImGui::Selectable(enum_data<E>[i].name.data(), selected)){
                    value = enum_data<E>[i].value;
                }
                if(selected){
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
    }
#endif
} //namespace Reflect