#pragma once

#include "EightWinds/VulkanHeader.h"


#include "EightWinds/Window.h"
#include "EightWinds/Instance.h"
#include "EightWinds/PhysicalDevice.h"
#include "EightWinds/LogicalDevice.h"
#include "EightWinds/Swapchain.h"
#include "EightWinds/RenderGraph/RenderGraph.h"

#include "EWEngine/EngineSettings.h"
#include "EWEngine/Systems/LeafSystem.h"

#include "EWEngine/Global.h"

#include "EWEngine/Data/Timing.h"

#include "marl/scheduler.h"

#include "LAB/Vector.h"

#include <string_view>

namespace EWE{

    static constexpr uint32_t application_wide_vk_version = VK_MAKE_API_VERSION(0, 1, 4, 0);
    static constexpr uint32_t rounded_down_vulkan_version = EWE::RoundDownVkVersion(application_wide_vk_version);

    void SetMainThread();
    bool CheckMainThread();


    struct EWEngine{
        std::filesystem::path root_directory;
        marl::Scheduler scheduler;
        LoopTimer render_loop_timer;
        LoopTimer physics_loop_timer;

        Instance instance;
        Window window;
        LogicalDevice logicalDevice;

        Queue& renderQueue;
        Queue& computeQueue;
        Queue& transferQueue;

        Swapchain swapchain;
        STC_Manager stcManager;
        RenderGraph*& current_renderGraph;

        uint8_t frameIndex;

        SubmissionTask graphics_stc_task;
        SubmissionTask compute_stc_task;

        LeafSystem leafSystem;

        uint64_t totalFramesSubmitted = 0;

        [[nodiscard]] explicit EWEngine(std::string_view application_name, std::filesystem::path const& root_directory);

        static Queue::Type GetQueueType(Queue& queue);
        static Queue& GetQueue(Queue::Type type);

        static lab::vec2 GetCursorWindowPos();

#if EWE_IMGUI
        void Imgui();
#endif
    };

    extern EWEngine* engine; //global
} //namespace EWE