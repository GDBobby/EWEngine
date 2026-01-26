//example
#include "EightWinds/VulkanHeader.h"

#include "EWEngine/EWEngine.h"
#include "EWEngine/Global.h"

#include "EightWinds/Framework.h"

#include "EightWinds/Pipeline/Layout.h"
#include "EightWinds/Pipeline/PipelineBase.h"
#include "EightWinds/Pipeline/Graphics.h"
#include "EightWinds/Shader.h"
#include "EightWinds/DescriptorImageInfo.h"

#include "EightWinds/RenderGraph/Command/Record.h"
#include "EightWinds/RenderGraph/Command/Execute.h"
#include "EightWinds/RenderGraph/GPUTask.h"

#include "EightWinds/GlobalPushConstant.h"

#include "EightWinds/Image.h"
#include "EightWinds/ImageView.h"

#include "EWEngine/NodeGraph/Graph.h"

#include "EWEngine/Tools/ImguiHandler.h"

#include "EightWinds/RenderGraph/RasterTask.h"

#include <cstdint>
#if EWE_DEBUG_BOOL
#include <cstdio>
#include <cassert>
#endif
#include <filesystem>
#include <chrono>
#include <array>
#include <thread>

#if EWE_DEBUG_BOOL
void PrintAllExtensions(VkPhysicalDevice physicalDevice) {
    uint32_t extCount = 0;
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extCount, nullptr);
    std::vector<VkExtensionProperties> extProps(extCount);
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extCount, extProps.data());
    printf("available extensions --\n");
    for (auto& prop : extProps) {
        printf("\t%s\n", prop.extensionName);
    }
}
#endif

