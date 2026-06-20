#pragma once

#include "EightWinds/DescriptorImageInfo.h"


namespace EWE{
    struct SpriteSheet{
        std::filesystem::path name;
        RuntimeArray<Image*> imgs;
        RuntimeArray<ImageView*> views;
        Sampler& sampler; //copy or reference?
        RuntimeArray<DescriptorImageInfo*> diis;

        Queue* owningQueue; //not allowed to be null, but is changeable

        uint16_t texel_width;
        uint16_t texel_height;

        TextureIndex GetIndex(uint16_t sprite_index) const{
            return diis[sprite_index]->index;
        }

        [[nodiscard]] explicit SpriteSheet(std::filesystem::path const& name, uint16_t texel_width, uint16_t texel_height, VkSamplerCreateInfo const& samplerCreateInfo);
        [[nodiscard]] explicit SpriteSheet(std::filesystem::path const& name, uint16_t texel_width, uint16_t texel_height, Sampler& sampler);
    };
}