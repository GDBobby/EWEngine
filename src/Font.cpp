#include "EWEngine/Graphics/Font.h"

#include "EWEngine/EWEngine.h"
#include "EWEngine/Graphics/FontObject.h"

#include "EightWinds/Command/STC.h"


namespace EWE {

    void AsyncLoadFont(Font& font, std::filesystem::path const& _path_ab, int pxSize) {

        auto font_load = [&, path_copy = _path_ab] {
            Log::Debug("beginning async font load\n");
            {
                const std::string thread_name = std::string("async font load : ") + path_copy.string();
                NameCurrentThread(thread_name);
            }

            font.image.owningQueue = &engine->transferQueue;
            font.image.name = font.name;

            font.image.data.extent.width = static_cast<uint32_t>(font.font->atlasW);
            font.image.data.extent.height = static_cast<uint32_t>(font.font->GetHeight());
            font.image.data.extent.depth = 1;
            font.image.data.arrayLayers = 1;
            font.image.data.mipLevels = 1;
            font.image.data.format = VK_FORMAT_R8_UNORM;
            font.image.data.layout = VK_IMAGE_LAYOUT_UNDEFINED;
            font.image.data.createFlags = 0;
            font.image.data.type = VK_IMAGE_TYPE_2D;
            font.image.data.samples = VK_SAMPLE_COUNT_1_BIT;
            font.image.data.tiling = VK_IMAGE_TILING_OPTIMAL;
            font.image.data.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

            TransferContext<Image> transferContext{
                .images{1},
                .regions{1},
                .stagingBuffer{
                    engine->logicalDevice,
                    font.font->GetDeviceSize(),
                    font.font->pixels.data()
                },
                .final_usage{
                    .stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                    .accessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
                    .layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                },
                .generatingMipMaps = false
            };
            transferContext.images[0] = &font.image;
            transferContext.regions[0] = VkBufferImageCopy{
                .bufferOffset = 0,
                .bufferRowLength = 0,
                .bufferImageHeight = font.image.data.extent.height,
                .imageSubresource{
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .mipLevel = 0,
                    .baseArrayLayer = 0,
                    .layerCount = font.image.data.arrayLayers
                },
                .imageOffset{},
                .imageExtent{
                    .width = static_cast<uint32_t>(font.font->atlasW),
                    .height = static_cast<uint32_t>(font.font->GetHeight()),
                    .depth = 1
                }
            };

            font.image.Create(
                VmaAllocationCreateInfo{
                    .flags = 0,
                    .usage = VMA_MEMORY_USAGE_AUTO,
                    .requiredFlags = 0,
                    .preferredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                    .memoryTypeBits = 0,
                    .pool = VK_NULL_HANDLE,
                    .pUserData = nullptr,
                    .priority = 1.f
                }
            );
            font.image.owningQueue = &engine->transferQueue;

            engine->stcManager.AsyncTransfer(transferContext, Queue::Type::Graphics);

            std::construct_at(&font.graphicsPkg.GetRef().view, font.image);
            std::construct_at(&font.graphicsPkg.GetRef().dii,
                font.sampler,
                font.graphicsPkg.GetRef().view,
                DescriptorType::Combined,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            );
            font.graphicsPkg.constructed = true;
            Log::Debug("end async font load\n");

        };
        engine->scheduler.enqueue(marl::Task{font_load});
    }

    Font::Font(std::filesystem::path const& path, uint8_t _pxSize, Sampler& _sampler)
    : name{path / std::to_string(_pxSize)},
        pxSize{_pxSize},
        font{nullptr},
        //objPkg{name},
        buffer{
            engine->logicalDevice,
            sizeof(Font::Vert), MAX_CHAR_COUNT,
            VmaAllocationCreateInfo{
                .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
                .usage = VMA_MEMORY_USAGE_AUTO,
                .requiredFlags = 0,
                .preferredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                .memoryTypeBits = 0,
                .pool = VK_NULL_HANDLE,
                .pUserData = nullptr,
                .priority = 1.f
            }
        },
        image{engine->logicalDevice},
        sampler{_sampler},
        graphicsPkg{},
        view{graphicsPkg.GetRef().view},
        dii{graphicsPkg.GetRef().dii}
    {
        font = new FontObject(path, pxSize); //i need this inlined?

        AsyncLoadFont(*this, path, pxSize);

        for_each_frame{
            buffer[frame].name = name / std::to_string(frame);
        }
    }

    Font::~Font() {

    }

    void Font::EndRenderUpdate() {
        //ifPack.GetRef(engine->frameIndex).enabled = char_instance_count > 0;
        //drawPack.GetRef(engine->frameIndex).instanceCount = char_instance_count;

        if (buffer[engine->frameIndex].GetMapped() != nullptr) {
            buffer[engine->frameIndex].Flush();
            buffer[engine->frameIndex].Unmap();
        }
        //auto debugLabelPack = *objPkg.paramPool.param_data[2].CastTo<ParamPack<Inst::BeginLabel>>();
        //debugLabelPack.GetRef(engine->frameIndex).name = string_debugger[engine->frameIndex].c_str();
    }

    void Font::AddText(TextStruct const& ts) {
        string_debugger[engine->frameIndex] += ts.string;
        Vert* mapped_mem = reinterpret_cast<Vert*>(buffer[engine->frameIndex].GetMapped());
        if (mapped_mem == nullptr) {
            mapped_mem = reinterpret_cast<Vert*>(buffer[engine->frameIndex].Map());
        }
        char_instance_count += font->ShapeText(
            ts,
            mapped_mem + (char_instance_count * 4)
        );
    }

    float Font::GetStringWidth(TextStruct const& ts) {
        return font->GetStringWidth(ts);
    }
}
