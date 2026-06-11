#pragma once

#include "EightWinds/VulkanHeader.h"

#include "EightWinds/Buffer.h"


namespace EWE{
namespace Basic{

    Buffer& Quad(bool withUV);
    //Buffer& Triangle(bool withUV);t

    /*
    //all in a triangle strip
    //the objectrasterconfig needs to be be handled explicitly
    //if i could return a Command::InstructionPackage* and wrap it in a ObjectPackage, that would be better
    //if using this in a performance critical scenario, use this vertex buffer as a staging buffer
    template<uint8_t CornerCount>
    requires(CornerCount > 0)
    Buffer& RegularPolygon(){

        const std::string grp_name = "grp[" + std::to_string(CornerCount) + ']';
        const std::filesystem::path grp_path{grp_name};
        auto* vertex_buffer = EWE::Global::assetManager->buffer.Get(grp_path);

        if(vertex_buffer != nullptr){
            return *vertex_buffer;
        }

        VmaAllocationCreateInfo vmaAllocInfo{
            .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
            .usage = VMA_MEMORY_USAGE_AUTO,
            .requiredFlags = 0,
            .preferredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            .memoryTypeBits = 0,
            .pool = VK_NULL_HANDLE,
            .pUserData = nullptr,
            .priority = 1.f
        };

        vertex_buffer = Global::assetManager->buffer.ConstructInto(
            Asset::CrossPlatformPathHash(grp_path),
            CornerCount * sizeof(lab::vec2), 1,
            vmaAllocInfo
        );

        *
        Command::ObjectPackage ret{};
        {
            ParamPack<Inst::Push> pushInst{
                .size = sizeof(VkDeviceAddress),
                .buffer_count = 1
            };
            pushInst.GetDeviceAddress(0) = buffer->deviceAddress;
            ret.paramPool.PushBack(pushInst);

            ParamPack<Inst::Draw> drawInst{
                .vertexCount = CornerCount,
                .instanceCount = 1,
                .firstVertex = 0,
                .firstInstance = 0
            };
            ret.paramPool.PushBack(drawInst);
        }
        *



        lab::vec2* pos_memory = reinterpret_cast<lab::vec2*>(vertex_buffer->Map());
        if constexpr(CornerCount == 1){
            //this is a singular point
            pos_memory[0] = lab::vec2{0.f};
        }
        else if constexpr(CornerCount == 2){
            pos_memory[0] = lab::vec2{-1.f, 0.f};
            pos_memory[1] = lab::vec2{1.f, 0.f};
        }
        else{
            for (uint8_t i = 0; i < CornerCount; ++i) {
                uint8_t index = i / 2;
                if (i % 2 == 1) {
                    index = CornerCount - index - 1;
                }
                const float angle = ((2.0f * std::numbers::pi_v<float> * index) / CornerCount);
                                        - (lab::GetPI_DividedBy(2.f));

                pos_memory[i].position = {
                    lab::Cos(angle),
                    lab::Sin(angle)
                };
            }
        }

        vertex_buffer->Flush();
        vertex_buffer->Unmap();

        return *vertex_buffer;
    }
        */
} //namespace Basic
} //namespace EWE