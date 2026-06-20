#pragma once

#include <filesystem>

namespace EWE{
    struct Image;
    struct SpriteSheet;
    struct CubeMap;
    bool InitializeImage(Image& image, std::filesystem::path const& img_path, Queue::Type dstQueueType, bool async_transfer = true);

    bool InitializeSpriteSheet(SpriteSheet& sheet, Queue::Type dstQueueType);
    bool InitializeCubeMap(CubeMap& cubemap, std::filesystem::path const& img_path, Queue::Type dstQueueType);
}