#pragma once

#include "EWEngine/Assets/Base.h"
#include "EightWinds/RenderGraph/RasterPackage.h"

namespace EWE{
namespace Asset{

    template<>
    struct Manager<RasterPackage>{
        FileSystem files;

        Hive<RasterPackage, 64> data_arena;
        KeyValueContainer<AssetHash, RasterPackage*> association_container{};

        [[nodiscard]] explicit Manager(std::filesystem::path const& root_path);

        static AssetHash GetHash(RasterPackage const& rt){
            return CrossPlatformPathHash(rt.name);
        }

        void Destroy(AssetHash hash);
        void Destroy(RasterPackage& rt);

        RasterPackage& Get(AssetHash hash);
        RasterPackage& Get(std::filesystem::path const& name);

#ifdef EWE_IMGUI
        void Imgui();
#endif

        static bool WriteToFile(RasterPackage const& rt);
        RasterPackage& ReadFile(std::filesystem::path const& name);
    };
} //namespace Asset
} //namespace EWE