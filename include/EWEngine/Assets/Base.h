#pragma once

#include "EWEngine/Assets/Hash.h"
#include "EWEngine/Assets/Extensions.h"

#include "EightWinds/Data/Hive.h"
#include "EightWinds/Data/KeyValueContainer.h"
#include "EWEngine/Assets/FileSystem.h"

#include "EWEngine/Imgui/DragDrop.h"

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

        FileSystem files;
        Hive<Resource, 64> data_arena;
        KeyValueContainer<AssetHash, Resource*> association_container;


        [[nodiscard]] explicit Manager(std::filesystem::path const& root_path)
        : files{root_path, acceptable_extensions<Resource>},
            data_arena{},
            association_container{}
        {}

        void Destroy(AssetHash hash){
            Resource& res = Get(hash);
            data_arena.DestroyElement(&res);
            association_container.Remove(hash);
        }
        void Destroy(Resource& res){
            AssetHash hash = GetHash(res);
            Destroy(hash);
        }

        //optional return?
        Resource* Get(AssetHash hash){
            auto iter = association_container.find(hash);
            if(iter != association_container.end()){
                return iter->value;
            }
            else{
                const auto path_hash_data = files.hashed_path.find(hash);
                if(path_hash_data == files.hashed_path.end()){
                    return nullptr;
                }
                Resource* ret = data_arena.GetCell();

                if(LoadAssetFromFile<Resource>(ret, files.root_directory, path_hash_data->value)){
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
        requires requires(Args&&... args) {
            { data_arena.AddElement(std::forward<Args>(args)...) };
        }
        Resource& ConstructInto(Args&&... args){
            auto& ele = data_arena.AddElement(std::forward<Args>(args)...);
            AssetHash hash = GetHash(ele);
            association_container.push_back(hash, &ele);
            return ele;
        }

#ifdef EWE_IMGUI
        void Imgui(){
            if(ImGui::Button("refresh files")){
                files.RefreshFiles();
            }

            for(auto& kvp : files.hashed_path){
                auto found = association_container.find(kvp.key);
                if(found == association_container.end()){
                    if(ImGui::Button(kvp.value.string().c_str())){
                        Get(kvp.key);
                    }
                }
                else{
                    if(ImGui::TreeNode(found->value->name.string().c_str())) {
                        DragDropPtr::Source(*found->value);
                        if constexpr (std::meta::is_complete_type(^^WriteAssetToFile<decltype(*found->value)>)){
                            WriteAssetToFile(*found->value);
                        }
                        //ImGuiExtension::Imgui(*kvp.value);
                        ImGui::TreePop();
                    }
                    else{
                        DragDropPtr::Source(*found->value);
                    }
                }
            }
        }
#endif
    };
} //namespace Asset
} //namespace EWE

