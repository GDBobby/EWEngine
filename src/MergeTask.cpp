#include "EWEngine/Graphics/SpecializedTasks/MergeTask.h"

#include "EWEngine/EWEngine.h"
#include "EWEngine/Global.h"

#include "EightWinds/VulkanHeader.h"

namespace EWE{

    AttachmentSetInfo GetMergeAttInfo(){
        AttachmentSetInfo mainSetInfo{};
        {
            mainSetInfo.relative_size = true;
            mainSetInfo.width = 1.f;
            mainSetInfo.height = 1.f;
            mainSetInfo.renderingFlags = 0;
            mainSetInfo.colors.ClearAndResize(1);
            mainSetInfo.using_depth = false;
            
            auto& color_back = mainSetInfo.colors[0];
            color_back.format = engine->swapchain.swapCreateInfo.imageFormat;
            color_back.clearValue.color.float32[0] = 0.f;
            color_back.clearValue.color.float32[1] = 0.f;
            color_back.clearValue.color.float32[2] = 0.f;
            color_back.clearValue.color.float32[3] = 0.f;
            color_back.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            color_back.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        }

        return mainSetInfo;
    }

    MergeTask::MergeTask()
    : objPkg{"mergeobj"},
        pkgRecord{"merge_record", engine->renderQueue},
        gpuTask{
            "mergetask",
            engine->logicalDevice,
            pkgRecord, false
        },
        subTask{
            "mergesub", 
            engine->logicalDevice, engine->renderQueue
        },
        fri{"main ri", engine->logicalDevice, engine->renderQueue, GetMergeAttInfo(), engine->window.screenDimensions.width, engine->window.screenDimensions.height}
    {

        ObjectRasterConfig objectConfig;
        objectConfig.SetDefaults();
        objectConfig.cullMode = VK_CULL_MODE_NONE;
        objectConfig.depthClamp = false;
        objectConfig.rasterizerDiscard = false;
        objectConfig.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;

        objPkg.payload.config = objectConfig;

        Shader& merge_vert = *Global::assetManager->shader.Get("merge.vert.spv");
        Shader& merge_frag = *Global::assetManager->shader.Get("merge.frag.spv");
        objPkg.payload.shaders[ShaderStage::Vertex] = &merge_vert;
        objPkg.payload.shaders[ShaderStage::Fragment] = &merge_frag;

        TaskRasterConfig merge_task_config;
        {
            merge_task_config.SetDefaults();
            merge_task_config.attachment_info = fri.full.setInfo;
            merge_task_config.depthStencilInfo.depthTestEnable = VK_FALSE;
            merge_task_config.depthStencilInfo.depthWriteEnable = VK_FALSE;
            merge_task_config.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
            merge_task_config.depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
            merge_task_config.depthStencilInfo.stencilTestEnable = VK_FALSE;
        }

        RasterPackage& mergeRaster = Global::assetManager->rasterTask.ConstructInto(
            "merge raster", 
            engine->logicalDevice, engine->renderQueue, 
            merge_task_config 
        );

        mergeRaster.scissor = engine->window.screenDimensions;
        mergeRaster.viewport.x = 0.f;
        mergeRaster.viewport.y = static_cast<float>(engine->window.screenDimensions.height);
        mergeRaster.viewport.width = static_cast<float>(engine->window.screenDimensions.width);
        mergeRaster.viewport.height = -static_cast<float>(engine->window.screenDimensions.height);
        mergeRaster.viewport.minDepth = 0.0f;
        mergeRaster.viewport.maxDepth = 1.f;


        {
            ParamPack<Inst::Push> temp_push_pack{
                .buffer_count = 0,
                .texture_count = MergeTask::max_merge_count,
            };
            temp_push_pack.size = temp_push_pack.Size();

            objPkg.paramPool.PushBack(temp_push_pack);
            ParamPack<Inst::Draw> draw_pack{
                .vertexCount = 4,
                .instanceCount = 1,
                .firstVertex = 0,
                .firstInstance = 0
            };
            objPkg.paramPool.PushBack(draw_pack);

        }
        push_pack = *reinterpret_cast<InstructionPointer<ParamPack<Inst::Push>>*>(&objPkg.paramPool.param_data[0]);
        mergeRaster.objectPackages.push_back(&objPkg);

        mergeRaster.Compile();
        mergeRaster.Undefer(fri);

        pkgRecord.packages.push_back(reinterpret_cast<Command::InstructionPackage*>(&mergeRaster));

        const UsageData<Image> initial_acquire_usage{
            .stage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            .accessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
        };
        
        gpuTask.resources.AddResource<Image>(initial_acquire_usage);
    
        subTask.specializedSubmission = true;
        subTask.uses_present_image = true;

        gpuTask.GenerateWorkload();
    
        subTask.packaged_tasks.push_back(
            [&](EWE::CommandBuffer& cmdBuf, uint8_t frameIndex){
                VkRenderingAttachmentInfo presentAttachmentInfo{
                    .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                    .pNext = nullptr,
                    .imageView = engine->swapchain.GetCurrentImageView(),
                    .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    .resolveMode = VK_RESOLVE_MODE_NONE,
                    .resolveImageView = VK_NULL_HANDLE,
                    .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                    .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                    .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                    .clearValue = {0.f, 0.f, 0.f, 0.f}
                };
                auto& vri = mergeRaster.deferred_vk_render_info->GetRef(frameIndex);
                vri.colorAttachmentCount = 1;
                vri.pColorAttachments = &presentAttachmentInfo;

                gpuTask.workload(cmdBuf, frameIndex);
                return true;
            }
        );
    
    }


    void MergeTask::AddToRenderGraph(RenderGraph& renderGraph){
        renderGraph.syncManager.AddAcquisition<Image>(gpuTask, present_img_att_index);

        renderGraph.tasks.push_back(&gpuTask);

        renderGraph.swap_image_instances.emplace_back(&gpuTask, present_img_att_index);

        const UsageData<Image> merge_acquire_usage{
            .stage = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
            .accessMask = VK_ACCESS_2_SHADER_READ_BIT,
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
        };
        {
            uint8_t current_merge_index = 1; //present is 0, start at 1
            for(auto const& tm : textures_to_be_merged){
                renderGraph.syncManager.AddTransition<Image>(tm.lh, TaskResourceIndex{&gpuTask, current_merge_index});
                gpuTask.resources.AddResource(tm.lh.task->resources.images[tm.lh.index].resource, merge_acquire_usage);
                current_merge_index++;
            }
        }
        renderGraph.presentBridge.final_swap_img_usage = &gpuTask.resources.images[present_img_att_index];
        //the first package, [0], will be the present package
        //the second package, if available, is what adjust the push constant

        //shrink down to 1, removing all extra tasks?
        if(subTask.packaged_tasks.size() > 1){
            subTask.packaged_tasks.resize(1);
            
        }

        for_each_frame{
            for(uint8_t i = textures_to_be_merged.Size(); i < max_merge_count; i++){
                push_pack.GetRef(frame).GetTextureIndex(i) = null_texture;
            }
        }

        subTask.packaged_tasks.push_back(
            [&](CommandBuffer& cmdBuf, uint8_t frameIndex){

                uint8_t current_merge_index = 0;
                for(auto const& tm : textures_to_be_merged){
                    push_pack.GetRef(frameIndex).GetTextureIndex(current_merge_index) = tm.texIndex[frameIndex];
                    current_merge_index++;
                }

                //this is the most convenient place to put this right now, it could potentially be in it's own separate lambda/functio
                renderGraph.presentBridge.Execute(cmdBuf);

                return true;
            }
        );
    }
    
} //namespace EWE