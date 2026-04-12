#pragma once

#include "EWEngine/Assets/Base.h"
#include "EightWinds/Command/ObjectInstructionPackage.h"

namespace EWE{
namespace Asset{

    /*
        right now, the plan is to write the param pool to file
        use hashes to shorten the data
    */

    template<>
    struct Manager<Command::ObjectPackage>{
        FileSystem files;
        //FileSystem meta_files;

        Hive<Command::ObjectPackage, 64> data_arena;
        KeyValueContainer<AssetHash, Command::ObjectPackage*> association_container{};

        [[nodiscard]] explicit Manager(std::filesystem::path const& root_path);

        static AssetHash GetHash(Command::ObjectPackage const& rec){
            return CrossPlatformPathHash(rec.name);
        }

        void Destroy(AssetHash hash);
        void Destroy(Command::ObjectPackage* pkg);

        Command::ObjectPackage& Get(AssetHash hash);
        Command::ObjectPackage& Get(std::filesystem::path const& name);

#ifdef EWE_IMGUI
        void Imgui();
#endif
    };
} //namespace Asset
} //namespace EWE