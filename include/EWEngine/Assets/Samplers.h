#pragma once

#include "EightWinds/Sampler.h"
#include "EightWinds/Data/KeyValueContainer.h"

#include "EWEngine/Assets/Manager.h"

namespace EWE{
namespace Asset{
    inline auto CondenseSampler(Sampler const& sampler){
        return Sampler::Condense(sampler.info);
    }

    template<>
    struct Manager<Sampler>{
        LogicalDevice& logicalDevice;

        static AssetHash GetHash(Sampler const& sampler){
            return Sampler::Condense(sampler.info);
        }

        [[nodiscard]] explicit Manager(LogicalDevice& logicalDevice);
        
        Hive<Sampler, 64> data_arena;
        KeyValueContainer<uint64_t, Sampler*> association_container;

        void Destroy(uint64_t condensed_val);
        void Destroy(Sampler* sampler);

        Sampler& Get(uint64_t condensed_val);
        Sampler& Get(VkSamplerCreateInfo const& samplerInfo);

#ifdef EWE_IMGUI
        void Imgui();
#endif
    };
} //namespace Asset
} //namespace EWE