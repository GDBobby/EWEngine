#pragma once

#include "EightWinds/Queue.h"

#include "EightWinds/Image.h"
#include "EightWinds/ImageView.h"
#include "EightWinds/Data/PerFlight.h"

#include "EightWinds/Backend/RenderInfo.h"

#include "EightWinds/Backend/Semaphore.h"
#include "EightWinds/Backend/SubmitInfo.h"

//#include "imgui/imgui.h"

#include <cstdint>

namespace EWE{
    struct CommandBuffer;

    struct ImguiHandler{
        Queue& queue; //graphics queue, doesnt need to be present
        //CommandPool cmdPool;

        //PerFlight<CommandBuffer> cmdBuffers;
        FullRenderInfo renderInfo;
        //PerFlight<Semaphore> semaphores;

        [[nodiscard]] explicit ImguiHandler(
            Queue& queue, 
            uint32_t imageCount, VkSampleCountFlagBits sampleCount
        );
        ~ImguiHandler();

        void InitializeImages();

        //let's make this contorl it's own pool
        bool isRendering = false;
        //void BeginCommandBuffer();
        void BeginRender(CommandBuffer& cmdBuf);
        void EndRender(CommandBuffer& cmdBuf);
        //void SetSubmissionData(PerFlight<Backend::SubmitInfo>& submitInfo);
    };
}