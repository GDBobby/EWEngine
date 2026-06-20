#pragma once

#include "EightWinds/Reflect/Reflect.h"

#include "EWEngine/Data/Color.h"
#include "EWEngine/Data/Vertex.h"

#include "LAB/Vector.h"
#include "LAB/Vector/Hash.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tinyobjloader/tiny_obj_loader.h>


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