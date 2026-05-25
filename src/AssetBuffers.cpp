#include "EWEngine/Assets/Buffers.h"

#include "EWEngine/EWEngine.h"

#include "EWEngine/Assets/Hash.h"
#include "EWEngine/Global.h"
#include "EWEngine/Imgui/DragDrop.h"
#include "EWEngine/Imgui/Objects.h"

namespace EWE{
namespace Asset{
    Manager<Buffer>::Manager(std::filesystem::path const& root_path)
    {
    }

    void Manager<Buffer>::Destroy(AssetHash hash){
        Buffer* buf = Get(hash);
        data_arena.DestroyElement(buf);
        association_container.Remove(hash);
    }
    void Manager<Buffer>::Destroy(Buffer& buf){
        //do I hash it first? idk
        AssetHash hash = GetHash(buf);
        Destroy(hash);
    }
    Buffer* Manager<Buffer>::Get(AssetHash hash){
        auto iter = association_container.find(hash);
        if(iter != association_container.end()){
            return iter->value;
        }
        else{
            return nullptr;
        }
    }
    Buffer* Manager<Buffer>::Get(std::filesystem::path const& name){
        return Get(CrossPlatformPathHash(name));
    }


    Buffer& Manager<Buffer>::ConstructInto(AssetHash hash,
        VkDeviceSize instanceSize, uint32_t instanceCount, 
        VmaAllocationCreateInfo const& vmaAllocCreateInfo, 
        VkBufferUsageFlags usageFlags
    ){
        auto& ele = data_arena.AddElement(engine->logicalDevice, instanceSize, instanceCount, vmaAllocCreateInfo, usageFlags);
        association_container.push_back(hash, &ele);
        return ele;
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
            if(ImGui::TreeNode(kvp.value->name.c_str())){
                Get(kvp.key);
                DragDropPtr::Source(*kvp.value);

                ImGui::TreePop();
            }
            else{
                DragDropPtr::Source(*kvp.value);
            }
        }
    }
#endif


} //namespace Asset
} //namespace EWE