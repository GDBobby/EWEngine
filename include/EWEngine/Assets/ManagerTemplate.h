#pragma once

#include "EightWinds/Data/Hive.h"
#include "EightWinds/Data/KeyValueContainer.h"
#include "EWEngine/Imgui/DragDrop.h"
#include <type_traits>

namespace EWE{
namespace Asset{

    template<typename Resource>
    struct Manager<Resource>{
        using Type = Resource;

        FileSystem files;
        Hive<Resource, 64> data_arena{};
        KeyValueContainer<AssetHash, Resource*> association_container{};

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
                return *iter->value;
            }
            else{
                auto path_hash_data = files.hashed_path.find(hash);
                if(path_hash_data == files.hashed_path.end()){
                    return nullptr;
                }
                auto const& fs_path = path_hash_data->value;
                if(!std::filesystem::exists(fs_path)){
                    return nullptr;
                }
                Resource* ret = data_arena.GetCell();

                if(ReadAssetFromFile<Resource>(ret, files.root_directory, fs_path)){
                    association_container.push_back(hash, ret);
                    return *ret;
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

        template<auto F, typename... Args>
        requires(std::is_same_v<std::invoke_result_t<F, Args>, bool>)
        Resource& ConstructInto(F&& f, Args&& args...){
            if(std::invoke(f, args...)){

                }
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
                    if(ImGui::TreeNode(found->value->name.c_str())) {
                        DragDropPtr::Source(*found->value);
                        if constexpr (std::meta::is_complete_type(^^WriteAssetToFile(*found->value)){
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