#pragma once

#include "EWEngine/Assets/Hash.h"

#include "EightWinds/Data/Hive.h"
#include "EightWinds/Data/KeyValueContainer.h"

#include "EightWinds/Buffer.h"
#include "EightWinds/Image.h"
#include "EightWinds/DescriptorImageInfo.h"

namespace EWE{

namespace Asset{
    //im doing this the lazy way rn
    //most likely going to need to rewrite this soon
    //hashing std::filesystem::path


    template<typename T>
    struct Manager{

        Hive<T, 32> assets{};
        KeyValueContainer<AssetHash, T*> quick_lookup_map;

        template<typename... Args>
        requires requires(Args&&... args) {
            assets.AddElement(std::forward<Args>(args)...);
        }
        T* CreateResource(Args&&... args){
            T* ret = assets.AddElement(std::forward<Args>(args)...);
            quick_lookup_map.emplace_back(HashingFunction(*ret), ret);
            return ret;
        }
        template<typename... Args>
        requires requires(Args&&... args) {
            assets.AddElement(std::forward<Args>(args)...);
        }
        T* CreateResource(AssetHash hash, Args&&... args){
            T* ret = assets.AddElement(std::forward<Args>(args)...);
            quick_lookup_map.emplace_back(hash, ret);
            return ret;
        }

        T* GetResource(AssetHash hash){
            return quick_lookup_map.At(hash).value;
        }

        AssetHash CheckHash(T* res){
            for(auto const& kvp : quick_lookup_map){
                if(kvp.value == res){
                    return kvp.key;
                }
            }
            return INVALID_HASH;
        }
    };

    AssetHash BufferHash(Buffer const& buffer);
    AssetHash ImageHash(Image const& image);
    AssetHash DIIHash(DescriptorImageInfo const& dii);

    //extern template struct Manager<Shader>;
    //extern template struct Manager<Sampler>;
    //extern template struct Manager<Image>;
    //extern template struct Manager<Buffer>;
    //extern template struct Manager<DescriptorImageInfo>;

} //namespace Asset
} //namespace EWE

