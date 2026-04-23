#pragma once

#include "EWEngine/Assets/Hash.h"

#include "EightWinds/Data/Hive.h"
#include "EightWinds/Data/KeyValueContainer.h"
#include "EWEngine/Assets/FileResource.h"

namespace EWE{
namespace Asset{

    template<typename T>
    AssetHash GetHash(T& res);

    template<typename T>
    bool LoadAssetFromFile(T* ptr_to_raw_mem, std::filesystem::path const& path);

    template<typename T>
    bool WriteAssetToFile(T& resource, std::filesystem::path const& path);

    template<typename T>
    bool UpdateMetaFile();
    

} //namespace Asset
} //namespace EWE

