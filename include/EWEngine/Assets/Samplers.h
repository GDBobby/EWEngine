#pragma once

#include "EightWinds/Sampler.h"
#include "EightWinds/Data/KeyValueContainer.h"

#include "EWEngine/Assets/Base.h"

namespace EWE{
namespace Asset{
    inline auto CondenseSampler(Sampler const& sampler){
        return Sampler::Condense(sampler.info);
    }

    template<>
    struct Manager<Sampler>{

        static AssetHash GetHash(Sampler const& sampler){
            return Sampler::Condense(sampler.info);
        }

        [[nodiscard]] explicit Manager();
        
        Hive<Sampler, 64> data_arena;
        KeyValueContainer<Sampler::CondensedType, Sampler*> association_container;

        void Destroy(Sampler::CondensedType condensed_val);
        void Destroy(Sampler* sampler);

        Sampler& Get(Sampler::CondensedType condensed_val);
        Sampler& Get(VkSamplerCreateInfo const& samplerInfo);

#ifdef EWE_IMGUI
        void Imgui();
#endif
    };
} //namespace Asset
} //namespace EWE