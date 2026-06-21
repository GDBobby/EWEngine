#pragma once

#include "EightWinds/Command/PackageRecord.h"
#include "EightWinds/RenderGraph/SubmissionTask.h"
namespace EWE{

    /*
    * this needs to be called, this is it's color attachment
        renderGraph.syncManager.AddAcquisition_Image(mergeTask, present_img_att_index);

    *for each image in usage, a transition needs to be added
        uint32_t acquire_imgui_output_index = mergeTask.resources.AddResource(temp_att_res, merge_acquire_usage);
        renderGraph.syncManager.AddTransition_Image(imguiTask, imgui_att_index, mergeTask, acquire_imgui_output_index);
    
    * and GenerateWorkload will need to be called when thats done
    */

    //i cant reverse up to the tex index, need to add it in
    struct SpecializedTransition{
        TaskResourceIndex lh;
        PerFlight<TextureIndex> texIndex;
    };

    struct MergeTask{//make this global maybe?

        Command::ObjectPackage objPkg;
        Command::PackageRecord pkgRecord;

        GPUTask gpuTask;
        SubmissionTask subTask;

        FullRenderInfo fri;

        InstructionPointer<ParamPack<Inst::Push>> push_pack;

        //the color attachment is the swap image

        static constexpr uint32_t present_img_att_index = 0;

        //this value is also in the shader, would need to be changed accordingly
        //unless i use a specializaiton constant? not setup for that yet
        static constexpr uint8_t max_merge_count = 4; 
        //max merge count could potentially be 8
        // but that's just extra branches in the fragment shader for what will most likely never be used
        // can be adjusted
        RuntimeArray<SpecializedTransition> textures_to_be_merged;

        [[nodiscard]] MergeTask();

        void AddToRenderGraph(RenderGraph& renderGraph);
    };

}