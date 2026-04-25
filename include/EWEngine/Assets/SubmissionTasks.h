#pragma once

#include "EightWinds/RenderGraph/SubmissionTask.h"
#include "EWEngine/Assets/Base.h"

namespace EWE{
namespace Asset{

    template<>
    bool LoadAssetFromFile(SubmissionTask* ptr_to_raw_mem, std::filesystem::path const& root_directory, std::filesystem::path const& path);
    template<>
    bool WriteAssetToFile(SubmissionTask const& resource, std::filesystem::path const& root_directory, std::filesystem::path const& path);
} //namespace Asset
} //namespace EWE