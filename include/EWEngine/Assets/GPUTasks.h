#pragma once

#include "EWEngine/Assets/Base.h"
#include "EightWinds/RenderGraph/GPUTask.h"

namespace EWE{
namespace Asset{

    template<>
    bool WriteAssetToFile(GPUTask const& task, std::filesystem::path const& root_directory, std::filesystem::path const& path);
} //namespace Asset
} //namespace EWE