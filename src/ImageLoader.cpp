#include "EWEngine/Assets/ImageLoader.h"

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