#include "EWEngine/Assets/Image/Loader.h"
#include "EightWinds/Command/STC_Manager.h"

#include "EWEngine/EWEngine.h"

#include "EWEngine/Graphics/SpriteSheet.h"
#include "EWEngine/Graphics/CubeMap.h"

#include "ImageProcessor.h"

#include <fstream>

namespace EWE{

    bool InitializeSingleImage(Image& img, ImageProcessor::RawImage& rawImg, Queue::Type dstQueueType, bool async_transfer) {
        img.data.extent.width = rawImg.width;
        img.data.extent.height = rawImg.height;
        img.data.extent.depth = rawImg.depth;
        img.data.format = ImageProcessor::ConvertFormatToVulkan(rawImg.format);
        EWE_ASSERT(img.data.format != VK_FORMAT_UNDEFINED);

        /* debugging
        ImageProcessor::BMP bmp{rawImg.width, rawImg.height, rawImg.format.alpha_width != 0};
        std::filesystem::path bmp_path = img_path;
        bmp_path.replace_extension(".bmp");
        bmp.Write(bmp_path.string(), rawImg.raw_data, rawImg.format);
        */

        VkImageLayout dstLayout;

        //Log::Warning("mipmaps aren't setup yet\n"); //move this print somewhere else
        switch(dstQueueType){
            case Queue::Type::Graphics: dstLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; break;
            case Queue::Type::Compute: dstLayout = VK_IMAGE_LAYOUT_GENERAL; break;
            //case Queue::Type::Transfer: //should be invalid, potentially for additional transfering? idk
            default: EWE_UNREACHABLE;

        }
        if(img.owningQueue != nullptr){
            //potentially do a assert here to make sure the queue type is the same as the owning queue?
            //or just dont set the owning queue
            //dstQueueType = engine->GetQueueType(*img.owningQueue);
        }
        else{
            img.owningQueue = &engine->transferQueue;
        }

        TransferContext<Image> transferContext{
            .images{1},
            .regions{1},
            .stagingBuffer{
                img.logicalDevice,
                rawImg.processed_data.size(),
                rawImg.processed_data.data()
            },
            .final_usage = UsageData<Image>{
                .stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                .accessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
                .layout = dstLayout
            },
            .generatingMipMaps = false
        };
        transferContext.images[0] = &img;
        transferContext.regions[0] = VkBufferImageCopy{
            .bufferOffset = 0,
            .bufferRowLength = img.data.extent.width,
            .bufferImageHeight = img.data.extent.height,
            .imageSubresource{
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = 0,
                .baseArrayLayer = 0,
                .layerCount = img.data.arrayLayers
            },
            .imageOffset{},
            .imageExtent{img.data.extent}
        };

        img.Create(
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
        EWE_ASSERT(*img.owningQueue == engine->transferQueue && "not ready for single queue uploads yet");

        if(async_transfer){
            engine->stcManager.AsyncTransfer(transferContext, dstQueueType);
        }
        else{
            engine->stcManager.InlineTransfer(transferContext);
        }
        //img.owningQueue = &engine->GetQueue(dstQueueType);
        return true;
    }

    bool InitializeImage(Image& img, std::filesystem::path const& img_path, Queue::Type dstQueueType, bool async_transfer){
        std::ifstream inFile{img_path, std::ios::binary | std::ios::ate};
        if(!inFile.is_open()){
            return false;
        }
        std::vector<std::byte> file_data{};
        {
            const std::size_t temp_size = inFile.tellg();
            file_data.resize(temp_size);
        }
        inFile.seekg(0, std::ios::beg);


        if(inFile.read(reinterpret_cast<char*>(file_data.data()), file_data.size())){

            try{
                ImageProcessor::RawImage rawImg{};
                const bool processed_ret = rawImg.ProcessData(file_data);
                if(processed_ret){
                    img.data.arrayLayers = 1;
                    img.data.mipLevels = 1;
                    img.data.layout = VK_IMAGE_LAYOUT_UNDEFINED;
                    img.data.createFlags = 0;
                    img.data.type = VK_IMAGE_TYPE_2D;
                    img.data.samples = VK_SAMPLE_COUNT_1_BIT; //multi sample
                    img.data.tiling = VK_IMAGE_TILING_OPTIMAL;
                    img.data.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
                    return InitializeSingleImage(img, rawImg, dstQueueType, async_transfer);
                }
                else{
                    return false;
                }
            }
            catch(std::exception& e){
                Log::Error("file load error : %s", e.what());
            }
        }
        else{
            return false;
        }

        return false;
    }

    bool InitializeSpriteSheet(SpriteSheet& sheet, Queue::Type dstQueueType){
        const std::filesystem::path file_name = engine->root_directory / "textures" / sheet.name;
        std::ifstream inFile{file_name, std::ios::binary | std::ios::ate};
        if(!inFile.is_open()){
            return false;
        }
        std::vector<std::byte> file_data{};
        {
            const std::size_t temp_size = inFile.tellg();
            file_data.resize(temp_size);
        }
        inFile.seekg(0, std::ios::beg);


        if(inFile.read(reinterpret_cast<char*>(file_data.data()), file_data.size())){

            try{
                ImageProcessor::RawImage rawImg{};
                const bool processed_ret = rawImg.ProcessData(file_data);
                if(processed_ret){
                    const auto format = ImageProcessor::ConvertFormatToVulkan(rawImg.format);
                    VkImageLayout dstLayout;

                    if (sheet.owningQueue != nullptr) {
                        switch(engine->GetQueueType(*sheet.owningQueue)){
                            case Queue::Type::Graphics: dstLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; break;
                            case Queue::Type::Compute: dstLayout = VK_IMAGE_LAYOUT_GENERAL; break;
                                //case Queue::Type::Transfer: //should be invalid, potentially for additional transfering? idk
                            default: EWE_UNREACHABLE;
                        }
                    }
                    else {
                        dstLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    }
                    if(sheet.owningQueue != nullptr){
                        //potentially do a assert here to make sure the queue type is the same as the owning queue?
                        //or just dont set the owning queue
                        //dstQueueType = engine->GetQueueType(*img.owningQueue);
                    }
                    else{
                        sheet.owningQueue = &engine->transferQueue;
                    }

                    sheet.width_in_sprites = rawImg.width / sheet.texel_width;
                    sheet.height_in_sprites = rawImg.height / sheet.texel_height;
                    sheet.imgs.ClearAndResize(sheet.width_in_sprites * sheet.height_in_sprites);
                    for (std::size_t i = 0; i < sheet.imgs.Size(); i++) {
                        sheet.imgs[i] = &Global::assetManager->image.ConstructInto(engine->logicalDevice);
                        sheet.imgs[i]->name = sheet.name;
                        sheet.imgs[i]->name += std::string("[") + std::to_string(i) + ']';
                    }

                    TransferContext<Image> transferContext{
                        .images{sheet.imgs.Size()},
                        .regions{sheet.imgs.Size()},
                        .stagingBuffer{engine->logicalDevice, rawImg.processed_data.size(), rawImg.processed_data.data()},
                        .final_usage{
                            .stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                            .accessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
                            .layout = dstLayout
                        },
                        .generatingMipMaps = false
                    };

                    const uint8_t bytes_per_texel = rawImg.format.BytesPerTexel();
                    const uint32_t sprite_byte_width = sheet.texel_width * bytes_per_texel;
                    const uint32_t sprite_size = sheet.texel_width * sheet.texel_height;
                    const uint32_t full_row_byte_size = sprite_size * sheet.width_in_sprites * bytes_per_texel;

                    for(std::size_t i = 0; i < sheet.imgs.Size(); i++) {
                        const uint32_t sprite_column = i % sheet.width_in_sprites;
                        const uint32_t sprite_row = i / sheet.width_in_sprites;

                        auto& img = *sheet.imgs[i];
                        transferContext.images[i] = &img;
                        transferContext.regions[i] = VkBufferImageCopy{
                            .bufferOffset = sprite_byte_width * sprite_column + sprite_row * full_row_byte_size,
                            .bufferRowLength = sheet.width_in_sprites * sheet.texel_width,
                            .bufferImageHeight = sheet.height_in_sprites * sheet.texel_height,
                            .imageSubresource{
                                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                                .mipLevel = 0,
                                .baseArrayLayer = 0,
                                .layerCount = 1
                            },
                            .imageOffset{
                                0,
                                0,
                                0
                            },
                            .imageExtent{
                                sheet.texel_width,
                                sheet.texel_height,
                                1
                            }
                        };
                        img.data.extent.width = sheet.texel_width;
                        img.data.extent.height = sheet.texel_height;
                        img.data.extent.depth = rawImg.depth;
                        img.owningQueue = sheet.owningQueue;
                        img.data.format = format;
                        img.data.arrayLayers = 1;
                        img.data.mipLevels = 1;
                        img.data.layout = VK_IMAGE_LAYOUT_UNDEFINED;
                        img.data.createFlags = 0;
                        img.data.type = VK_IMAGE_TYPE_2D;
                        img.data.samples = VK_SAMPLE_COUNT_1_BIT; //multi sample
                        img.data.tiling = VK_IMAGE_TILING_OPTIMAL;
                        img.data.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
                        img.Create(
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
                        EWE_ASSERT(*img.owningQueue == engine->transferQueue && "not ready for single queue uploads yet");
                    }
                    engine->stcManager.AsyncTransfer(transferContext, dstQueueType);
                    return true;
                }
                else{
                    return false;
                }
            }
            catch(std::exception& e){
                Log::Error("file load error : %s", e.what());
            }
        }
        else{
            return false;
        }

        return false;
    }
    bool InitializeCubeMap(CubeMap& cubemap, std::filesystem::path const& img_path, Queue::Type dstQueueType){
        Log::Error("not setup yet\n");
        return false;
    }
}