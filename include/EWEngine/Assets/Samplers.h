#pragma once

#include "EightWinds/Sampler.h"
#include "EWEngine/Assets/Base.h"

namespace EWE{
namespace Asset{

    //specialized, samplers aren't read or written to from file
    //instead, they are condensed to a 64bit value

    template<>
    struct Manager<Sampler>{
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