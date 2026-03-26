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
    Manager<Image>::Manager(LogicalDevice& _logicalDevice, std::filesystem::path const& root_path)
    : logicalDevice{_logicalDevice},
        image_files{root_path, std::vector<std::string>{".png", ".jpg", ".bmp", ".dds"}}//,
        //meta_files{root_path, std::vector<std::string>{".mie"}}
    {

    }


    static constexpr std::size_t meta_version = 0;

    void Manager<Image>::UpdateMetaFile(AssetHash hash, Image& img){
        const std::filesystem::path meta_path = (image_files.root_directory / img.name).replace_extension("mie");

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
    void Manager<Image>::UpdateMetaFile(AssetHash hash){
        auto found = association_container.find(hash);
        EWE_ASSERT(found != association_container.end());
        auto& img = *found->value;
        UpdateMetaFile(hash, img);
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

    Image::Data ReadMetaFile(std::filesystem::path const& path){
        std::ifstream inFile{path, std::ios::binary};
        if(!inFile.is_open()){
            inFile.open(path);
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

    Image::Data LoadMetaData(std::filesystem::path const& root_directory, KeyValuePair<AssetHash, std::filesystem::path> const& img_kvp){
        auto meta_file_path = img_kvp.value;
        meta_file_path.replace_extension(".mie");
        meta_file_path = root_directory / meta_file_path;
        return ReadMetaFile(meta_file_path);
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
            auto image_path_hash_data = image_files.hashed_path.at(hash);
            auto const& fs_path = image_path_hash_data.value;
            Image& img = data_arena.AddElement(logicalDevice);
            img.name = fs_path;
            association_container.push_back(hash, &img);

            auto full_img_load_path = image_files.root_directory / fs_path;

            auto load_img_fiber = marl::Task{[&img, img_kvp = image_path_hash_data, full_img_load_path, root_dir = image_files.root_directory](){
                img.data = LoadMetaData(root_dir, img_kvp);
                auto old_extent = img.data.extent;
                auto old_format = img.data.format;
                auto old_miplevels = img.data.mipLevels;
                InitializeImage(img, full_img_load_path, Queue::Type::Graphics);

            }};
            Global::scheduler->enqueue(std::move(load_img_fiber));

            //do a comparison here on overwritten values, possibly
            //thats extent, format, miplevels


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
                    DragDropPtr::Source<Image>(*found->value);
                    ImguiExtension::Imgui(*found->value);
                    if(ImGui::Button("update meta values")){
                        UpdateMetaFile(kvp.key, *found->value);
                    }
                    ImGui::TreePop();
                }
                else{
                    DragDropPtr::Source<Image>(*found->value);
                }
                ImGui::PopID();
            }
        }
    }
#endif
} //namespace asset
} //namespace ewe