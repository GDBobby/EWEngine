#pragma once

#include "EWEngine/Assets/Manager.h"
#include "EightWinds/RenderGraph/GPUTask.h"

namespace EWE{
namespace Asset{

    template<>
    struct Manager<GPUTask>{
        LogicalDevice& logicalDevice;
        FileSystem files;

        Hive<GPUTask, 64> data_arena;
        KeyValueContainer<AssetHash, GPUTask> association_container{};

        [[nodiscard]] explicit Manager(LogicalDevice& logicalDevice, std::filesystem::path const& root_path);

        static AssetHash GetHash(GPUTask const& task){
            return CrossPlatformPathHash(task.name);
        }

        void Destroy(AssetHash hash);
        void Destroy(GPUTask& task);

        GPUTask& Get(AssetHash hash);
        GPUTask& Get(std::filesystem::path const& name);

        template<typename... Args>
        requires(std::is_constructible_v<GPUTask, Args...>)
        GPUTask& Create(Args&&... args){
            return data_arena.AddElement(std::forward<Args>(args)...);
        }

#ifdef EWE_IMGUI
        void Imgui();
#endif

        static bool WriteToFile(GPUTask& task);
    };
} //namespace Asset
} //namespace EWE