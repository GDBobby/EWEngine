#pragma once

#include <meta>
#include <vector>
#include <string_view>

namespace EWE{
    struct ReflectedData{
        std::string_view name;
        std::size_t size;
        std::size_t offset;
        std::size_t alignment;

        std::vector<ReflectedData> children;
    };

    template<typename T>
    constexpr ReflectedData IterateIntoStruct(T const& obj){
        ReflectedData ret{
            .name = std::meta::identifier_of(^^T),
            .size = sizeof(T),
            .offset = 0,
            .alignment = alignof(T)
        }
    }
}