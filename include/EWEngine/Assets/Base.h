#pragma once

#include "EWEngine/Assets/Hash.h"
#include "EWEngine/Assets/Extensions.h"

#include "EightWinds/Data/Hive.h"
#include "EightWinds/Data/KeyValueContainer.h"
#include "EWEngine/Assets/FileSystem.h"

#include "EWEngine/Imgui/DragDrop.h"
#include "EWEngine/Imgui/Objects.h"

#include <type_traits>
#include <mutex>

namespace EWE{
namespace Asset{

    template<typename T>
    bool LoadAssetFromFile(T* ptr_to_raw_mem, std::filesystem::path const& root_directory, std::filesystem::path const& path);
    template<typename T>
    bool WriteAssetToFile(T const& resource, std::filesystem::path const& root_directory, std::filesystem::path const& path);

    template<typename T>
    //requires trivially constructible? or construct into?
    bool ReadMetaFile(T& meta, std::filesystem::path const& root_directory, std::filesystem::path const& file_path);
    template<typename T>
    bool WriteMetaFile(T const& res, std::filesystem::path const& root_directory, std::filesystem::path const& file_path);
    



    template<typename Resource>
    struct Manager{
        using Type = Resource;

        std::recursive_mutex mut;
        std::vector<FileSystem> files;

        Hive<Resource, 64> data_arena;
        KeyValueContainer<AssetHash, Resource*> association_container;


        [[nodiscard]] explicit Manager(std::filesystem::path const& root_path)
        : files{},
            data_arena{},
            association_container{}
        {
            files.emplace_back(root_path, acceptable_extensions<Resource>);

        }

        void Destroy(AssetHash hash){
            Resource* res = Get(hash);
            if(res == nullptr){
                Log::Warning("attempting to delete non-existing object\n");
                return;
            }
            data_arena.DestroyElement(res);
            association_container.Remove(hash);
        }
        void Destroy(Resource& res){
            //by routing it to hash, the object is looked up, ensuring it exists
            AssetHash hash = GetHash(res); 
            Destroy(hash);
        }

        //optional return?
        Resource* Get(AssetHash hash){
            std::unique_lock lock{mut}; //recursive lock?
            auto iter = association_container.find(hash);
            if(iter != association_container.end()){
                return iter->value;
            }
            else{
                if(files.size() == 0){
                    return nullptr;
                }

                std::filesystem::path root_dir = files[0].root_directory;
                auto path_hash_data = files[0].hashed_path.find(hash);
                if(path_hash_data == files[0].hashed_path.end()){
                    uint8_t iter = 1;
                    while(iter != files.size()){
                        root_dir = files[iter].root_directory;
                        path_hash_data = files[iter].hashed_path.find(hash);
                        if(path_hash_data != files[iter].hashed_path.end()){
                            root_dir = files[iter].root_directory;
                            break;
                        }
                        iter++;
                    }

                    if(iter >= files.size()) {
                        return nullptr;
                    }
                }
                Resource* ret = data_arena.GetCell();
                Log::Debug("loading asset - %s / %s\n", root_dir.string().c_str(), path_hash_data->value.string().c_str());
                if(LoadAssetFromFile<Resource>(ret, root_dir, path_hash_data->value)){
                    association_container.push_back(hash, ret);
                    return ret;
                }
                else{
                    data_arena.ReturnCell(ret);
                    return nullptr;
                }
            }
        }
        Resource* Get(std::filesystem::path const& name) {
            return Get(CrossPlatformPathHash(name));
        }

        template<typename... Args>
        requires std::constructible_from<Resource, Args...>
        Resource& ConstructInto(Args&&... args) {
            std::unique_lock lock{mut};
            auto& ele = data_arena.AddElement(std::forward<Args>(args)...);
            AssetHash hash = GetHash(ele);
            association_container.push_back(hash, &ele);
            return ele;
        }

#ifdef EWE_IMGUI
        void Imgui(){
            for(auto& file_dir : files){
                std::string refresh_name = "refresh dir : [";
                refresh_name += file_dir.root_directory.string() + ']';
                if(ImGui::Button(refresh_name.c_str())){
                    file_dir.RefreshFiles();
                }
            }

            for(auto& file_pack : files) {
                for(auto& kvp : file_pack.hashed_path){
                    auto found = association_container.find(kvp.key);
                    if(found == association_container.end()){
                        if(ImGui::Button(kvp.value.string().c_str())){
                            Get(kvp.key);
                        }
                    }
                    else{
                        if(ImGui::TreeNode(found->value->name.string().c_str())) {
                            DragDropPtr::Source(*found->value);

                            ImGui::PushID(found->value);
                            if(ImGui::Button("unload")){
                                Destroy(*found->value);
                            }
                            ImGui::PopID();

                            if constexpr (std::meta::is_complete_type(^^WriteAssetToFile<decltype(*found->value)>)){
                                WriteAssetToFile(*found->value);
                            }
                            if constexpr(std::meta::is_complete_type(^^ImguiExtension::Imgui<decltype(*found->value)>)){
                                ImguiExtension::Imgui(*found->value);
                            }
                            ImGui::TreePop();
                        }
                        else{
                            DragDropPtr::Source(*found->value);
                        }
                    }
                }
            }


            
            for(auto& kvp : association_container){
                for(auto& file_pack : files){
                    auto found = file_pack.hashed_path.find(kvp.key);
                    if(found == file_pack.hashed_path.end()){
                        bool tree_open = ImGui::TreeNode(kvp.value->name.string().c_str());
                        DragDropPtr::Source(*kvp.value);
                        if(tree_open){
                            if constexpr (std::meta::is_complete_type(^^WriteAssetToFile<decltype(*kvp.value)>)){
                                WriteAssetToFile(*kvp.value);
                            }
                            if constexpr(std::meta::is_complete_type(^^ImguiExtension::Imgui<decltype(*kvp.value)>)){
                                ImguiExtension::Imgui(*kvp.value);
                            }
                            ImGui::TreePop();
                        }
                    }
                }
            }
        }

        Resource* Imgui_Select(){

            for(auto& file_dir : files){
                std::string refresh_name = "refresh dir : [";
                refresh_name += file_dir.root_directory.string() + ']';
                if(ImGui::Button(refresh_name.c_str())){
                    file_dir.RefreshFiles();
                }
            }

            for(auto& file_pack : files){
                for(auto& kvp : file_pack.hashed_path){
                    auto found = association_container.find(kvp.key);
                    if(found == association_container.end()) {
                        if(ImGui::Button(kvp.value.string().c_str())){
                            return Get(kvp.key);
                        }
                    }
                    else{
                        if(ImGui::Button(kvp.value.string().c_str())){
                            return found->value;
                        }
                    }
                }
            }

            return nullptr;
        }
#endif
    };
} //namespace Asset
} //namespace EWE

