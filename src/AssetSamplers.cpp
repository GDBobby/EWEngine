#include "EWEngine/Assets/Samplers.h"

#include "EWEngine/Imgui/Objects.h"
#include "EWEngine/Imgui/DragDrop.h"
#include "imgui.h"

namespace EWE{
namespace Asset{
    Manager<Sampler>::Manager() 
    : data_arena{}, 
    association_container{}
    {}

    void Manager<Sampler>::Destroy(Sampler::CondensedType condensed_val){
        for(auto iter = association_container.begin(); iter != association_container.end(); iter++){
            if(iter->key == condensed_val){
                data_arena.DestroyElement(iter->value);
                association_container.Remove(iter);
                return;
            }
        }
        EWE_UNREACHABLE;
    }
    void Manager<Sampler>::Destroy(Sampler* sampler){
        for(auto iter = association_container.begin(); iter != association_container.end(); iter++){
            if(iter->value == sampler){ //pointer comparison is best here
                data_arena.DestroyElement(sampler);
                association_container.Remove(iter);
                return;
            }
        }
        EWE_UNREACHABLE;
    }

    Sampler& Manager<Sampler>::Get(Sampler::CondensedType condensed_val){
        for(auto& kvp : association_container){
            if(kvp.key == condensed_val){
                return *kvp.value;
            }
        }

        auto samplerInfo = Sampler::Expand(condensed_val);
        auto& ret = data_arena.AddElement(*Global::logicalDevice, samplerInfo);
        association_container.push_back(condensed_val, &ret);
        return ret;
    }

    Sampler& Manager<Sampler>::Get(VkSamplerCreateInfo const& samplerInfo){
        uint64_t condensed = Sampler::Condense(samplerInfo);
        return Get(condensed);
    }


#ifdef EWE_IMGUI

    void Manager<Sampler>::Imgui(){
        for(auto& kvp : association_container){
            if(ImGui::TreeNode(std::to_string(kvp.key).c_str())){
                DragDropPtr::Source<Sampler>(*kvp.value);

                VkSamplerCreateInfo samplerInfo{kvp.value->info};
                ImguiExtension::Imgui(samplerInfo);
                ImGui::TreePop();
            }
            else{
                DragDropPtr::Source<Sampler>(*kvp.value);
            }
        }
    }
#endif
} //namespace Asset
} //namespace EWE