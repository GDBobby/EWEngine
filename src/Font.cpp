#include "EWEngine/Graphics/Font.h"

#include "EWEngine/EWEngine.h"
#include "EWEngine/Graphics/FontObject.h"

#include "EightWinds/Command/STC.h"


namespace EWE {

    void AsyncLoadFont(Font& font, std::filesystem::path const& _path, int pxSize) {

        auto font_load = [&, path_copy = _path] {
            Log::Debug("beginning async font load\n");
            {
                const std::string thread_name = std::string("async font load : ") + path_copy.string();
                NameCurrentThread(thread_name);
            }

            font.image.owningQueue = &engine->transferQueue;

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

            for_each_frame{
                font.pushPack.GetRef(frame).GetTextureIndex(0) = font.graphicsPkg.GetRef().dii.index;
                int index_copy = font.pushPack.GetRef(frame).GetTextureIndex(0);
                Log::Debug("adjusted idnex : %d\n", index_copy);
            };
            Log::Debug("end async font load\n");

        };
        engine->scheduler.enqueue(marl::Task{font_load});
    }

    Font::Font(std::filesystem::path const& path, int pxSize, Sampler& _sampler)
    : name{path},
        font{nullptr},
        objPkg{name},
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
        objPkg.payload.shaders[ShaderStage::Vertex] = Global::assetManager->shader.Get("textoverlay.vert.spv");
        objPkg.payload.shaders[ShaderStage::Fragment] = Global::assetManager->shader.Get("textoverlay.frag.spv");
        objPkg.payload.config.SetDefaults();
        objPkg.payload.config.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;

        label_name = name;
        ParamPack<Inst::BeginLabel> labelPack{
            .name = label_name.c_str(),
            .red = 0.f,
            .green = 1.f,
            .blue = 0.f
        };

        objPkg.paramPool.PushBack(Inst::If); //0
        objPkg.paramPool.PushBack(labelPack); //1
        objPkg.paramPool.PushBack(Inst::BeginLabel);
        objPkg.paramPool.PushBack(Inst::Push); //2
        objPkg.paramPool.PushBack(Inst::Draw); //3
        objPkg.paramPool.PushBack(Inst::EndLabel);
        objPkg.paramPool.PushBack(Inst::EndLabel); //4
        objPkg.paramPool.PushBack(Inst::EndIf); //5

        ifPack = *objPkg.paramPool.param_data[0].CastTo<ParamPack<Inst::If>>();
        pushPack = *objPkg.paramPool.param_data[3].CastTo<ParamPack<Inst::Push>>();
        drawPack = *objPkg.paramPool.param_data[4].CastTo<ParamPack<Inst::Draw>>();

        auto debugLabelPack = *objPkg.paramPool.param_data[2].CastTo<ParamPack<Inst::BeginLabel>>();
        for_each_frame{
            debugLabelPack.GetRef(frame).red = 0.f;
            debugLabelPack.GetRef(frame).green = 1.f;
            debugLabelPack.GetRef(frame).blue = 0.f;

            pushPack.GetRef(frame).buffer_count = 1;
            pushPack.GetRef(frame).texture_count = 1;
            pushPack.GetRef(frame).size = pushPack.GetRef(frame).Size();
            pushPack.GetRef(frame).GetDeviceAddress(0) = buffer[frame].deviceAddress;

            drawPack.GetRef(frame).firstInstance = 0;
            drawPack.GetRef(frame).firstVertex = 0;
            drawPack.GetRef(frame).vertexCount = 4;
            drawPack.GetRef(frame).instanceCount = 0;
        }

        for_each_frame{
            buffer[frame].name = name / std::to_string(frame);
        }
    }

    Font::~Font() {

    }

    void Font::EndRenderUpdate() {
        ifPack.GetRef(engine->frameIndex).enabled = char_instance_count > 0;
        drawPack.GetRef(engine->frameIndex).instanceCount = char_instance_count;

        auto debugLabelPack = *objPkg.paramPool.param_data[2].CastTo<ParamPack<Inst::BeginLabel>>();
        debugLabelPack.GetRef(engine->frameIndex).name = string_debugger[engine->frameIndex].c_str();


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
