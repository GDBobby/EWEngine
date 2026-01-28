#pragma once

#include "EightWinds/LogicalDevice.h"
#include "EightWinds/Window.h"
#include "EightWinds/Queue.h"

#include "EightWinds/Image.h"
#include "EightWinds/ImageView.h"
#include "EightWinds/CommandBuffer.h"
#include "EightWinds/Data/PerFlight.h"

#include "EightWinds/Backend/RenderInfo.h"

#include "EightWinds/Backend/Semaphore.h"
#include "EightWinds/Backend/SubmitInfo.h"
#include "EightWinds/RenderGraph/GPUTask.h"

#include "EightWinds/RenderGraph/RasterTask.h"

//#include "imgui/imgui.h"
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"

#include <cstdint>
#include <span>

namespace EWE{
    struct ImguiHandler{
        Queue& queue; //graphics queue, doesnt need to be present
        CommandPool cmdPool;

        PerFlight<CommandBuffer> cmdBuffers;
        FullRenderInfo renderInfo;
        PerFlight<Semaphore> semaphores;

        [[nodiscard]] explicit ImguiHandler(
            Queue& queue, 
            uint32_t imageCount, VkSampleCountFlagBits sampleCount
        );
        ~ImguiHandler();

        void InitializeImages();

        //let's make this contorl it's own pool
        bool isRendering = false;
        void BeginCommandBuffer();
        void BeginRender();
        void EndRender();
        void SetSubmissionData(PerFlight<Backend::SubmitInfo>& submitInfo);
    };
}