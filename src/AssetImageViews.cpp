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

    ImageView& Manager<ImageView>::Get(AssetHash hash){
        for(auto& kvp : association_container){
            if(kvp.key == hash){
                return *kvp.value;
            }
        }
        //doesn't already exist
        Image& img = images.Get(hash);
        while(!img.readyForUsage){
            //just relinquishing control to whatever the OS deems fit
            //could busy wait, idk if it matters
            std::this_thread::sleep_for(std::chrono::nanoseconds(1)); 
            
        }
        ImageView& view = data_arena.AddElement(img);
        association_container.push_back(hash, &view);

        return view;
    }


#if EWE_IMGUI
    void Manager<ImageView>::Imgui(){
        for(auto& kvp : association_container){
            ImguiExtension::Imgui(*kvp.value);
        }
    }
#endif
}
}