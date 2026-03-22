#include "EWEngine/Assets/DII.h"

#include "EWEngine/Assets/Hash.h"
#include "EWEngine/Imgui/Objects.h"
#include "EWEngine/Imgui/DragDrop.h"
#include "EightWinds/Reflect/Enum.h"
#include "EightWinds/DescriptorImageInfo.h"
#include "EightWinds/Sampler.h"

#include "EWEngine/Global.h"

#if EWE_IMGUI
    #include "imgui.h"
    #include "backends/imgui_impl_vulkan.h"
#endif

namespace EWE{
namespace Asset{

    
    
#if EWE_IMGUI
    ImTextureRef GetTextureRef(DescriptorImageInfo& dii){
        if(!dii.view.image.owningQueue){
            return ImTextureID_Invalid;
        }
        if(*dii.view.image.owningQueue != Global::stcManager->GetQueue(Queue::Graphics)){
            return ImTextureID_Invalid;
        }
        if(dii.type != DescriptorType::Combined){
            return ImTextureID_Invalid;
        }
        if(dii.imageInfo.imageLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL || dii.imageInfo.imageLayout == VK_IMAGE_LAYOUT_GENERAL){
            return ImGui_ImplVulkan_AddTexture(dii.sampler != nullptr ? dii.sampler->sampler : VK_NULL_HANDLE, dii.view.view, dii.imageInfo.imageLayout);
        }
        else{
            return ImTextureID_Invalid;
        }

    }
#endif

    Manager<DescriptorImageInfo>::Manager(
        LogicalDevice& logicalDevice,
        Manager<Sampler>& samplers,
        Manager<ImageView>& views,
        std::filesystem::path const& root_path
    )
    : logicalDevice{logicalDevice},
        samplers{samplers},
        views{views},
        filesystem{root_path, std::vector<std::string>{".dii"}}
    {

    }

    static constexpr std::size_t dii_version = 0;

    void Manager<DescriptorImageInfo>::Read(std::filesystem::path const& file_name){

        std::ifstream stream{file_name, std::ios::binary};
        EWE_ASSERT(stream.is_open());

        Stream::Operator streamHandler{stream};

        std::size_t buffer;
        streamHandler.Process(buffer);
        EWE_ASSERT(buffer == dii_version);
        //this assumes that only the default view can be used
        //otherwise, i would need to store a handle for the view
        AssetHash view_hash;
        streamHandler.Process(view_hash);

        ImageView& view = views.Get(view_hash);

        Sampler* sampler = nullptr;
        uint8_t temp_buffer;
        streamHandler.Process(temp_buffer);
        if(temp_buffer == 1){
            //if we're here, and it's nullptr, we're reading. otherwise, writing
            uint64_t sampler_condensed;
            streamHandler.Process(sampler_condensed);
            sampler = &samplers.Get(sampler_condensed);
        }

        DescriptorType type;
        streamHandler.Process(type);

        VkImageLayout layout;
        streamHandler.Process(layout);

        if(sampler != nullptr){
            data_arena.AddElement(*sampler, view, type, layout);
        }
        else{
            data_arena.AddElement(view, type, layout);
        }
    }

    void Manager<DescriptorImageInfo>::Write(DescriptorImageInfo const& dii, std::filesystem::path const& file_name){

        std::ofstream stream{file_name, std::ios::binary};
        EWE_ASSERT(stream.is_open());

        Stream::Operator streamHandler{stream};

        std::size_t buffer = dii_version;
        streamHandler.Process(buffer);

        //this assumes that only the default view can be used
        //otherwise, i would need to store a handle for the view
        auto hash = views.GetHash(dii.view);
        streamHandler.Process(hash);

        uint8_t temp_buffer = dii.sampler != nullptr;
        streamHandler.Process(temp_buffer);
        if(dii.sampler != nullptr){
            uint64_t sample_condensed = Sampler::Condense(dii.sampler->info);
            streamHandler.Process(sample_condensed);
        }

        streamHandler.Process(dii.type);
        streamHandler.Process(dii.imageInfo.imageLayout);
    }

    DescriptorImageInfo& Manager<DescriptorImageInfo>::Get(AssetHash hash){
        auto found = association_container.find(hash);
        if(found == association_container.end()){
            //look into the file system
        }
        else{
            return *found->value;
        }
    }
    DescriptorImageInfo& Manager<DescriptorImageInfo>::Get(std::string_view name){
        auto hash = CrossPlatformPathHash(name);
        for(auto kvp : association_container){
            if(kvp.value->name == name){
                return *kvp.value;
            }
        }
        //wasnt found, break into the file system
    }

