#include "EWEngine/Default/Models.h"

#include "LAB/Support/Simple.h"
#include "LAB/Vector.h"
#include "EWEngine/Global.h"
#include "EWEngine/EWEngine.h"

namespace EWE{
namespace Basic{

    Buffer& CreateBuffer(AssetHash hash, std::size_t vert_count, std::size_t vert_size){
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

        return engine->assetManager.buffer.ConstructInto(
            hash,
            vert_count * vert_size, 1,
            vmaAllocInfo
        );
    }

    Buffer& Quad(bool withUV){
        std::string grp_name = "bg_square";
        if(withUV){
            grp_name += "_uv";
        }
        const std::filesystem::path grp_path{grp_name};
        AssetHash buffer_hash = Asset::CrossPlatformPathHash(grp_path);
        
        auto* vertex_buffer = EWE::engine->assetManager.buffer.Get(buffer_hash);
        if(vertex_buffer != nullptr){
            return *vertex_buffer;
        }
        
        if(withUV){
            vertex_buffer = &CreateBuffer(buffer_hash, 4, sizeof(lab::vec2) + sizeof(lab::vec2));
            struct Vert{
                lab::vec2 pos;
                lab::vec2 uv;
            };
            Vert* pos_memory = reinterpret_cast<Vert*>(vertex_buffer->Map());
            pos_memory[0].pos = lab::vec2{-0.5f, -0.5f};
            pos_memory[1].pos = lab::vec2{0.5f, -0.5f};
            pos_memory[2].pos = lab::vec2{0.5f, 0.5f};
            pos_memory[3].pos = lab::vec2{-0.5f, 0.5f};

            pos_memory[0].uv = lab::vec2{0.f, 0.f};
            pos_memory[1].uv = lab::vec2{1.f, 0.f};
            pos_memory[2].uv = lab::vec2{1.f, 1.f};
            pos_memory[3].uv = lab::vec2{0.f, 1.f};
        }
        else{
            vertex_buffer = &CreateBuffer(buffer_hash, 4, sizeof(lab::vec2));
            lab::vec2* pos_memory = reinterpret_cast<lab::vec2*>(vertex_buffer->Map());
            pos_memory[0] = lab::vec2{-0.5f, -0.5f};
            pos_memory[1] = lab::vec2{0.5f, -0.5f};
            pos_memory[2] = lab::vec2{0.5f, 0.5f};
            pos_memory[3] = lab::vec2{-0.5f, 0.5f};
        }
        vertex_buffer->name = grp_path;

        vertex_buffer->Flush();
        vertex_buffer->Unmap();

        return *vertex_buffer;
    }
}
}