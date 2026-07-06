#include "EWEngine/Assets/Buffers.h"

#include "EWEngine/EWEngine.h"

#include "EWEngine/Assets/Hash.h"
#include "EWEngine/Imgui/DragDrop.h"
#include "EWEngine/Imgui/Objects.h"

namespace EWE{
namespace Asset{
    Manager<Buffer>::Manager(std::filesystem::path const& root_path)
    {
    }

    void Manager<Buffer>::Destroy(AssetHash hash){
        std::unique_lock lock{mut};
        Buffer* buf = Get(hash);
        data_arena.DestroyElement(buf);
        association_container.Remove(hash);
    }
    void Manager<Buffer>::Destroy(Buffer& buf){
        AssetHash hash = GetHash(buf);
        std::unique_lock lock{mut};
        data_arena.DestroyElement(&buf);
        association_container.Remove(hash);
    }
    Buffer* Manager<Buffer>::Get(AssetHash hash){
        std::unique_lock lock{mut};
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

    Buffer& Manager<Buffer>::ConstructInto(std::filesystem::path const& name,
        VkDeviceSize instanceSize, uint32_t instanceCount,
        VmaAllocationCreateInfo const& vmaAllocCreateInfo,
        VkBufferUsageFlags usageFlags
    ){
        const AssetHash hash = CrossPlatformPathHash(name);

        std::unique_lock lock{mut};
        auto& ret = data_arena.AddElement(engine->logicalDevice, instanceSize, instanceCount, vmaAllocCreateInfo, usageFlags);
        association_container.push_back(hash, &ret);
        ret.SetName(name.string());
        return ret;
    }

    AssetHash Manager<Buffer>::ConvertBDAToHash(DeviceAddress addr){
        std::unique_lock lock{mut};
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
        std::unique_lock lock{mut};
        for(auto& kvp : association_container){
            if(ImGui::TreeNode(kvp.value->name.string().c_str())) {
                Get(kvp.key); //what is this??
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