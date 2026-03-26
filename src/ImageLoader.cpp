#include "EWEngine/Assets/Image/Loader.h"
#include "EWEngine/Global.h"
#include "EWEngine/STC_Manager.h"

#define USING_VULKAN_CONVERSION
#include "ImageProcessor.h"

#include <fstream>

namespace EWE{
    bool InitializeImage(Image& img, std::filesystem::path const& img_path, Queue::Type dstQueueType){
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

                    /* debugging
                    ImageProcessor::BMP bmp{rawImg.width, rawImg.height, rawImg.format.alpha_width != 0};
                    std::filesystem::path bmp_path = img_path;
                    bmp_path.replace_extension(".bmp");
                    bmp.Write(bmp_path.string(), rawImg.raw_data, rawImg.format);
                    */

                    StagingBuffer* stagingBuffer = new StagingBuffer{img.logicalDevice, rawImg.processed_data.size(), rawImg.processed_data.data()};

                    VkImageLayout dstLayout;

                    Logger::Print<Logger::Level::Warning>("mipmaps aren't setup yet\n");
                    switch(dstQueueType){
                        case Queue::Type::Graphics: dstLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; break;
                        case Queue::Type::Compute: dstLayout = VK_IMAGE_LAYOUT_GENERAL; break;
                        //case Queue::Type::Transfer: //should be invalid, potentially for additional transfering? idk
                        default: EWE_UNREACHABLE;

                    }
                    if(img.owningQueue != nullptr){
                        //potentially do a assert here to make sure the queue type is the same as the owning queue?
                        //or just dont set the owning queue
                        //dstQueueType = Global::stcManager->GetQueueType(*img.owningQueue);
                    }
                    else{
                        img.owningQueue = &Global::stcManager->transferQueue;
                    }

                    AsyncTransferContext_Image transferContext{
                        .resource{img,
                            UsageData<Image>{
                                .stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                                .accessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
                                .layout = dstLayout
                            }
                        },
                        .stagingBuffer = stagingBuffer,
                        .generatingMipMaps = false,
                        .image_region{
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
                            .imageExtent = img.data.extent
                        }
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
                    EWE_ASSERT(*img.owningQueue == Global::stcManager->transferQueue && "not ready for single queue uploads yet");
                    Global::stcManager->AsyncTransfer(transferContext, dstQueueType);
                    //img.owningQueue = &Global::stcManager->GetQueue(dstQueueType);
                    return true;
                }
                else{
                    return false;
                }
            }
            catch(std::exception& e){
                Logger::Print<Logger::Level::Error>("file load error : %s", e.what());
            }
        }
        else{
            return false;
        }

        return false;
    }
}