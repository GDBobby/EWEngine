#include "EWEngine/Assets/Hash.h"
#include "EWEngine/Assets/ObjectPackages.h"
#include "EWEngine/Assets/BaseInstPackages.h"
#include "EWEngine/Global.h"
#include "EightWinds/Backend/Logger.h"
#include "EightWinds/GlobalPushConstant.h"

#include "EightWinds/ObjectRasterConfig.h"
#include "EightWinds/VulkanHeader.h"

#include "EWEngine/Imgui/DragDrop.h"

#include <fstream>

namespace EWE{
namespace Asset{
    
    Manager<Command::ObjectPackage>::Manager(std::filesystem::path const& root_path)
    : files{root_path, std::vector<std::string>{".eop"}}
    {
    }

    void Manager<Command::ObjectPackage>::Destroy(AssetHash hash){
        Command::ObjectPackage& pkg = Get(hash);
        data_arena.DestroyElement(&pkg);
        association_container.Remove(hash);
    }
    void Manager<Command::ObjectPackage>::Destroy(Command::ObjectPackage* instPackage){
        //do I hash it first? idk
        AssetHash hash = GetHash(*instPackage);
        Destroy(hash);
    }

    Command::ObjectPackage& Manager<Command::ObjectPackage>::Get(AssetHash hash){
        auto iter = association_container.find(hash);
        if(iter != association_container.end()){
            return *iter->value;
        }
        else{
            auto path_hash_data = files.hashed_path.at(hash);
            auto const& fs_path = path_hash_data.value;
            auto full_load_path = files.root_directory / fs_path;

            auto& ret = data_arena.AddElement();
            if(ReadInstPkgFile(&ret, full_load_path)){
                association_container.push_back(hash, &ret);
            }
            else{
                EWE_ASSERT(false, "failed to read obj pkg file");
            }

            return ret;
        }
    }
    Command::ObjectPackage& Manager<Command::ObjectPackage>::Get(std::filesystem::path const& name){
        return Get(CrossPlatformPathHash(name));
    }

#ifdef EWE_IMGUI
    void Manager<Command::ObjectPackage>::Imgui(){
        if(ImGui::Button("refresh files")){
            files.RefreshFiles();
        }

        for(auto& kvp : files.hashed_path){
            auto found = association_container.find(kvp.key);
            if(found == association_container.end()){
                if(ImGui::Button(kvp.value.string().c_str())){
                    Get(kvp.value);
                }
            }
        }
        for(auto& kvp : association_container){
            if(ImGui::TreeNode(kvp.value->name.c_str())){
                DragDropPtr::Source(*kvp.value);
                ImGui::TreePop();
            }
            //Logger::Print("%s\n", kvp.value->name.c_str());
            DragDropPtr::Source(*kvp.value);
        }
    }
#endif

} //namespace Asset
} //namespace EWE