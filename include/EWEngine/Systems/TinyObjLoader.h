#pragma once

#include "EightWinds/Reflect/Reflect.h"

#include "EWEngine/Data/Color.h"

#include "LAB/Vector.h"
#include "LAB/Vector/Hash.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tinyobjloader/tiny_obj_loader.h>

namespace EWE{
    struct VertexProperty{
        bool position;
        bool color;
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
            if constexpr (VP.position)  members.push_back(std::meta::data_member_spec(^^lab::vec3, {.name="position"}));
            if constexpr (VP.color)     members.push_back(std::meta::data_member_spec(^^Color_RGB<float>, {.name = "color"}));
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

namespace EWE{    
    template <typename T>
    concept IndexType = std::same_as<T, uint16_t> || std::same_as<T, uint32_t>;

    template<VertexProperty VP, IndexType Index>
    struct Model{
        using Vert = Vertex<VP>;

        std::vector<Vert> vertices;
        std::vector<Index> indices;
    };

    //move Model and Vertex most likely, so that this can be included exclusively from a singular cpp file
    template<VertexProperty VP, IndexType Index>
    Model<VP, Index> LoadModelTinyObj(std::filesystem::path const& filepath){

        Model<VP, Index> ret;

        using ModelType = Model<VP,Index>;
        using Vert = ModelType::Vert;

        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filepath.string().c_str())) {
            Log::Error("%s - err :%s \n", warn.c_str(), err.c_str());
        }

        ret.vertices.clear();
        ret.indices.clear();

        std::unordered_map<Vert, uint32_t> uniqueVertices{};
        for (const auto& shape : shapes) {
            for (const auto& index : shape.mesh.indices) {
                Vert vertex;

                if (index.vertex_index >= 0) {
                    if constexpr(VP.position){
                        vertex.data.position = {
                            attrib.vertices[3 * index.vertex_index + 0],
                            attrib.vertices[3 * index.vertex_index + 1],
                            attrib.vertices[3 * index.vertex_index + 2],
                        };
                    }
                    if constexpr(VP.color){
                        vertex.data.color = {
                            attrib.colors[3 * index.vertex_index + 0],
                            attrib.colors[3 * index.vertex_index + 1],
                            attrib.colors[3 * index.vertex_index + 2],
                        };
                    }
                }

                if constexpr(VP.normal){
                    if (index.normal_index >= 0) {
                        vertex.data.normal = {
                            attrib.normals[3 * index.normal_index + 0],
                            attrib.normals[3 * index.normal_index + 1],
                            attrib.normals[3 * index.normal_index + 2],
                        };
                    }
                }

                if constexpr(VP.uv){
                    if (index.texcoord_index >= 0) {
                        vertex.data.uv = {
                            attrib.texcoords[2 * index.texcoord_index + 0],
                            attrib.texcoords[2 * index.texcoord_index + 1],
                        };
                    }
                    else {
                        vertex.data.uv = {
                            0.f,
                            0.f
                        };
                    }
                }

                auto vertFind = uniqueVertices.find(vertex);
                if (vertFind == uniqueVertices.end()) {
                    uniqueVertices.try_emplace(vertex, static_cast<uint32_t>(ret.vertices.size()));
                    ret.indices.push_back(static_cast<uint32_t>(ret.vertices.size()));
                    ret.vertices.push_back(vertex);
                }
                else {
                    ret.indices.push_back(vertFind->second);
                }
            }
        }
        return ret;
    }
} //namespace EWE