#include "EWEngine/Assets/Hash.h"
#include "EWEngine/Assets/ImageViews.h"
#include "EWEngine/Imgui/Objects.h"


namespace EWE{
namespace Asset{
    Manager<ImageView>::Manager(LogicalDevice& logicalDevice, Manager<Image>& images)
    : logicalDevice{logicalDevice},
        images{images}
    {

    }

    ImageView& Manager<ImageView>::Get(AssetHash hash){
        for(auto& kvp : association_container){
            if(kvp.key == hash){
                return *kvp.value;
            }
        }
        //doesn't already exist
        Image* img = images.Get(hash);
        ImageView& view = data_arena.AddElement(*img);
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