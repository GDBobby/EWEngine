#pragma once

#include "EWEngine/Assets/Base.h"
#include "EightWinds/Backend/RenderInfo.h"

namespace EWE{
namespace Asset{
    template<>
    bool WriteAssetToFile(FullRenderInfo const& fri, std::filesystem::path const& root_directory, std::filesystem::path const& path);
    template<>
    bool LoadAssetFromFile(FullRenderInfo* ptr_to_raw_mem, std::filesystem::path const& root_directory, std::filesystem::path const& name);
}//namespace Asset
}//namepsace EWE