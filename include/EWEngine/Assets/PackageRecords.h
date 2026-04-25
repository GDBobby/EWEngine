#pragma once

#include "EWEngine/Assets/Base.h"
#include "EightWinds/Command/PackageRecord.h"

namespace EWE{
namespace Asset{
    template<>
    bool WriteAssetToFile(Command::PackageRecord const& record, std::filesystem::path const& root_directory, std::filesystem::path const& path);
    template<>
    bool LoadAssetFromFile(Command::PackageRecord* ptr_to_raw_mem, std::filesystem::path const& root_directory, std::filesystem::path const& path);
} //namespace Asset
} //namespace EWE