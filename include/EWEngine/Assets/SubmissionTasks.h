#pragma once

#include "EWEngine/Assets/Base.h"
#include "EightWinds/RenderGraph/SubmissionTask.h"

#include "EightWinds/Data/KeyValueContainer.h"

namespace EWE{
namespace Asset{

    template<>
    struct Manager<SubmissionTask>{
        FileSystem files;

        Hive<SubmissionTask, 64> data_arena;
        KeyValueContainer<AssetHash, SubmissionTask*> association_container{};

        [[nodiscard]] explicit Manager(std::filesystem::path const& root_path);

        static AssetHash GetHash(SubmissionTask const& task){
            return CrossPlatformPathHash(task.name);
        }

        void Destroy(AssetHash hash);
        void Destroy(SubmissionTask& task);

        SubmissionTask& Get(AssetHash hash);
        SubmissionTask& Get(std::filesystem::path const& name);

#ifdef EWE_IMGUI
        void Imgui();
#endif

        bool WriteToFile(SubmissionTask const& task);
        SubmissionTask& ReadFile(std::filesystem::path const& file_path);
    };
} //namespace Asset
} //namespace EWE