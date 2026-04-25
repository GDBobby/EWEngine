#pragma once

#include "EWEngine/Assets/Base.h"
#include "EightWinds/Command/InstructionPackage.h"
#include "EightWinds/Command/ParamPool.h"

namespace EWE{
namespace Asset{

    bool WriteToInstPkgFile(Command::ParamPool const& param_pool, void const* payload, Command::InstructionPackage::Type pkg_type, std::filesystem::path const& root_directory, std::filesystem::path const& path);
    
    template<>
    bool WriteAssetToFile(Command::InstructionPackage const& pkg, std::filesystem::path const& root_directory, std::filesystem::path const& path);
    template<>
    bool LoadAssetFromFile(Command::InstructionPackage* construct_addr, std::filesystem::path const& root_directory, std::filesystem::path const& path);

} //namespace Asset
} //namespace EWE