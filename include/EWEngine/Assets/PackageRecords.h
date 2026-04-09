##pragma once

#include "EWEngine/Assets/Manager.h"
#include "EightWinds/Command/InstructionPackage.h"
#include "EightWinds/Command/PackageRecord.h"

namespace EWE{
namespace Asset{

    template<>
    struct Manager<Command::PackageRecord>{
        LogicalDevice& logicalDevice;
        FileSystem files;

        Hive<Command::PackageRecord, 64> data_arena;
        KeyValueContainer<AssetHash, Command::PackageRecord*> association_container{};

        [[nodiscard]] explicit Manager(LogicalDevice& logicalDevice, std::filesystem::path const& root_path);

        static AssetHash GetHash(Command::PackageRecord const& rec){
            return CrossPlatformPathHash(rec.name);
        }

        void Destroy(AssetHash hash);
        void Destroy(Command::PackageRecord& rec);

        GPUTask& Get(AssetHash hash);
        GPUTask& Get(std::filesystem::path const& name);

#ifdef EWE_IMGUI
        void Imgui();
#endif

        static bool WriteToFile(Command::PackageRecord const& task);
        Command::PackageRecord& ReadFile(std::filesystem::path const& name):
    };
} //namespace Asset
} //namespace EWE