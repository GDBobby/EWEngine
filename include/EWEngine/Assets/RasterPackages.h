#pragma once

#include "EWEngine/Assets/Base.h"
#include "EWEngine/Assets/SubmissionTasks.h"
#include "EightWinds/RenderGraph/RasterPackage.h"

namespace EWE{
namespace Asset{
    template<>
    bool WriteAssetToFile(RasterPackage const& rt, std::filesystem::path const& root_directory, std::filesystem::path const& path);
    template<>
    bool LoadAssetFromFile(RasterPackage* ptr_to_raw_mem, std::filesystem::path const& root_directory, std::filesystem::path const& name);
} //namespace Asset
} //namespace EWE