#pragma once

#include "EWEngine/Assets/Base.h"
#include "EightWinds/Command/ObjectInstructionPackage.h"

namespace EWE{
namespace Asset{
    template<>
    bool WriteAssetToFile(Command::ObjectPackage const& rt, std::filesystem::path const& root_directory, std::filesystem::path const& path);
} //namespace Asset
} //namespace EWE