#pragma once

#include "EightWinds/VulkanHeader.h"
#include "EightWinds/Window.h"
#include "EightWinds/Instance.h"
#include "EightWinds/PhysicalDevice.h"
#include "EightWinds/LogicalDevice.h"
#include "EightWinds/Swapchain.h"

#include "EightWinds/RenderGraph/RenderGraph.h"

#include <string_view>

namespace EWE{
    struct EWEngine{
        Instance instance;
        Window window;
        LogicalDevice logicalDevice;
        Swapchain swapchain;
        RenderGraph renderGraph;

        uint64_t totalFramesSubmitted = 0;

        [[nodiscard]] explicit EWEngine(std::string_view application_name);

#if EWE_IMGUI
        void Imgui();
#endif
    };
} //namespace EWE