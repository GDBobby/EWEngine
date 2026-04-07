#include "EWEngine/Assets/Buffers.h"

#include "EWEngine/Assets/Hash.h"
#include "EWEngine/Imgui/DragDrop.h"
#include "EWEngine/Imgui/Objects.h"

namespace EWE{
namespace Asset{
Manager<Buffer>::Manager(LogicalDevice& _logicalDevice, std::filesystem::path const& root_path)
    : logicalDevice{_logicalDevice}//,
        //files{root_path, std::vector<std::string>{".png", ".jpg", ".bmp", ".dds"}}//,
        //meta_files{root_path, std::vector<std::string>{".mie"}}
    {

    }

    void Manager<Buffer>::Destroy(AssetHash hash){
        Buffer* buf = Get(hash);
        EWE_ASSERT(buf != nullptr);
        data_arena.DestroyElement(buf);
        association_container.Remove(hash);
    }
    void Manager<Buffer>::Destroy(Buffer* buf){
        //do I hash it first? idk
        AssetHash hash = GetHash(*buf);
        Destroy(hash);
    }
    Buffer* Manager<Buffer>::Get(AssetHash hash){
        auto iter = association_container.find(hash);
        if(iter != association_container.end()){
            return iter->value;
        }
        else{
            EWE_UNREACHABLE;
            //auto buf_path_hash_data = files.hashed_path.at(hash);
            //auto const& fs_path = buf_path_hash_data.value;
            Buffer& buf = data_arena.AddElement(logicalDevice);
            //buf.name = fs_path;
            association_container.push_back(hash, &buf);

            //auto full_buf_load_path = files.root_directory / fs_path;


            return &buf;
        }
    }
    Buffer* Manager<Buffer>::Get(std::string_view name){
        return Get(CrossPlatformPathHash(name));
    }


    AssetHash Manager<Buffer>::ConvertBDAToHash(VkDeviceAddress addr){
        for(auto& kvp : association_container){
            if(kvp.value->deviceAddress == addr){
                return kvp.key;
            }
        }
        EWE_UNREACHABLE;
    }

#ifdef EWE_IMGUI
    void Manager<Buffer>::Imgui(){
        //filesystem.Imgui();
        for(auto& kvp : association_container){
            //auto found = association_container.find(kvp.key);
            //if(found == association_container.end()){
                if(ImGui::Button(kvp.value->name.c_str())){
                    Get(kvp.key);
                }
            //}
            //else{
            //    ImGui::PushID(kvp.key);
            //    ImGui::Text(kvp.value.string().c_str());
            //    DragDropPtr::Source<Buffer>(*found->value);
            //    
            //    ImGui::PopID();
            //}
        }
    }
#endif
} //namespace Asset
} //namespace EWE