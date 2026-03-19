#include "EWEngine/Assets/Hash.h"
#include "EWEngine/Assets/Images.h"

#include "EWEngine/Assets/ImageLoader.h"
#include "EWEngine/Imgui/Framework_Imgui.h"
#include "EightWinds/Data/StreamHelper.h"
#include <vulkan/vulkan_core.h>

namespace EWE{
namespace Asset{
    Manager<Image>::Manager(LogicalDevice& logicalDevice, std::filesystem::path const& root_path)
    : logicalDevice{logicalDevice},
        image_files{root_path, std::vector<std::string>{".png", ".jpg"}},
        meta_files{root_path, std::vector<std::string>{".mie"}}
    {

    }


    static constexpr std::size_t meta_version = 0;

    void Manager<Image>::UpdateMetaFile(AssetHash hash, Image& img){
        const std::filesystem::path meta_path = (meta_files.root_directory / img.name).replace_extension("mie");

        std::ofstream outFile{meta_path, std::ios::binary};
        if(!outFile.is_open()){
            outFile.open(meta_path);
            if(!outFile.is_open()){
                EWE_ASSERT(outFile.is_open());
            }
        }
        Stream::Operator out_stream{outFile};
        std::size_t v_buffer = meta_version;
        out_stream.Process(v_buffer);
        out_stream.Process(img.data);
        outFile.close();
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

    Image::Data Manager<Image>::LoadMetaData(AssetHash hash){
        auto found = meta_files.hashed_path.find(hash);
        if(found == meta_files.hashed_path.end()){
            return GetDefaultMetaData();
        }
        //EWE_ASSERT(found != meta_files.hashed_path.end());
        const std::filesystem::path meta_path = (meta_files.root_directory / found->value);

        std::ifstream inFile{meta_path, std::ios::binary};
        if(!inFile.is_open()){
            inFile.open(meta_path);
            if(!inFile.is_open()){
                return GetDefaultMetaData();
                //EWE_ASSERT(inFile.is_open());
            }
        }
        Stream::Operator in_stream{inFile};
        std::size_t v_buffer = meta_version;
        in_stream.Process(v_buffer);
        Image::Data img_data;
        in_stream.Process(img_data);
        inFile.close();
        return img_data;
    }
    

    void Manager<Image>::UpdateMetaFile(AssetHash hash){
        auto found = association_container.find(hash);
        EWE_ASSERT(found != association_container.end());
        auto& img = *found->value;
        UpdateMetaFile(hash, img);
    }

    void Manager<Image>::Destroy(AssetHash hash){
        Image* img = Get(hash);
        EWE_ASSERT(img != nullptr);
        data_arena.DestroyElement(img);
        association_container.Remove(hash);
    }
    void Manager<Image>::Destroy(Image* img){
        //do I hash it first? idk
        AssetHash hash = GetHash(*img);
        Destroy(hash);
    }
    Image* Manager<Image>::Get(AssetHash hash){
        auto iter = association_container.find(hash);
        if(iter != association_container.end()){
            return iter->value;
        }
        else{
            auto const& fs_path = image_files.hashed_path.at(hash).value;
            Image& img = data_arena.AddElement(logicalDevice);

            InitializeImage(img, image_files.root_directory / fs_path);

            auto old_extent = img.data.extent;
            auto old_format = img.data.format;
            //do a comparison here on values that will get overwritten, possibly
            //thats extent, format, miplevels
            img.data = LoadMetaData(hash);

            img.data.extent = old_extent;
            img.data.format = old_format;

            img.name = fs_path;

            association_container.push_back(hash, &img);
            return &img;
        }
    }
    Image* Manager<Image>::Get(std::string_view name){
        return Get(CrossPlatformPathHash(name));
    }


#ifdef EWE_IMGUI
    void Manager<Image>::Imgui(){
        //filesystem.Imgui();
        for(auto& kvp : image_files.hashed_path){
            auto found = association_container.find(kvp.key);
            if(found == association_container.end()){
                if(ImGui::Button(kvp.value.string().c_str())){
                    Get(kvp.key);
                }
            }
            else{
                ImGui::PushID(kvp.key);
                if(ImGui::TreeNode(kvp.value.string().c_str())){
                    ImguiExtension::Imgui(*found->value);
                    if(ImGui::Button("update meta values")){
                        UpdateMetaFile(kvp.key, *found->value);
                    }
                    ImGui::TreePop();
                }
                ImGui::PopID();
            }
        }
    }
#endif
} //namespace asset
} //namespace ewe