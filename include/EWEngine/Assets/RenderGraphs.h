#pragma once

#include "EWEngine/Assets/Base.h"
#include "EightWinds/RenderGraph/RenderGraph.h"

namespace EWE{
namespace Asset{
    

    template<>
    bool LoadAssetFromFile(RenderGraph* ptr_to_raw_mem, std::filesystem::path const& root_directory, std::filesystem::path const& path);
    template<>
    bool WriteAssetToFile(RenderGraph const& resource, std::filesystem::path const& root_directory, std::filesystem::path const& path);


} //namepsace Asset
} //namespace EWE