#include "EWEngine/Graphics/SpriteSheet.h"

#include "EWEngine/Assets/DII.h"
#include "EWEngine/EWEngine.h"
#include "EWEngine/Assets/Image/Loader.h"

#include "ImageProcessor.h"

namespace EWE{

    SpriteSheet::SpriteSheet(std::filesystem::path const& _name, uint16_t _texel_width, uint16_t _texel_height, Sampler& _sampler)
    : name{_name},
        imgs{0},
        sampler{_sampler},
        diis{0},
        owningQueue{nullptr},
        texel_width(_texel_width),
        texel_height(_texel_height)
    {
        InitializeSpriteSheet(*this, Queue::Type::Graphics);

        const std::size_t sprite_count = imgs.Size();
        views.ClearAndResize(sprite_count);
        diis.ClearAndResize(sprite_count);
        for (std::size_t i = 0; i < sprite_count; i++) {
            views[i] = Global::assetManager->imageView.Get(*imgs[i]);
            diis[i] = &Global::assetManager->dii.ConstructInto(sampler, *views[i], DescriptorType::Combined, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }
    }
    

    SpriteSheet::SpriteSheet(std::filesystem::path const& _name, uint16_t _texel_width, uint16_t _texel_height, VkSamplerCreateInfo const& samplerCreateInfo)
    : SpriteSheet{_name, _texel_width, _texel_height, Global::assetManager->sampler.Get(samplerCreateInfo)}
    {}
    
} //namespace EWE