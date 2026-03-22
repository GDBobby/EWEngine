#pragma once

#include <filesystem>

namespace EWE{
    using AssetHash = uint64_t;
namespace Asset{
    static constexpr AssetHash INVALID_HASH = UINT64_MAX; 

    AssetHash CrossPlatformPathHash(std::filesystem::path const& path);

} //namespace Asset
} //namespace EWE