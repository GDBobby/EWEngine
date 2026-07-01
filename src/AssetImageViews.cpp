#include "EWEngine/Assets/Hash.h"
#include "EWEngine/Assets/ImageViews.h"
#include "EWEngine/Imgui/Objects.h"
#include <chrono>
#include <thread>


namespace EWE{
namespace Asset{
    Manager<ImageView>::Manager(std::filesystem::path const& root_dir, Manager<Image>& _images)
    : images{_images}
    {

    }


    void Manager<ImageView>::Destroy(AssetHash hash){
        ImageView* res = Get(hash);
        if(res == nullptr){
            Log::Warning("attempting to delete non-existing object\n");
            return;
        }
        data_arena.DestroyElement(res);
        association_container.Remove(hash);
    }
    void Manager<ImageView>::Destroy(ImageView& res){
        //by routing it to hash, the object is looked up, ensuring it exists
        AssetHash hash = GetHash(res); 
        Destroy(hash);
    }

    ImageView* Manager<ImageView>::Get(AssetHash hash){
        std::unique_lock<std::mutex> lock{mut};
        for(auto& kvp : association_container){
            if(kvp.key == hash){
                return kvp.value;
            }
        }
        //doesn't already exist
        Image* img = images.Get(hash);
        if(img == nullptr){
            return nullptr;
        }
        while(!img->readyForUsage){
            //just relinquishing control to whatever the OS deems fit
            //could busy wait, idk if it matters
            std::this_thread::sleep_for(std::chrono::nanoseconds(1)); 
            
        }
        ImageView& view = data_arena.AddElement(*img);
        association_container.push_back(hash, &view);

        return &view;
    }
    ImageView* Manager<ImageView>::Get(Image& img){
        std::unique_lock<std::mutex> lock{mut};
        AssetHash image_hash = GetHash(img);
        ImageView& view = data_arena.AddElement(img);
        association_container.push_back(image_hash, &view);

        return &view;
    }


#if EWE_IMGUI
    void Manager<ImageView>::Imgui(){
        std::unique_lock<std::mutex> lock{mut};
        for(auto& kvp : association_container){
            ImguiExtension::Imgui(*kvp.value);
        }
    }
#endif
}
}