    DescriptorImageInfo& Manager<DescriptorImageInfo>::Get(Creation params){
        //search for a match, potentially, and return the existing one
        DescriptorImageInfo* ret = nullptr;
        if(params.sampler){
            ret = &data_arena.AddElement(*params.sampler, *params.view, params.type, params.layout);
            ret->name = params.view->image.name + std::to_string(Sampler::Condense(params.sampler->info)) + ".dii";
        }
        else{
            ret = &data_arena.AddElement(*params.view, params.type, params.layout);
            ret->name = params.view->image.name + ".dii";
        }
        association_container.push_back(CrossPlatformPathHash(ret->name), ret);
#if EWE_IMGUI
        auto tex_ref = GetTextureRef(*ret);
        if(tex_ref != ImTextureID_Invalid){
            imgui_texture_refs.push_back(ret, tex_ref);
        }
#endif
        
        return *ret;
    }



#ifdef EWE_IMGUI
    void Manager<DescriptorImageInfo>::Imgui(){
        //filesystem.Imgui();
        ImGui::Checkbox("show creation", &showCreation);
        if(showCreation){
            if(creation_params.sampler != nullptr){
                ImGui::Text("sampler : %zu", Sampler::Condense(creation_params.sampler->info));
            }
            else{
                ImGui::Text("no sampler");
            }
            if(DragDropPtr::Target(creation_params.sampler)){
                auto rehashed = Sampler::Condense(creation_params.sampler->info);
                auto found = samplers.association_container.find(rehashed);
                if(found == samplers.association_container.end()){
                    samplers.association_container.push_back(rehashed, creation_params.sampler);
                }
            }

            if(creation_params.view != nullptr){
                ImGui::Text("image view : %s", creation_params.view->image.name.c_str());
            }
            else{
                ImGui::Text("no image view");
            }
            DragDropPtr::Target(creation_params.view);

            Image* img_target = nullptr;
            if(DragDropPtr::Target(img_target)){
                AssetHash img_hash = Asset::INVALID_HASH;
                for(auto& img_kvp : Global::images->association_container){
                    if(img_kvp.value == img_target){
                        img_hash = img_kvp.key;
                        break;
                    }
                }
                EWE_ASSERT(img_hash != Asset::INVALID_HASH);
                creation_params.view = &views.Get(img_hash);
            }

            Reflect::Enum::Imgui_Combo_Selectable("descriptor type", creation_params.type);
            Reflect::Enum::Imgui_Combo_Selectable("layout", creation_params.layout);

            if(creation_params.view != nullptr){
                if(ImGui::Button("create")){
                    Get(creation_params);
                }
            }
        }

        if(association_container.size() > 0){
            ImGui::Separator();
        }

        for(auto& kvp : association_container){
            ImGui::PushID(kvp.key);
            if(ImGui::TreeNode(kvp.value->name.c_str())){
                auto found_image = imgui_texture_refs.find(kvp.value);
                if(found_image != imgui_texture_refs.end()){
                    ImVec2 image_size{
                        static_cast<float>(found_image->key->view.image.data.extent.width),
                        static_cast<float>(found_image->key->view.image.data.extent.height)
                    };
                    ImGui::Image(found_image->value, image_size);
                }


                if(kvp.value->sampler != nullptr){
                    if(ImGui::TreeNode("sampler")){
                        VkSamplerCreateInfo samplerInfo = kvp.value->sampler->info;
                        ImguiExtension::Imgui(samplerInfo);
                        ImGui::TreePop();
                    }
                }
                else{
                    ImGui::Text("no sampler");
                }
                if(ImGui::TreeNode("image view")){
                    ImguiExtension::Imgui(kvp.value->view);
                    ImGui::TreePop();
                }
                ImGui::Text("descriptor type : %s", Reflect::Enum::ToString(kvp.value->type).data());
                ImGui::Text("layout : %s", Reflect::Enum::ToString(kvp.value->imageInfo.imageLayout).data());
                ImGui::TreePop();
            }
            ImGui::PopID();
        }
    }
#endif

} //namespace Asset
} //namespace EWE