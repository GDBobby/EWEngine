#include "EWEngine/Assets/Hash.h"
#include "EWEngine/Assets/Images.h"

namespace EWE{
namespace Asset{
    Manager<Image>::Manager(LogicalDevice& logicalDevice, std::filesystem::path const& root_path)
    : logicalDevice{logicalDevice},
        filesystem{root_path, std::vector<std::string>{".png", ".jpg"}}
    {

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
            auto fs_path = filesystem.hashed_path.at(hash);
            Image& img = data_arena.AddElement(logicalDevice);
            association_container.push_back(hash, &img);
            return &img;
        }
    }
    Image* Manager<Image>::Get(std::string_view name){
        return Get(CrossPlatformPathHash(name));
    }


#ifdef EWE_IMGUI
    void Manager<Image>::Imgui(){
        filesystem.Imgui();
    }
#endif
} //namespace asset
} //namespace ewe