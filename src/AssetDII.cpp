#include "EWEngine/Assets/DII.h"

#include "EWEngine/Assets/Hash.h"
#include "EWEngine/Imgui/Objects.h"
#include "EWEngine/Imgui/DragDrop.h"
#include "EightWinds/Reflect/Enum.h"
#include "EightWinds/DescriptorImageInfo.h"
#include "EightWinds/Sampler.h"

#include "EWEngine/Global.h"
#include <vulkan/vulkan_core.h>

#if EWE_IMGUI
    #include "imgui.h"
    #include "backends/imgui_impl_vulkan.h"
#endif

namespace EWE{
namespace Asset{

    
    
#if EWE_IMGUI

    bool CanAttemptImguiRef(DescriptorImageInfo const& dii){

        if(!dii.view.image.owningQueue){
            return false;
        }
        if(*dii.view.image.owningQueue != Global::stcManager->GetQueue(Queue::Graphics)){
            return false;
        }
        if(dii.type != DescriptorType::Combined){
            return false;
        }
        if(dii.imageInfo.imageLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL || dii.imageInfo.imageLayout == VK_IMAGE_LAYOUT_GENERAL){
            return true;
        }
        return false;
    }

    ImTextureRef GetTextureRef(DescriptorImageInfo const& dii){
        if(!CanAttemptImguiRef(dii)){
            return ImTextureID_Invalid;
        }
        return ImGui_ImplVulkan_AddTexture(dii.sampler != nullptr ? dii.sampler->sampler : VK_NULL_HANDLE, dii.view.view, dii.imageInfo.imageLayout);
    }
#endif

    Manager<DescriptorImageInfo>::Manager(
        std::filesystem::path const& root_path,
        Manager<Sampler>& _samplers,
        Manager<ImageView>& _views
    )
    : files{root_path, std::vector<std::string>{".dii"}},
        samplers{_samplers},
        views{_views}
    {

    }

    static constexpr std::size_t dii_version = 0;

    DescriptorImageInfo& Manager<DescriptorImageInfo>::Read(std::filesystem::path const& file_name){

        std::ifstream stream{files.root_directory / file_name, std::ios::binary};
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
            return data_arena.AddElement(*sampler, view, type, layout);
        }
        else{
            return data_arena.AddElement(view, type, layout);
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
        for(auto kvp : association_container){
            if(kvp.key == hash){
                return *kvp.value;
            }
        }
        //wasnt found, break into the file system
        auto found_file = files.hashed_path.find(hash);
        EWE_ASSERT(found_file != files.hashed_path.end());
        auto& ret = Read(found_file->value);
        ret.name = found_file->value;
        association_container.push_back(found_file->key, &ret);
        return ret;
    }
    DescriptorImageInfo& Manager<DescriptorImageInfo>::Get(std::filesystem::path const& name){
        auto hash = CrossPlatformPathHash(name);
        return Get(hash);
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

    AssetHash Manager<DescriptorImageInfo>::ConvertTextureIndexToHash(TextureIndex index) const{
        for(auto& kvp : association_container){
            if(kvp.value->index == index){
                return kvp.key;
            }
        }
        EWE_UNREACHABLE;
        return INVALID_HASH;
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
                for(auto& img_kvp : Global::assetManager->image.association_container){
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

        for(auto& kvp : files.hashed_path){
            if(association_container.find(kvp.key) == association_container.end()){
                if(ImGui::Button(kvp.value.string().c_str())){
                    Get(kvp.value.string());
                    Logger::Print("created dii : %s\n", kvp.value.string().c_str());
                }
            }
        }

        for(auto& kvp : association_container){
            ImGui::PushID(kvp.key);
            if(ImGui::TreeNode(kvp.value->name.c_str())){
                DragDropPtr::Source(*kvp.value);
                auto found_image = imgui_texture_refs.find(kvp.value);
                if(found_image != imgui_texture_refs.end() && found_image->key->view.image.data.layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL){
                    ImVec2 image_size{
                        static_cast<float>(found_image->key->view.image.data.extent.width),
                        static_cast<float>(found_image->key->view.image.data.extent.height)
                    };
                    ImGui::Image(found_image->value, image_size);
                }
                else if(CanAttemptImguiRef(*kvp.value)){
                    if(ImGui::Button("attempt to load preview")){
                        auto tex_ref = GetTextureRef(*kvp.value);
                        if(tex_ref != ImTextureID_Invalid){
                            imgui_texture_refs.push_back(kvp.value, tex_ref);
                        }
                    }
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
                
                if(ImGui::Button("write to file")) {
                    Write(*kvp.value, files.root_directory / kvp.value->name);
                    Logger::Print("wrote [%s] to file[%s]\n", kvp.value->name.c_str(), files.root_directory.string().c_str());
                }
                
                ImGui::TreePop();
            }
            else{
                DragDropPtr::Source(*kvp.value);
            }
            ImGui::PopID();
        }
    }
#endif

} //namespace Asset
} //namespace EWE