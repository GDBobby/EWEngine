#include "EWEngine/Assets/RenderGraphs.h"

namespace EWE{
namespace Asset{


    template<>
    bool LoadAssetFromFile(RenderGraph* ptr_to_raw_mem, std::filesystem::path const& root_directory, std::filesystem::path const& path){

        return true;
    }
    template<>
    bool WriteAssetToFile(RenderGraph const& resource, std::filesystem::path const& root_directory, std::filesystem::path const& path){

        return true;
    }

} //namespace Asset
} //namespace EWE