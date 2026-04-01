#pragma once

#include "EWEngine/Assets/Manager.h"
#include "EightWinds/RenderGraph/Command/InstructionPackage.h"

namespace EWE{
namespace Asset{

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

#ifdef EWE_IMGUI
        void Imgui();
#endif
    };
} //namespace Asset
} //namespace EWE