#include "EWEngine/Graphics/SpriteSheet.h"

#include "EWEngine/Assets/DII.h"
#include "EWEngine/EWEngine.h"
#include <vulkan/vulkan_core.h>

namespace EWE{

    SpriteSheet::SpriteSheet(std::filesystem::path const& _name, uint16_t _texel_width, uint16_t _texel_height, Sampler& _sampler)
    : name{_name},
        imgs{0},
        sampler{_sampler},
        diis{0},
        texel_width(_texel_width),
        texel_height(_texel_height)
    {
        Asset::DiiCreation diiCreation{
            .sampler = &sampler,
            .type = DescriptorType::Combined,
            .layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        };

        /*
        VkImageViewCreateInfo viewCreateInfo{
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .image = image.image,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = image.data.format,
            .components{
                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY
            },
            .subresourceRange{
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = image.data.mipLevels,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };
        */

        for(uint16_t i = 0; i < diis.Size(); i++){
            //diiCreation.view = engine->
        }
    }
    

    SpriteSheet::SpriteSheet(std::filesystem::path const& _name, uint16_t _texel_width, uint16_t _texel_height, VkSamplerCreateInfo const& samplerCreateInfo)
    : SpriteSheet{_name, _texel_width, _texel_height, Global::assetManager->sampler.Get(samplerCreateInfo)}
    {}
    
} //namespace EWE