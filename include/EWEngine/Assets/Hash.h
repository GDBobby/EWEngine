#pragma once

#include <filesystem>

namespace EWE{
namespace Asset{

    static constexpr uint64_t INVALID_HASH = UINT64_MAX; 
    using AssetHash = uint64_t;

    AssetHash CrossPlatformPathHash(std::filesystem::path const& path);

} //namespace Asset
} //namespace EWE