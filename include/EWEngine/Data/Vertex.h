#pragma once

#include "LAB/Vector.h"
#include "EWEngine/Data/Color.h"
#include <meta>



namespace EWE{
    struct VertexProperty{
        uint8_t position;
        uint8_t color;
        bool normal;
        bool tangent;
        bool uv;
        //bool bitangent?
        //uint8_t bone?
    };

    template<VertexProperty VP>
    struct Vertex {
        struct Storage;
        consteval {
            std::vector<std::meta::info> members;
            if constexpr (VP.position == 1)
                members.push_back(std::meta::data_member_spec(^^float, {.name="position"}));
            if constexpr (VP.position == 2)
                members.push_back(std::meta::data_member_spec(^^lab::vec2, {.name="position"}));
            else if constexpr(VP.position == 3)
                members.push_back(std::meta::data_member_spec(^^lab::vec3, {.name="position"}));
            else if constexpr(VP.position == 4)
                members.push_back(std::meta::data_member_spec(^^lab::vec4, {.name="position"}));

            if constexpr (VP.color == 1)
                members.push_back(std::meta::data_member_spec(^^float, {.name="color"}));
            else if constexpr (VP.color == 2)
                members.push_back(std::meta::data_member_spec(^^lab::vec2, {.name="color"}));
            else if constexpr(VP.color == 3)
                members.push_back(std::meta::data_member_spec(^^lab::vec3, {.name="color"}));
            else if constexpr(VP.color == 4)
                members.push_back(std::meta::data_member_spec(^^lab::vec4, {.name="color"}));

            if constexpr (VP.normal)    members.push_back(std::meta::data_member_spec(^^lab::vec3, {.name="normal"}));
            if constexpr (VP.tangent)   members.push_back(std::meta::data_member_spec(^^lab::vec3, {.name="tangent"}));
            if constexpr (VP.uv)        members.push_back(std::meta::data_member_spec(^^lab::vec2, {.name="uv"}));
            std::meta::define_aggregate(^^Storage, members);
        }
        Storage data;

        bool operator==(Vertex const& other) const{
            bool any_not_equal = false;
            template for (constexpr auto Member : std::define_static_array(std::meta::nonstatic_data_members_of(^^Storage, std::meta::access_context::current()))) {
                any_not_equal |= data.[:Member:] != other.data.[:Member:];
            }
            return !any_not_equal;
        }
    };
} //namespace EWE

template <EWE::VertexProperty VP>
struct std::hash<EWE::Vertex<VP>> {
    std::size_t operator()(EWE::Vertex<VP> const& vertex) const {
        std::size_t seed = 0;
        template for (constexpr auto Member : std::define_static_array(std::meta::nonstatic_data_members_of(^^typename EWE::Vertex<VP>::Storage, std::meta::access_context::current()))) {
            EWE::HashCombine(seed, std::hash<decltype(vertex.data.[:Member:])>{}(vertex.data.[:Member:]));
        }
        return seed;
    }
};