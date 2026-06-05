#pragma once
#include "EightWinds/DescriptorImageInfo.h"

namespace EWE{
    struct CubeMap{
        std::filesystem::path name;
        Image& image;
        
    };
}