int main() {

#if defined(__SANITIZE_ADDRESS__)
    printf("compiled with asan\n");
#endif

    EWE::EWEngine engine{ "triangle hello world" };
    EWE::LogicalDevice& logicalDevice = engine.logicalDevice;

    EWE::Queue* renderQueue = nullptr;
    for (auto& queue : logicalDevice.queues) {
        if (queue.family.SupportsSurfacePresent() && queue.family.SupportsGraphics()) {
            renderQueue = &queue;
            break;
        }
    }
    if (renderQueue == nullptr) {
#if EWE_DEBUG_BOOL
        printf("failed to find a render queue, exiting\n");
#endif
        std::this_thread::sleep_for(std::chrono::seconds(5));
        return -1;
    }
#if EWE_DEBUG_NAMING
    renderQueue->SetName("render queue");
#endif

#if EWE_DEBUG_BOOL
    printf("current dir - %s\n", std::filesystem::current_path().string().c_str());
#endif

    //need to fix htis. its something with my windows debugger
    auto current_working_directory = std::filesystem::current_path();
    if (current_working_directory.parent_path().parent_path().stem() == "build") {
        current_working_directory = current_working_directory.parent_path().parent_path().parent_path();
#if EWE_DEBUG_BOOL
        printf("build redacted working dir - %s\n", current_working_directory.string().c_str());
#endif
    }
    else if (current_working_directory.parent_path().stem() == "build") {
        current_working_directory = current_working_directory.parent_path().parent_path();
    }
    std::filesystem::current_path(current_working_directory);
#if EWE_DEBUG_BOOL
    printf("current dir - %s\n", std::filesystem::current_path().string().c_str());
#endif
    EWE::Framework framework(logicalDevice);

    EWE::ImguiHandler imguiHandler{ *renderQueue, 3, VK_SAMPLE_COUNT_1_BIT };

    //pipeline
    auto* triangle_vert = framework.shaderFactory.GetShader("examples/common/shaders/basic.vert.spv");
    auto* triangle_frag = framework.shaderFactory.GetShader("examples/common/shaders/basic.frag.spv");

    EWE::PipeLayout triangle_layout(logicalDevice, std::initializer_list<EWE::Shader*>{ triangle_vert, triangle_frag });
    //passconfig should be using a full rendergraph setup
    EWE::TaskRasterConfig passConfig;
    passConfig.SetDefaults();
    passConfig.depthStencilInfo.depthTestEnable = VK_FALSE;
    passConfig.depthStencilInfo.depthWriteEnable = VK_FALSE;
    passConfig.depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
    passConfig.depthStencilInfo.stencilTestEnable = VK_FALSE;

    auto& color_back = passConfig.color_att_info.emplace_back();
    color_back.format = VK_FORMAT_R8G8B8A8_UNORM;
    color_back.info.clearValue.color.float32[0] = 0.f;
    color_back.info.clearValue.color.float32[1] = 0.f;
    color_back.info.clearValue.color.float32[2] = 0.f;
    color_back.info.clearValue.color.float32[3] = 0.f;
    color_back.info.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_back.info.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    auto& depth_temp = passConfig.depth_att_info;
    depth_temp.info.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_temp.info.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_temp.info.clearValue.depthStencil.depth = 0.f;
    depth_temp.info.clearValue.depthStencil.stencil = 0; //idk what to set this to tbh

    EWE::RasterTask triangle_raster_task{"triangle raster task", logicalDevice, *renderQueue, passConfig, true};

    EWE::ObjectRasterData triangle_rasterObj;
    triangle_rasterObj.layout = &triangle_layout;
    triangle_rasterObj.config.SetDefaults();
    triangle_rasterObj.config.cullMode = VK_CULL_MODE_NONE;
    triangle_rasterObj.config.depthClamp = false;
    triangle_rasterObj.config.rasterizerDiscard = false;
    triangle_rasterObj.config.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;

    //std::vector<VkDynamicState> dynamicState{ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
     
    //VertexDrawData has a copy of a deferred<VertexParamPack> that will be automatically adjusted
    //also has a push constant, which will be automatically applied
    EWE::VertexDrawData triangle_drawData{};
    triangle_raster_task.AddDraw(triangle_rasterObj, triangle_drawData);

    EWE::CommandRecord triangleRecord{};
    triangle_raster_task.Record(triangleRecord);

    EWE::Node::Graph nodeGraph{};
    nodeGraph.Record(triangleRecord);

    EWE::RenderGraph& renderGraph = engine.renderGraph;
    EWE::GPUTask& renderTask = renderGraph.tasks.AddElement(logicalDevice, *renderQueue, triangleRecord, "render");

    triangle_raster_task.scissor = EWE::Global::window->screenDimensions;
    triangle_raster_task.viewport.x = 0.f;
    triangle_raster_task.viewport.y = static_cast<float>(EWE::Global::window->screenDimensions.height);
    triangle_raster_task.viewport.width = static_cast<float>(EWE::Global::window->screenDimensions.width);
    triangle_raster_task.viewport.height = -static_cast<float>(EWE::Global::window->screenDimensions.height);
    triangle_raster_task.viewport.minDepth = 0.0f;
    triangle_raster_task.viewport.maxDepth = 1.f;
    triangle_raster_task.AdjustPipelines();


    VmaAllocationCreateInfo vmaAllocInfo{
        .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO,
        .requiredFlags = 0,
        .preferredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        .memoryTypeBits = 0,
        .pool = VK_NULL_HANDLE,
        .pUserData = nullptr,
        .priority = 1.f
    };

    struct alignas(16) TriangleVertex {
        float pos[2]; //xy
        float color[3]; //rgb, the 4th element isnt read, but i need it for alignment
    };
    for (auto& str : triangle_vert->structData) {
        if (str.name == "Vertex") {

#if EWE_DEBUG_BOOL
            printf("size comparison - %zu : %zu\n", str.size, sizeof(TriangleVertex));
            //im getting fucked by spv. its forcing 32bit alignment, even tho spirvcross says 20bit
            //assert(str.size == sizeof(TriangleVertex));
#endif
        }
    }
    EWE::Buffer vertex_buffer{ logicalDevice, sizeof(TriangleVertex) * 3, 1, vmaAllocInfo, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT };
#if EWE_DEBUG_NAMING
    vertex_buffer.SetName("vertex buffer");
#endif
    {
        TriangleVertex* mappedData = reinterpret_cast<TriangleVertex*>(vertex_buffer.Map());

        mappedData[0].pos[0] = -0.5f;
        mappedData[0].pos[1] = -0.5f;

        mappedData[0].color[0] = 1.f;
        mappedData[0].color[1] = 0.f;
        mappedData[0].color[2] = 0.f;

        mappedData[1].pos[0] = 0.f;
        mappedData[1].pos[1] = 0.5f;

        mappedData[1].color[0] = 0.f;
        mappedData[1].color[1] = 1.f;
        mappedData[1].color[2] = 0.f;

        mappedData[2].pos[0] = 0.5f;
        mappedData[2].pos[1] = -0.5f;

        mappedData[2].color[0] = 0.f;
        mappedData[2].color[1] = 0.f;
        mappedData[2].color[2] = 1.f;

        vertex_buffer.Flush();
        vertex_buffer.Unmap();
    }

    triangle_drawData.buffers[0] = &vertex_buffer;
    for (uint8_t i = 0; i < EWE::max_frames_in_flight; i++) {
        triangle_drawData.UpdateBuffer();
        triangle_drawData.paramPack->GetRef(i).firstInstance = 0;
        triangle_drawData.paramPack->GetRef(i).firstVertex = 0;
        triangle_drawData.paramPack->GetRef(i).instanceCount = 1;
        triangle_drawData.paramPack->GetRef(i).vertexCount = 3;
    }

    EWE::VertexDrawData merge_drawData{};
    passConfig.pipelineRenderingCreateInfo.pColorAttachmentFormats = &engine.swapchain.swapCreateInfo.imageFormat;
    auto* merge_vert = framework.shaderFactory.GetShader("examples/common/shaders/merge.vert.spv");
    auto* merge_frag = framework.shaderFactory.GetShader("examples/common/shaders/merge.frag.spv");
    EWE::PipeLayout merge_layout(logicalDevice, std::initializer_list<EWE::Shader*>{ merge_vert, merge_frag });
    /*    
    EWE::HeapBlock<EWE::CommandRecord> mergeRecords{
        engine.swapchain.available_surface_formats.size() 
    };
    EWE::HeapBlock<EWE::GPUTask*> mergeTasks{
        engine.swapchain.available_surface_formats.size()
    };// = renderGraph.tasks.AddElement(logicalDevice, *renderQueue, mergeRecord, "merge");
    EWE::HeapBlock<EWE::RasterTask> mergeRasters{
        engine.swapchain.available_surface_formats.size()
    };

    for (uint8_t i = 0; i < engine.swapchain.available_surface_formats.size(); i++) {
        mergeRecords.ConstructAt(i);

        auto& format = engine.swapchain.available_surface_formats[i];
        passConfig.color_att_info[0].format = format.format;
        mergeRasters.ConstructAt(i, "merge raster", logicalDevice, *renderQueue, passConfig, false);
        mergeRasters[i].AddDraw(merge_rasterObj, merge_drawData);

        mergeRasters[i].Record(mergeRecords[i]);

        mergeTasks.ConstructAt(i, &renderGraph.tasks.AddElement(logicalDevice, *renderQueue, mergeRecords[i], "merge"));
        //mergePipelines.emplace_back(new EWE::GraphicsPipeline(logicalDevice, 0, &triangle_layout, passConfig, objectConfig, dynamicState));
    }
    */
    EWE::CommandRecord mergeRecord{};
    EWE::GPUTask& mergeTask{ renderGraph.tasks.AddElement(logicalDevice, *renderQueue, mergeRecord, "merge") };
    EWE::RasterTask mergeRaster{"merge raster", logicalDevice, *renderQueue, passConfig, false};
    
    EWE::ObjectRasterData merge_rasterObj;
    merge_rasterObj.config = triangle_rasterObj.config;
    merge_rasterObj.layout = &merge_layout;

    for (uint8_t i = 0; i < EWE::max_frames_in_flight; i++) {
        merge_drawData.paramPack->GetRef(i).firstInstance = 0;
        merge_drawData.paramPack->GetRef(i).firstVertex = 0;
        merge_drawData.paramPack->GetRef(i).vertexCount = 4;
        merge_drawData.paramPack->GetRef(i).instanceCount = 1;
    }

    auto& first_node = nodeGraph.AddNode("First Node");
    first_node.buffer->position = lab::vec2(0.f);
    first_node.buffer->scale = lab::vec2(1.f);
    first_node.buffer->backgroundColor = lab::vec3(0.f, 1.f, 0.f);
    first_node.buffer->foregroundColor = lab::vec3(1.f, 0.f, 0.f);
    first_node.buffer->foregroundScale = 0.99f;
    first_node.buffer->titleColor = lab::vec3(0.f, 0.f, 1.f);
    first_node.buffer->titleScale = 0.2f;

    nodeGraph.InitializeRender();

    EWE::Input::Mouse mouseData{};
    mouseData.TakeCallbackControl();

    EWE::UsageData<EWE::Image> initial_acquire_usage{
        .stage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        .accessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    uint32_t render_att_index = renderTask.resources.AddResource(triangle_raster_task.renderTracker->full.color_images[0], initial_acquire_usage);
    renderGraph.syncManager.AddAcquisition_Image(renderTask, render_att_index);
    uint32_t present_img_att_index = mergeTask.resources.AddResource<EWE::Image>(initial_acquire_usage);
    renderGraph.syncManager.AddAcquisition_Image(mergeTask, present_img_att_index);

    EWE::GPUTask& imguiTask{ renderGraph.tasks.AddElement(logicalDevice, *renderQueue, "imgui") };

    uint32_t imgui_att_index = imguiTask.resources.AddResource(imguiHandler.renderTracker.full.color_images[0], initial_acquire_usage);

    EWE::UsageData<EWE::Image> merge_acquire_usage{
        .stage = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
        .accessMask = VK_ACCESS_2_SHADER_READ_BIT,
        .layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };



    uint32_t acquire_render_output_index = mergeTask.resources.AddResource(mergeRaster.renderTracker->full.color_images[0], merge_acquire_usage);
    renderGraph.syncManager.AddTransition_Image(renderTask, render_att_index, mergeTask, acquire_render_output_index);
    uint32_t acquire_imgui_output_index = mergeTask.resources.AddResource(imguiHandler.renderTracker.full.color_images[0], merge_acquire_usage);
    renderGraph.syncManager.AddTransition_Image(imguiTask, imgui_att_index, mergeTask, acquire_imgui_output_index);

    renderGraph.presentBridge.SetSubresource(
        VkImageSubresourceRange{
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    );

    EWE::SubmissionTask world_render_submission{ *EWE::Global::logicalDevice, *renderQueue, true, "world render"};
    EWE::SubmissionTask imgui_submission{ *EWE::Global::logicalDevice, *renderQueue, true, "imgui"};

    imguiHandler.SetSubmissionData(imgui_submission.submitInfo);
    imgui_submission.external_workload = [](EWE::Backend::SubmitInfo&, uint8_t frameIndex) {
        return true;
    };
    EWE::SubmissionTask attachment_blit_submission{ *EWE::Global::logicalDevice, *renderQueue, true, "attachment blit"};

    for (uint8_t i = 0; i < EWE::max_frames_in_flight; i++) {
        world_render_submission.submitInfo[i].signalSemaphores.push_back(
            VkSemaphoreSubmitInfo{
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                .pNext = nullptr,
                .semaphore = world_render_submission.signal_semaphores[i],
                .value = 0,
                .stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                .deviceIndex = 0
            }
        );
        attachment_blit_submission.submitInfo[i].signalSemaphores.push_back(
            VkSemaphoreSubmitInfo{
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                .pNext = nullptr,
                .semaphore = engine.swapchain.GetCurrentPresentSemaphore(),
                .value = 0,
                .stageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                .deviceIndex = 0
            }
        );

        world_render_submission.submitInfo[i].AddCommandBuffer(world_render_submission.cmdBuffers[i]);
        attachment_blit_submission.submitInfo[i].AddCommandBuffer(attachment_blit_submission.cmdBuffers[i]);
    }

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

    EWE::Sampler attachmentSampler{ logicalDevice, samplerCreateInfo };

    EWE::PerFlight<EWE::DescriptorImageInfo> world_attachment_descriptor(
        EWE::DescriptorImageInfo{logicalDevice, attachmentSampler, triangle_raster_task.renderTracker->full.color_views[0][0], EWE::DescriptorType::Combined, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
        EWE::DescriptorImageInfo{logicalDevice, attachmentSampler, triangle_raster_task.renderTracker->full.color_views[0][1], EWE::DescriptorType::Combined, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}
    );
    EWE::PerFlight<EWE::DescriptorImageInfo> imgui_attachment_descriptor(
        EWE::DescriptorImageInfo{logicalDevice, attachmentSampler, imguiHandler.renderTracker.full.color_views[0][0], EWE::DescriptorType::Combined, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
        EWE::DescriptorImageInfo{logicalDevice, attachmentSampler, imguiHandler.renderTracker.full.color_views[0][1], EWE::DescriptorType::Combined, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}
    );

    std::size_t currentMergeTaskIndex = 0;

    attachment_blit_submission.full_workload = [&](EWE::CommandBuffer& cmdBuf, uint8_t frameIndex) {
        //i dont really know how to bridge tasks in different submisisons yet
        //fully explicit right now
        EWE::Image& presentImage = engine.swapchain.GetCurrentImage();
        mergeTask.prefix.imageAcquisitions.back().rhs[frameIndex].resource = &presentImage;

        merge_drawData.textures[0] = &world_attachment_descriptor[frameIndex];
        merge_drawData.textures[1] = &imgui_attachment_descriptor[frameIndex];

        merge_drawData.UpdateBuffer();

        EWE::ImageView present_view{ presentImage };
        mergeRaster.renderTracker->compact.color_attachments.clear();
        mergeRaster.renderTracker->vk_data[frameIndex].colorAttachmentInfo.clear();
        mergeRaster.renderTracker->compact.color_attachments.push_back(
            EWE::SimplifiedAttachment{
                .imageView = {&present_view},
                .info = EWE::AttachmentInfo{
                    .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                    .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                    .clearValue = {0.f, 0.f, 0.f, 0.f}
                }
            }
        );
        mergeRaster.renderTracker->compact.depth_attachment.imageView[EWE::Global::frameIndex] = nullptr;
        mergeRaster.renderTracker->CascadeFull();

        
        //if (currentSwapchainFormat != engine.swapchain.surface_format.format) {
        //    uint8_t swapchainFormatIndex = 0;
        //    currentSwapchainFormat = engine.swapchain.surface_format.format;
        //    for (uint8_t i = 0; i < engine.swapchain.available_surface_formats.size(); i++) {
        //        if (currentSwapchainFormat == engine.swapchain.available_surface_formats[i].format) {
        //            swapchainFormatIndex = i;
        //            break;
        //        }
        //    }
        //    currentMergeTaskIndex = swapchainFormatIndex;
        //}

        mergeTask.Execute(cmdBuf, frameIndex);
        renderGraph.presentBridge.UpdateSrcData(&mergeTask.queue, &mergeTask.prefix.imageAcquisitions.back().rhs[frameIndex], frameIndex);
        renderGraph.presentBridge.Execute(cmdBuf);
        return true;
    };
    

    renderGraph.execution_order = {
        std::vector<EWE::SubmissionTask*>{&imgui_submission, &world_render_submission},
        std::vector<EWE::SubmissionTask*>{&attachment_blit_submission}
    };

    for (uint8_t i = 0; i < EWE::max_frames_in_flight; i++) {
        renderGraph.presentSubmission.incomingSemaphores[i].push_back(engine.swapchain.GetCurrentPresentSemaphore());
        attachment_blit_submission.submitInfo[i].WaitOnPrevious(imgui_submission.submitInfo[i]);
        attachment_blit_submission.submitInfo[i].WaitOnPrevious(world_render_submission.submitInfo[i]);

        attachment_blit_submission.submitInfo[i].waitSemaphores.push_back(
            VkSemaphoreSubmitInfo{
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                .pNext = nullptr,
                //just an initial value, this is overwritten every frame
                .semaphore = engine.swapchain.acquire_semaphores[0].vkSemaphore,
                .value = 0,
                .stageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,//theres prob a better option
                .deviceIndex = 0
            }

        );
    }


    try { //beginning of render loop
        auto timeBegin = std::chrono::high_resolution_clock::now();
        VkDescriptorImageInfo descImg;
        std::chrono::nanoseconds elapsedTime = std::chrono::nanoseconds(0);
        constexpr auto frameDuration = std::chrono::duration<double>(1.0 / 60.0); // seconds per frame
        while (true) {
            const auto timeEnd = std::chrono::high_resolution_clock::now();
            elapsedTime += timeEnd - timeBegin;
            timeBegin = timeEnd;
            if (elapsedTime >= frameDuration) {

                glfwPollEvents();

                if (renderGraph.Acquire(EWE::Global::frameIndex)) {
                    auto& swapImage = engine.swapchain.GetCurrentImage();

                    auto& waitSemInfos = attachment_blit_submission.submitInfo[EWE::Global::frameIndex].waitSemaphores;
                    waitSemInfos.back().semaphore = engine.swapchain.GetAcquireSemaphore(EWE::Global::frameIndex);
                    auto& signalSemInfos = attachment_blit_submission.submitInfo[EWE::Global::frameIndex].signalSemaphores;
                    signalSemInfos.back().semaphore = engine.swapchain.GetCurrentPresentSemaphore();

                    renderGraph.presentSubmission.incomingSemaphores[EWE::Global::frameIndex].back() = engine.swapchain.GetCurrentPresentSemaphore();

                    //i think i also want the previous frame's submit signal to be waited on here, but idk
                    //auto& secondBackSemInfo = waitSemInfos[waitSemInfos.size() - 2].semaphore = engine.swapchain.GetCurrentSemaphores().present

                    nodeGraph.UpdateRender(mouseData, EWE::Global::frameIndex);
#ifdef EWE_IMGUI
                    imguiHandler.BeginRender();
                    //nodeGraph.Imgui();
                    //engine.Imgui();
                    //ImGui::ShowDemoWindow();
                    imguiHandler.EndRender();
#endif
                    renderGraph.Execute(EWE::Global::frameIndex);
                    renderGraph.presentSubmission.Present(EWE::Global::frameIndex);


                    EWE::Global::frameIndex = (EWE::Global::frameIndex + 1) % EWE::max_frames_in_flight;
                    engine.totalFramesSubmitted++;
                }
                else {
                }
                elapsedTime = std::chrono::nanoseconds(0);
            }
        }
    }
    catch (EWE::EWEException& except) {
        framework.HandleVulkanException(except);
#if EWE_DEBUG_BOOL
        assert(false && "caught exception");
#endif
    }


#if EWE_DEBUG_BOOL
    printf("returning successfully\n");
#endif

    delete triangle_vert;
    delete triangle_frag;


    std::this_thread::sleep_for(std::chrono::seconds(5)); 
    return 0;
}