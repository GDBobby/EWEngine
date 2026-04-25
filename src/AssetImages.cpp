#include "EWEngine/Assets/Hash.h"
#include "EWEngine/Assets/Images.h"

#include "EWEngine/Assets/Image/Loader.h"
#include "EWEngine/Imgui/DragDrop.h"
#include "EWEngine/Imgui/Objects.h"
#include "EightWinds/Data/KeyValueContainer.h"
#include "EightWinds/Data/StreamHelper.h"
#include "imgui.h"
#include <vulkan/vulkan_core.h>


namespace EWE{
namespace Asset{

    template<>
    bool LoadAssetFromFile(Image* ptr_to_raw_mem, std::filesystem::path const& root_directory, std::filesystem::path const& path){
        const auto full_path = root_directory / path;
        Image& img = *std::construct_at(ptr_to_raw_mem, *Global::logicalDevice);
        img.name = path;

        auto load_img_fiber = marl::Task{[&img, full_path, root_directory, path](){
            ReadMetaFile(img, root_directory, path);
            //auto old_extent = img.data.extent;
            //auto old_format = img.data.format;
            //auto old_miplevels = img.data.mipLevels;
            img.readyForUsage = InitializeImage(img, full_path, Queue::Type::Graphics);

        }};
        Global::scheduler->enqueue(std::move(load_img_fiber));
        //do a comparison here on overwritten values, possibly
        //thats extent, format, miplevels

        return std::filesystem::exists(full_path); //poll Image::readyForUsage later
    }

    static constexpr std::size_t meta_version = 0;

    template<>
    bool WriteMetaFile(Image const& img, std::filesystem::path const& root_directory, std::filesystem::path const& file_path){
        const std::filesystem::path meta_path = (root_directory / img.name).replace_extension("mie");

        std::ofstream outFile{meta_path, std::ios::binary};
        if(!outFile.is_open()){
            outFile.open(meta_path);
            if(!outFile.is_open()){
                EWE_ASSERT(outFile.is_open());
                return false;
            }
        }
        Stream::Operator out_stream{outFile};
        std::size_t v_buffer = meta_version;
        out_stream.Process(v_buffer);
        if(v_buffer != meta_version){
            return false;
        }
        out_stream.Process(img.data);
        outFile.close();
        return true;
    }

    Image::Data GetDefaultMetaData(){
        return Image::Data{
            .arrayLayers = 1,
            .mipLevels = 1,
            .layout = VK_IMAGE_LAYOUT_UNDEFINED,
            .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
            .type = VK_IMAGE_TYPE_2D,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = VK_IMAGE_TILING_OPTIMAL
        };
    }

    template<>
    bool ReadMetaFile(Image& meta, std::filesystem::path const& root_directory, std::filesystem::path const& path){
        std::ifstream inFile{root_directory / path, std::ios::binary};
        if(!inFile.is_open()){
            inFile.open(root_directory / path);
            if(!inFile.is_open()){
                meta.data = GetDefaultMetaData();
                return false;
                //EWE_ASSERT(inFile.is_open());
            }
        }
        Stream::Operator in_stream{inFile};
        std::size_t v_buffer = meta_version;
        in_stream.Process(v_buffer);
        in_stream.Process(meta.data);
        inFile.close();
        return true;
    }
} //namespace asset
} //namespace ewe