#pragma once

#include <vector>
#include <filesystem>
#include <unordered_map>

namespace EWE{

    struct FileSystem{

        //std::vector<std::filesystem::path> folders;
        std::filesystem::path root_directory;
        std::vector<std::filesystem::path> files; //will include the folders up to the root directory

        [[nodiscard]] explicit FileSystem(std::filesystem::path root_directory, std::vector<std::string_view> const& acceptable_extensions);

#if EWE_IMGUI
        void Imgui();
#endif
    };

    struct LogicalDevice;
    struct Shader;

    struct ShaderFileSystem{
        FileSystem files;
        LogicalDevice& logicalDevice;
        [[nodiscard]] explicit ShaderFileSystem(LogicalDevice& logicalDevice, std::filesystem::path root_directory);


        std::unordered_map<std::filesystem::path, Shader*> shaders;

        Shader* GetShader(std::string_view file_path);

#if EWE_IMGUI
        void Imgui();
#endif
    };
}