#pragma once

#include "EightWinds/DescriptorImageInfo.h"
#include "EightWinds/RenderGraph/RenderGraph.h"

namespace EWE{
    void CreateImguiSubmission();

    /*
    * it's own attachment needs to be reacquired
        renderGraph.syncManager.AddAcquisition_Image(imguiTask, imgui_att_index);
        
    */
    struct ImguiTask{
        GPUTask gpuTask;
        SubmissionTask subTask;
        PerFlight<DescriptorImageInfo> attachment_diis;

        [[nodiscard]] ImguiTask();

        //it needs to be added to the submission groups separately
        void AddToRenderGraph(RenderGraph& renderGraph);

        static constexpr uint32_t color_attachment_index = 0;
    };

}