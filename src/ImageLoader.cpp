#include "EWEngine/Assets/Image/Loader.h"
#include "EightWinds/Command/STC_Manager.h"

#include "EWEngine/EWEngine.h"

#include "EWEngine/Graphics/SpriteSheet.h"
#include "EWEngine/Graphics/CubeMap.h"
#include <vulkan/vulkan_core.h>

#define USING_VULKAN_CONVERSION
#include "ImageProcessor.h"

#include <fstream>

namespace EWE{

    bool InitializeImage(Image& img, std::filesystem::path const& img_path, Queue::Type dstQueueType, bool async_transfer){
        std::ifstream inFile{img_path, std::ios::binary | std::ios::ate};
        if(!inFile.is_open()){
            return false;
        }
        ImageProcessor::RawImage rawImg{};
        std::vector<std::byte> file_data{};
        {
            const std::size_t temp_size = inFile.tellg();
            file_data.resize(temp_size);
        }
        inFile.seekg(0, std::ios::beg);


        if(inFile.read(reinterpret_cast<char*>(file_data.data()), file_data.size())){

            try{
                const bool processed_ret = rawImg.ProcessData(file_data);
                if(processed_ret){
                    img.data.extent.width = rawImg.width;
                    img.data.extent.height = rawImg.height;
                    img.data.extent.depth = rawImg.depth;
                    img.data.format = ImageProcessor::ConvertFormatToVulkan(rawImg.format);
                    img.data.arrayLayers = 1;
                    img.data.mipLevels = 1;
                    img.data.layout = VK_IMAGE_LAYOUT_UNDEFINED;
                    img.data.createFlags = 0;
                    img.data.type = VK_IMAGE_TYPE_2D;
                    img.data.samples = VK_SAMPLE_COUNT_1_BIT; //multi sample
                    img.data.tiling = VK_IMAGE_TILING_OPTIMAL;
                    img.data.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

                    /* debugging
                    ImageProcessor::BMP bmp{rawImg.width, rawImg.height, rawImg.format.alpha_width != 0};
                    std::filesystem::path bmp_path = img_path;
                    bmp_path.replace_extension(".bmp");
                    bmp.Write(bmp_path.string(), rawImg.raw_data, rawImg.format);
                    */

                    VkImageLayout dstLayout;

                    Log::Warning("mipmaps aren't setup yet\n");
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
                        .stagingBuffer{img.logicalDevice, rawImg.processed_data.size(), rawImg.processed_data.data()},
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
                        .bufferRowLength = 0,
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


    bool InitializeSpriteSheet(SpriteSheet& sheet, std::filesystem::path const& img_path, Queue::Type dstQueueType, uint16_t texel_width, uint16_t texel_height){
        std::ifstream inFile{img_path, std::ios::binary | std::ios::ate};
        if(!inFile.is_open()){
            return false;
        }
        ImageProcessor::RawImage rawImg{};
        std::vector<std::byte> file_data{};
        {
            const std::size_t temp_size = inFile.tellg();
            file_data.resize(temp_size);
        }
        inFile.seekg(0, std::ios::beg);


        if(inFile.read(reinterpret_cast<char*>(file_data.data()), file_data.size())){

            try{
                const bool processed_ret = rawImg.ProcessData(file_data);
                if(processed_ret){
                    auto format = ImageProcessor::ConvertFormatToVulkan(rawImg.format);
                    VkImageLayout dstLayout;

                    switch(dstQueueType){
                        case Queue::Type::Graphics: dstLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; break;
                        case Queue::Type::Compute: dstLayout = VK_IMAGE_LAYOUT_GENERAL; break;
                        //case Queue::Type::Transfer: //should be invalid, potentially for additional transfering? idk
                        default: EWE_UNREACHABLE;

                    }
                    if(sheet.owningQueue != nullptr){
                        //potentially do a assert here to make sure the queue type is the same as the owning queue?
                        //or just dont set the owning queue
                        //dstQueueType = engine->GetQueueType(*img.owningQueue);
                    }
                    else{
                        sheet.owningQueue = &engine->transferQueue;
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
                    for(std::size_t i = 0; i <sheet.imgs.Size(); i++){
                        transferContext.images[i] = sheet.imgs[i]; 
                        Log::Error("fix this\n");
                        transferContext.regions[i] = VkBufferImageCopy{};//calc region
                        sheet.imgs[i]->data.extent.width = texel_width;
                        sheet.imgs[i]->data.extent.height = texel_height;
                        sheet.imgs[i]->data.extent.depth = rawImg.depth;
                        sheet.imgs[i]->data.format = format;
                        sheet.imgs[i]->owningQueue = sheet.owningQueue;
                        sheet.imgs[i]->Create(
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
                        EWE_ASSERT(*sheet.imgs[i]->owningQueue == engine->transferQueue && "not ready for single queue uploads yet");
                    }

                    
                    engine->stcManager.AsyncTransfer(transferContext, dstQueueType);
                    //img.owningQueue = &engine->GetQueue(dstQueueType);
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