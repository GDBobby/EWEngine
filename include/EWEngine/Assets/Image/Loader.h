#pragma once

#include <filesystem>

namespace EWE{
    struct Image;
    struct SpriteSheet;
    struct CubeMap;
    bool InitializeImage(Image& image, std::filesystem::path const& img_path, Queue::Type dstQueueType);

    bool InitializeSpriteSheet(SpriteSheet& sheet, std::filesystem::path const& img_path, Queue::Type dstQueueType, uint16_t texel_width, uint16_t texel_height);
    bool InitializeCubeMap(CubeMap& cubemap, std::filesystem::path const& img_path, Queue::Type dstQueueType);
}