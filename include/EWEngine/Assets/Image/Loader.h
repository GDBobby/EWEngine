#pragma once

#include <filesystem>

namespace EWE{
    struct Image;
    bool InitializeImage(Image& image, std::filesystem::path const& img_path, Queue::Type dstQueueType);
}