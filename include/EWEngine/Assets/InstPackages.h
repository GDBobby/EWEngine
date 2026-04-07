#pragma once

#include "EWEngine/Assets/Manager.h"
#include "EightWinds/Command/InstructionPackage.h"

namespace EWE{
namespace Asset{

    /*
        right now, the plan is to write the param pool to file
        use hashes to shorten the data
    */

    template<>
    struct Manager<Command::InstructionPackage>{
        LogicalDevice& logicalDevice;
        FileSystem files;
        //FileSystem meta_files;

        Hive<Command::InstructionPackage, 64> data_arena;
        KeyValueContainer<AssetHash, Command::InstructionPackage*> association_container{};

        [[nodiscard]] explicit Manager(LogicalDevice& logicalDevice, std::filesystem::path const& root_path);

        static AssetHash GetHash(Command::InstructionPackage const& rec){
            return CrossPlatformPathHash(rec.name);
        }

        //void UpdateMetaFile(AssetHash hash);
        //void UpdateMetaFile(AssetHash hash, Command::InstructionPackage& img);

        void Destroy(AssetHash hash);
        void Destroy(Command::InstructionPackage* sampler);

        Command::InstructionPackage* Get(AssetHash hash);
        Command::InstructionPackage* Get(std::string_view name);

        bool WriteToFile(Command::InstructionPackage& pkg, std::filesystem::path const& path);
        static Command::InstructionPackage* ReadFile(std::filesystem::path const& path);

#ifdef EWE_IMGUI
        void Imgui();
#endif
    };
} //namespace Asset
} //namespace EWE