#pragma once

#include "EWEngine/Assets/Base.h"
#include "EightWinds/RenderGraph/GPUTask.h"

#include "EightWinds/Command/ParamPointerChain.h"

#include "EightWinds/Shader.h"

namespace EWE{

    //what about ShaderMeta??? Shader.h line 19
    struct GPUTaskMeta_Helper{
        struct PushHelper{
            ParamPointerChain pointer_chain;
            PushConstant push;
            std::vector<bool> buffer_active;
            std::vector<bool> texture_active;

            [[nodiscard]] explicit PushHelper(ParamPointerChain const& pointer_chain_to_push, std::span<Shader*> shaders);
        };
        std::vector<PushHelper> pushes;

        GPUTask& task;

        //need to offset the textue index by buffer size
        void ToggleResource(bool value, ParamPointerChain const& chain_into, uint8_t res_index);

        enum HelperType{
            SubTask,
            RenderGraph
        };
        [[nodiscard]] explicit GPUTaskMeta_Helper(GPUTask& task, HelperType hType);
    };

namespace Asset{
    template<>
    bool WriteAssetToFile(GPUTask const& task_meta, std::filesystem::path const& root_directory, std::filesystem::path const& path);
    template<>
    bool LoadAssetFromFile(GPUTask* ptr_to_raw_mem, std::filesystem::path const& root_directory, std::filesystem::path const& path);
} //namespace Asset
} //namespace EWE


namespace std {
    template <>
    struct hash<EWE::ParamPointerChain> {
        std::size_t operator()(EWE::ParamPointerChain const& chain) const noexcept;
    };
} //namespace std