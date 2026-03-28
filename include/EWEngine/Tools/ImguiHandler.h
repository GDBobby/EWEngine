#pragma once

#include "EightWinds/Queue.h"

#include "EightWinds/Image.h"
#include "EightWinds/ImageView.h"
#include "EightWinds/Data/PerFlight.h"

#include "EightWinds/Backend/RenderInfo.h"

#include "EightWinds/Backend/Semaphore.h"
#include "EightWinds/Backend/SubmitInfo.h"

//#include "imgui/imgui.h"

#include "LAB/Vector.h"

#include <cstdint>

namespace EWE{
    struct CommandBuffer;


    struct ImguiViewport{
        VkRect2D current_viewport{
            .offset{0, 0},
            .extent{
                UINT32_MAX, UINT32_MAX
            }
        };
        ImGuiContext* context;

        std::function<void(EWE::ImguiViewport& vp)> exec_func = nullptr;
    };  

    struct ImguiHandler{
        Queue& queue;
        FullRenderInfo renderInfo;
        const uint32_t image_count;
        const VkSampleCountFlagBits sample_count;

        std::vector<ImguiViewport> viewports{};

        [[nodiscard]] explicit ImguiHandler(
            Queue& queue, 
            uint32_t imageCount, VkSampleCountFlagBits sampleCount
        );
        ~ImguiHandler();

        void InitializeImages();

        ImGuiContext* InitializeContext();

        //let's make this contorl it's own pool
        bool isRendering = false;
        //void BeginCommandBuffer();
        void Render(CommandBuffer& cmdBuf);
        //void SetSubmissionData(PerFlight<Backend::SubmitInfo>& submitInfo);
    };
}