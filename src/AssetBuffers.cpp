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
        Buffer& buf = Get(hash);
        data_arena.DestroyElement(&buf);
        association_container.Remove(hash);
    }
    void Manager<Buffer>::Destroy(Buffer& buf){
        //do I hash it first? idk
        AssetHash hash = GetHash(buf);
        Destroy(hash);
    }
    Buffer& Manager<Buffer>::Get(AssetHash hash){
        auto iter = association_container.find(hash);
        if(iter != association_container.end()){
            return *iter->value;
        }
        else{
            Buffer& buf = data_arena.AddElement(logicalDevice);
            association_container.push_back(hash, &buf);
            //set name??
            return buf;
        }
    }
    Buffer& Manager<Buffer>::Get(std::filesystem::path const& name){
        auto& ret = Get(CrossPlatformPathHash(name));
        return ret;
    }
    Buffer& Manager<Buffer>::Get(std::filesystem::path const& name,
            VkDeviceSize instanceSize, uint32_t instanceCount, 
            VmaAllocationCreateInfo const& vmaAllocCreateInfo, 
            VkBufferUsageFlags usageFlags
    )
    {
        const auto hash = CrossPlatformPathHash(name);
        auto iter = association_container.find(hash);
        if(iter != association_container.end()){
            Logger::Print<Logger::Error>("attempting to re-create an existing buffer\n");
            return *iter->value;
        }
        else{
            Buffer& buf = data_arena.AddElement(logicalDevice, instanceSize, instanceCount, vmaAllocCreateInfo, usageFlags);
            association_container.push_back(hash, &buf);
            buf.SetName(name.string());
            return buf;
        }
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