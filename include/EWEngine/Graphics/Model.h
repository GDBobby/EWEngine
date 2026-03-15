#pragma once

#include "EightWinds/VulkanHeader.h"
#include "EightWinds/Buffer.h"


namespace EWE{
    struct RasterTask;

    struct Model{
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
        [[nodiscard]] explicit Model(const std::span<T> vertices, const std::span<U>indices);

        template<typename T>
        [[nodiscard]] explicit Model(const std::span<T> vertices);




        void AddToRasterTask(RasterTask& rasterTask);
    };
}