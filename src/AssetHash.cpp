#include "EWEngine/Assets/Hash.h"

#include "xxhash.h"

namespace EWE{
namespace Asset{

    AssetHash CrossPlatformPathHash(std::filesystem::path const& path){
        std::string gen_string = path.generic_string();

        return XXH3_64bits(gen_string.data(), gen_string.size());
    }
} //namespace Asset
} //namespace EWE