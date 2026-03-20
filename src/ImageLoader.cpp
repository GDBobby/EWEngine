#include "EWEngine/Assets/ImageLoader.h"
#include "EWEngine/Global.h"
#include <vulkan/vulkan_core.h>

#define USING_VULKAN_CONVERSION
#include "ImageProcessor.h"

#include <fstream>

namespace EWE{
    bool InitializeImage(Image& img, std::filesystem::path const& img_path){
        std::ifstream inFile{img_path, std::ios::binary | std::ios::ate};
        if(!inFile.is_open()){
            return false;
        }
        ImageProcessor::RawImage rawImg{};
        {
            const std::size_t temp_size = inFile.tellg();
            rawImg.raw_data.resize(temp_size);
        }
        inFile.seekg(0, std::ios::beg);


        if(inFile.read(reinterpret_cast<char*>(rawImg.raw_data.data()), rawImg.raw_data.size())){
            if(rawImg.ProcessData()){
                img.data.extent.width = rawImg.width;
                img.data.extent.height = rawImg.height;
                img.data.extent.depth = rawImg.depth;
                img.data.format = ImageProcessor::ConvertFormatToVulkan(rawImg.format);

                StagingBuffer* stagingBuffer = new StagingBuffer{img.logicalDevice, rawImg.raw_data.size()};

                Queue::Type dstQueueType = Queue::Graphics;
                if(img.owningQueue != nullptr){
                    dstQueueType = Global::stcManager->GetQueueType(*img.owningQueue);
                }
                else{
                    img.owningQueue = &Global::stcManager->transferQueue;
                }

                AsyncTransferContext_Image transferContext{
                    .resource{img,
                        UsageData<Image>{
                            .stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                            .accessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
                            .layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
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

                //now, upload the data to the GPU
                return true;
            }
            else{
                EWE_ASSERT(false, "fialed to read\n");
                return false;
            }
        }
        else{
            return false;
        }

        return false;
    }
}