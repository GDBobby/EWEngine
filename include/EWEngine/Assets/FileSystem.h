#pragma once

#include "EWEngine/Assets/Hash.h"

#include <span>
#include <filesystem>
#include <string_view>
#include <string>

namespace EWE{
namespace Asset{
    struct FileSystem{

        //std::vector<std::filesystem::path> folders;
        std::filesystem::path root_directory;
        std::vector<std::string> acceptable_extensions;

        KeyValueContainer<AssetHash, std::filesystem::path> hashed_path;

        [[nodiscard]] explicit FileSystem(
            std::filesystem::path const& root_directory, 
            std::span<const std::string_view> acceptable_extensions
        );

#if EWE_IMGUI
        void Imgui();
#endif

        void RefreshFiles();
    };
} //namespace Asset
} //namespace EWE