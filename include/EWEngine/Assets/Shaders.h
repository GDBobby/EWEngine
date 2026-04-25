#pragma once

#include "EightWinds/Shader.h"
#include "EWEngine/Assets/Base.h"


namespace EWE{
    struct Shader;
namespace Asset{

    template<>
    bool LoadAssetFromFile(Shader* ptr_to_raw_mem, std::filesystem::path const& root_directory, std::filesystem::path const& path);
    
    template<>
    bool ReadMetaFile(ShaderMeta& meta, std::filesystem::path const& root_directory, std::filesystem::path const& file_path);
} //namespace Asset
} //namespace EWE