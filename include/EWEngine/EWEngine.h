#pragma once

#include "EightWinds/VulkanHeader.h"


#include "EightWinds/Window.h"
#include "EightWinds/Instance.h"
#include "EightWinds/PhysicalDevice.h"
#include "EightWinds/LogicalDevice.h"
#include "EightWinds/Swapchain.h"
#include "EightWinds/RenderGraph/RenderGraph.h"

#include "EWEngine/EngineSettings.h"
#include "EWEngine/TextOverlay.h"
#include "EWEngine/Imgui/ImguiHandler.h"
#include "EWEngine/Systems/SceneManager.h"
#include "EWEngine/Assets/Manager.h"
#include "EWEngine/Systems/Sound_Engine.h"

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
        marl::Scheduler scheduler;
        uint8_t frameIndex;

        Instance instance;
        Window window;
        LogicalDevice logicalDevice;

        Queue& renderQueue;
        Queue& computeQueue;
        Queue& transferQueue;

        Swapchain swapchain;

        AssetManager assetManager;
        STC_Manager stcManager;
        TextOverlay textOverlay;
        SoundEngine soundEngine;
        SceneManager sceneManager;

        ImguiHandler imguiHandler;

        LoopTimer render_loop_timer;
        LoopTimer physics_loop_timer;

        uint64_t totalFramesSubmitted = 0;

        RenderGraph*& current_renderGraph;

        SubmissionTask& graphics_stc_task;
        SubmissionTask& compute_stc_task;

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