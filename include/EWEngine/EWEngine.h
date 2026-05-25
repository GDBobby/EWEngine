#pragma once

#include "EightWinds/Preprocessor.h"

#include "EWEngine/EngineSettings.h"
#include "EightWinds/VulkanHeader.h"
#include "EightWinds/Window.h"
#include "EightWinds/Instance.h"
#include "EightWinds/PhysicalDevice.h"
#include "EightWinds/LogicalDevice.h"
#include "EightWinds/Swapchain.h"

#include "EWEngine/STC_Manager.h"

#include "EightWinds/RenderGraph/RenderGraph.h"

#include "EWEngine/TextOverlay.h"

#include <string_view>

namespace EWE{
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

        STC_Manager stcManager;
        //TextOverlay textOverlay;
        AssetManager assetManager;
        SoundEngine soundEngine;

        uint64_t totalFramesSubmitted = 0;

        RenderGraph*& current_renderGraph;

        SubmissionTask& graphics_stc_task;
        SubmissionTask& compute_stc_task;

        [[nodiscard]] explicit EWEngine(std::string_view application_name, std::filesystem::path const& root_directory);

        Queue::Type GetQueueType(Queue& queue) const;
        Queue& GetQueue(Queue::Type type);
#if EWE_IMGUI
        void Imgui();
#endif
    };

    extern EWEngine* engine; //global
} //namespace EWE