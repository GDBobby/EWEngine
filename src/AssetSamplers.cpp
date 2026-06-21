#include "EWEngine/Assets/Samplers.h"

#include "EWEngine/EWEngine.h"

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
        std::unique_lock<std::mutex> lock{mut};
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
        std::unique_lock<std::mutex> lock{mut};
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
        std::unique_lock<std::mutex> lock{mut};
        for(auto& kvp : association_container){
            if(kvp.key == condensed_val){
                return *kvp.value;
            }
        }

        auto samplerInfo = Sampler::Expand(condensed_val);
        auto& ret = data_arena.AddElement(engine->logicalDevice, samplerInfo);
        association_container.push_back(condensed_val, &ret);
        return ret;
    }

    Sampler& Manager<Sampler>::Get(VkSamplerCreateInfo const& samplerInfo){
        uint64_t condensed = Sampler::Condense(samplerInfo);
        return Get(condensed);
    }


#ifdef EWE_IMGUI

    void Manager<Sampler>::Imgui(){
        std::unique_lock<std::mutex> lock{mut};
        for(auto& sa : data_arena){
            if(ImGui::TreeNode(std::to_string(reinterpret_cast<std::size_t>(&sa)).c_str())){
                DragDropPtr::Source<Sampler>(sa);

                VkSamplerCreateInfo samplerInfo{sa.info};
                ImguiExtension::Imgui(samplerInfo);
                ImGui::TreePop();
            }
            else{
                DragDropPtr::Source<Sampler>(sa);
            }
        }
    }
#endif
} //namespace Asset
} //namespace EWE