#pragma once

#include "EWEngine/Assets/Base.h"
#include "EightWinds/Command/InstructionPackage.h"

namespace EWE{
namespace Asset{

    /*
        right now, the plan is to write the param pool to file
        use hashes to shorten the data
    */

    template<>
    struct Manager<Command::InstructionPackage>{
        FileSystem files;
        //FileSystem meta_files;

        Hive<Command::InstructionPackage, 64> data_arena;
        KeyValueContainer<AssetHash, Command::InstructionPackage*> association_container{};

        [[nodiscard]] explicit Manager(std::filesystem::path const& root_path);

        static AssetHash GetHash(Command::InstructionPackage const& rec){
            return CrossPlatformPathHash(rec.name);
        }

        //void UpdateMetaFile(AssetHash hash);
        //void UpdateMetaFile(AssetHash hash, Command::InstructionPackage& img);

        void Destroy(AssetHash hash);
        void Destroy(Command::InstructionPackage& pkg);

        Command::InstructionPackage& Get(AssetHash hash);
        Command::InstructionPackage& Get(std::filesystem::path const& name);

#ifdef EWE_IMGUI
        void Imgui();
#endif
    };


    bool WriteToInstPkgFile(Command::ParamPool const& param_pool, void* payload, Command::InstructionPackage::Type pkg_type, std::filesystem::path const& path);
    bool WriteToInstPkgFile(Command::InstructionPackage& pkg, std::filesystem::path const& path);
    bool ReadInstPkgFile(Command::InstructionPackage* construct_addr, std::filesystem::path const& path);

} //namespace Asset
} //namespace EWE