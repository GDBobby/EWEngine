#include "EWEngine/Graphics/SpecializedTasks/ImguiTask.h"
#include "EWEngine/Global.h"
#include "EightWinds/Data/ForwardArgConstructionHelper.h"

namespace EWE{

    Sampler& GetImguiAttachmentSampler(){
        VkSamplerCreateInfo samplerCreateInfo{
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .magFilter = VK_FILTER_NEAREST,
            .minFilter = VK_FILTER_NEAREST,
            .mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
            .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
            .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
            .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
            .mipLodBias = 0.f,
            .anisotropyEnable = VK_FALSE,
            .maxAnisotropy = 0.f,
            .compareEnable = VK_FALSE,
            .compareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
            .minLod = 0.f,
            .maxLod = 1.f,
            .borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK,
            .unnormalizedCoordinates = VK_FALSE
        };

        return EWE::Global::assetManager->sampler.Get(samplerCreateInfo);
    }

    ImguiTask::ImguiTask()
    : gpuTask{"imgui", engine->logicalDevice, engine->renderQueue, Command::ParamPool{}},
        subTask{"imgui", engine->logicalDevice, engine->renderQueue},
        attachment_diis{
            ArgumentPack_ConstructionHelper<4>{},
            GetImguiAttachmentSampler(),
            *EWE::Global::imguiHandler->renderInfo.full.color_views[0][0],
            EWE::DescriptorType::Combined, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        
            GetImguiAttachmentSampler(),
            *EWE::Global::imguiHandler->renderInfo.full.color_views[0][1],
            EWE::DescriptorType::Combined, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        }
    {
        EWE_ASSERT(Global::imguiTask == nullptr);
        Global::imguiTask = this;

        PerFlight<Image*> temp_att_res{
            &Global::imguiHandler->renderInfo.full.color_views[0][0]->image,
            &Global::imguiHandler->renderInfo.full.color_views[0][1]->image
        };
        const UsageData<Image> initial_acquire_usage{
            .stage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            .accessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
        };
        gpuTask.resources.AddResource(temp_att_res, initial_acquire_usage);
        
        subTask.specializedSubmission = true;

        subTask.packaged_tasks.push_back(
            [&](CommandBuffer& cmdBuf, uint8_t frameIndex) {
                gpuTask.prefix.Execute(cmdBuf, frameIndex);
                Global::imguiHandler->Render(cmdBuf);
                return true;
            }
        );
    }

    void ImguiTask::AddToRenderGraph(RenderGraph& renderGraph){
        //anything necessary here?
        renderGraph.syncManager.AddAcquisition<Image>(gpuTask, color_attachment_index);
        renderGraph.tasks.push_back(&gpuTask);
    }
} //namespace EWE