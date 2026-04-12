#pragma once

#include "EightWinds/VulkanHeader.h"
#include "EightWinds/Buffer.h"


namespace EWE{
    struct RasterPackage;

    struct Mesh{ //also a meshlet
        struct Layout{
            struct Member{
                uint8_t size;
                uint16_t offset;
            };
            std::vector<Member> members;
        };

        //optional?
        Buffer vertex_buffer;

        std::optional<Buffer> index_buffer;


        template<typename T, std::integral U>
        requires(std::is_same_v<uint16_t, U> || std::is_same_v<uint32_t, U>)
        [[nodiscard]] explicit Mesh(const std::span<T> vertices, const std::span<U>indices);

        template<typename T>
        [[nodiscard]] explicit Mesh(const std::span<T> vertices);




        void AddToRasterPackage(RasterPackage& rasterTask);
    };
}