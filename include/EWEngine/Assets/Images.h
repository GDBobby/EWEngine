#pragma once

#include "EWEngine/Assets/Base.h"
#include "EightWinds/Sampler.h"
#include "EightWinds/Image.h"

namespace EWE{
namespace Asset{

    template<>
    //this is async
    bool LoadAssetFromFile(Image* ptr_to_raw_mem, std::filesystem::path const& root_directory, std::filesystem::path const& path);

    template<>
    bool ReadMetaFile(Image& meta, std::filesystem::path const& root_directory, std::filesystem::path const& path);
    template<>
    bool WriteMetaFile(Image const& img, std::filesystem::path const& root_directory, std::filesystem::path const& path);
} //namespace Asset
} //namespace EWE