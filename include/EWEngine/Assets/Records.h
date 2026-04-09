#pragma once

#include "EWEngine/Assets/Manager.h"
#include "EightWinds/Command/Record.h"

#include "EightWinds/Command/InstructionPackage.h"

namespace EWE{
namespace Asset{

    template<>
    struct Manager<Command::Record>{
        LogicalDevice& logicalDevice;
        FileSystem files;
        //FileSystem meta_files;

        Hive<Command::Record, 64> data_arena;
        KeyValueContainer<AssetHash, Command::Record*> association_container{};

        [[nodiscard]] explicit Manager(LogicalDevice& logicalDevice, std::filesystem::path const& root_path);

        static AssetHash GetHash(Command::Record const& rec){
            return CrossPlatformPathHash(rec.name);
        }

        //void UpdateMetaFile(AssetHash hash);
        //void UpdateMetaFile(AssetHash hash, Command::Record& img);

        void Destroy(AssetHash hash);
        void Destroy(Command::Record* record);

        Command::Record* Get(AssetHash hash);
        Command::Record* Get(std::string_view name);

#ifdef EWE_IMGUI
        void Imgui();
#endif
    };
} //namespace Asset
} //namespace EWE