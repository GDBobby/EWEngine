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
#include "EightWinds/RenderGraph/TaskBridge.h"

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

    //if either of these formats are changed, passConfig needs to be changed as well. these just happen to match the defaults
    VkFormat colorFormat = VK_FORMAT_R8G8B8A8_UNORM;
    VkFormat depthFormat = VK_FORMAT_D16_UNORM;

    EWE::PerFlight<EWE::Image> colorAttachmentImages{ logicalDevice };
    EWE::PerFlight<EWE::Image> depthAttachmentImages{ logicalDevice };

    //most of the pnext seem useless. some other useful stuff, 
    /*
    VkOpaqueCaptureDescriptorDataCreateInfoEXT,
    VkExternalMemoryImageCreateInfo
    VkImageFormatListCreateInfo,
    */
    for (uint8_t i = 0; i < EWE::max_frames_in_flight; i++) {
        colorAttachmentImages[i].arrayLayers = 1;
        depthAttachmentImages[i].arrayLayers = 1;
        colorAttachmentImages[i].extent = { EWE::Global::window->screenDimensions.width, EWE::Global::window->screenDimensions.height, 1 };
        depthAttachmentImages[i].extent = { EWE::Global::window->screenDimensions.width, EWE::Global::window->screenDimensions.height, 1 };
        colorAttachmentImages[i].mipLevels = 1;
        depthAttachmentImages[i].mipLevels = 1;
        colorAttachmentImages[i].owningQueue = renderQueue;
        depthAttachmentImages[i].owningQueue = renderQueue;
        colorAttachmentImages[i].samples = VK_SAMPLE_COUNT_1_BIT;
        depthAttachmentImages[i].samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachmentImages[i].tiling = VK_IMAGE_TILING_OPTIMAL;
        depthAttachmentImages[i].tiling = VK_IMAGE_TILING_OPTIMAL;
        colorAttachmentImages[i].type = VK_IMAGE_TYPE_2D;
        depthAttachmentImages[i].type = VK_IMAGE_TYPE_2D;
    }

    VmaAllocationCreateInfo vmaAllocCreateInfo{};
    vmaAllocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
    //if(imageCreateInfo.width * height > some amount){
    vmaAllocCreateInfo.flags = static_cast<VmaAllocationCreateFlags>(VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT) | static_cast<VmaAllocationCreateFlags>(VMA_ALLOCATOR_CREATE_EXTERNALLY_SYNCHRONIZED_BIT);
    //}
    for (auto& cai : colorAttachmentImages) {
        cai.format = colorFormat;
        cai.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        cai.Create(vmaAllocCreateInfo);
    }
    for (auto& dai : depthAttachmentImages) {
        dai.format = depthFormat;
        dai.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        dai.Create(vmaAllocCreateInfo);
    }
#if EWE_DEBUG_NAMING
    colorAttachmentImages[0].SetName("cai 0");
    colorAttachmentImages[1].SetName("cai 1");
    depthAttachmentImages[0].SetName("dai 0");
    depthAttachmentImages[1].SetName("dai 1");
#endif


    EWE::ImguiHandler imguiHandler{ *renderQueue, 3, VK_SAMPLE_COUNT_1_BIT };

    //before getting into the render, the layouts of the attachments need to be transitioned
    {
        EWE::CommandPool stc_cmdPool{ logicalDevice, *renderQueue, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT };

        VkCommandBufferAllocateInfo cmdBufAllocInfo{};
        cmdBufAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdBufAllocInfo.pNext = nullptr;
        cmdBufAllocInfo.commandBufferCount = 1;
        cmdBufAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmdBufAllocInfo.commandPool = stc_cmdPool.commandPool;

        VkCommandBuffer temp_stc_cmdBuf;
        EWE::EWE_VK(vkAllocateCommandBuffers, logicalDevice.device, &cmdBufAllocInfo, &temp_stc_cmdBuf);
        stc_cmdPool.allocatedBuffers++;

        EWE::CommandBuffer transition_stc(stc_cmdPool, temp_stc_cmdBuf);
        VkCommandBufferBeginInfo beginSTCInfo{};
        beginSTCInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginSTCInfo.pNext = nullptr;
        beginSTCInfo.pInheritanceInfo = nullptr;
        beginSTCInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        transition_stc.Begin(beginSTCInfo);

        std::vector<VkImageMemoryBarrier2> transition_barriers(4);

        uint64_t current_barrier_index = 0;

        for (auto& cai : colorAttachmentImages) {
            transition_barriers[current_barrier_index].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
            transition_barriers[current_barrier_index].pNext = nullptr;
            transition_barriers[current_barrier_index].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            transition_barriers[current_barrier_index].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            transition_barriers[current_barrier_index].srcAccessMask = VK_ACCESS_2_NONE;
            transition_barriers[current_barrier_index].dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
            transition_barriers[current_barrier_index].image = cai.image;
            transition_barriers[current_barrier_index].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            transition_barriers[current_barrier_index].newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            transition_barriers[current_barrier_index].subresourceRange = EWE::ImageView::GetDefaultSubresource(cai);
            transition_barriers[current_barrier_index].srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
            transition_barriers[current_barrier_index].dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

            current_barrier_index++;
        }
        for (auto& dai : depthAttachmentImages) {
            transition_barriers[current_barrier_index].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
            transition_barriers[current_barrier_index].pNext = nullptr;
            transition_barriers[current_barrier_index].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            transition_barriers[current_barrier_index].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            transition_barriers[current_barrier_index].srcAccessMask = VK_ACCESS_2_NONE;
            transition_barriers[current_barrier_index].dstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            transition_barriers[current_barrier_index].image = dai.image;
            transition_barriers[current_barrier_index].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            transition_barriers[current_barrier_index].newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
            transition_barriers[current_barrier_index].subresourceRange = EWE::ImageView::GetDefaultSubresource(dai);
            transition_barriers[current_barrier_index].srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
            transition_barriers[current_barrier_index].dstStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT;

            current_barrier_index++;
        }

        VkDependencyInfo transition_dependency{};
        transition_dependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        transition_dependency.pNext = nullptr;
        transition_dependency.dependencyFlags = 0;
        transition_dependency.bufferMemoryBarrierCount = 0;
        transition_dependency.memoryBarrierCount = 0;
        transition_dependency.imageMemoryBarrierCount = static_cast<uint32_t>(transition_barriers.size());
        transition_dependency.pImageMemoryBarriers = transition_barriers.data();

        vkCmdPipelineBarrier2(transition_stc, &transition_dependency);

        transition_stc.End();

        VkFenceCreateInfo fenceCreateInfo{
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0
        };
        EWE::Fence stc_fence{ logicalDevice, fenceCreateInfo };

        VkSubmitInfo stc_submit_info{
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .pNext = nullptr,
            .waitSemaphoreCount = 0,
            .pWaitDstStageMask = nullptr,
            .commandBufferCount = 1,
            .pCommandBuffers = &temp_stc_cmdBuf,
            .signalSemaphoreCount = 0
        };

        renderQueue->Submit(1, &stc_submit_info, stc_fence);

        transition_stc.state = EWE::CommandBuffer::State::Pending;

        EWE::EWE_VK(vkWaitForFences, logicalDevice.device, 1, &stc_fence.vkFence, VK_TRUE, 5 * static_cast<uint64_t>(1.0e9));

        for (auto& cai : colorAttachmentImages) {
            cai.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        }
        for (auto& dai : depthAttachmentImages) {
            dai.layout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
        }
    }

    EWE::PerFlight<EWE::ImageView> colorAttViews{ colorAttachmentImages };
    EWE::PerFlight<EWE::ImageView> depthAttViews{ depthAttachmentImages };

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

    EWE::RasterTask triangle_raster_task{"triangle raster task", logicalDevice, *renderQueue, passConfig, true};

    EWE::ObjectRasterData triangle_rasterObj;
    EWE::ObjectRasterConfig& objectConfig = triangle_rasterObj.config;
    objectConfig.SetDefaults();
    objectConfig.cullMode = VK_CULL_MODE_NONE;
    objectConfig.depthClamp = false;
    objectConfig.rasterizerDiscard = false;
    objectConfig.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;

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

    auto& color_att = renderTask.renderTracker->compact.color_attachments.emplace_back();
    color_att.imageView[0] = &colorAttViews[0];
    color_att.imageView[1] = &colorAttViews[1];
    color_att.clearValue.color.float32[0] = 0.f;
    color_att.clearValue.color.float32[1] = 0.f;
    color_att.clearValue.color.float32[2] = 0.f;
    color_att.clearValue.color.float32[3] = 0.f;
    color_att.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_att.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    auto& depth_att = renderTask.renderTracker->compact.depth_attachment;
    depth_att.imageView[0] = &depthAttViews[0];
    depth_att.imageView[1] = &depthAttViews[1];
    depth_att.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_att.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_att.clearValue.depthStencil.depth = 0.f;
    depth_att.clearValue.depthStencil.stencil = 0; //idk what to set this to tbh
    
    renderTask.SetRenderInfo();


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
    def_push->GetRef().buffer_addr[0] = vertex_buffer.deviceAddress;
    def_draw->GetRef().firstInstance = 0;
    def_draw->GetRef().firstVertex = 0;
    def_draw->GetRef().instanceCount = 1;
    def_draw->GetRef().vertexCount = 3;
    

    EWE::CommandRecord mergeRecord{};
    mergeRecord.BeginRender();
    auto* deferred_merge_pipe = mergeRecord.BindPipeline();
    auto* def_merge_vp_scissor = mergeRecord.SetViewportScissor();
    mergeRecord.BindDescriptor();
    auto* deferred_merge_push = mergeRecord.Push();
    auto deferred_merge_draw = mergeRecord.Draw();
    mergeRecord.EndRender();
    EWE::GPUTask& mergeTask = renderGraph.tasks.AddElement(logicalDevice, *renderQueue, mergeRecord, "merge");

    def_merge_vp_scissor->GetRef().scissor = EWE::Global::window->screenDimensions;
    def_merge_vp_scissor->GetRef().viewport.x = 0.f;
    def_merge_vp_scissor->GetRef().viewport.y = static_cast<float>(EWE::Global::window->screenDimensions.height);
    def_merge_vp_scissor->GetRef().viewport.width = static_cast<float>(EWE::Global::window->screenDimensions.width);
    def_merge_vp_scissor->GetRef().viewport.height = -static_cast<float>(EWE::Global::window->screenDimensions.height);
    def_merge_vp_scissor->GetRef().viewport.minDepth = 0.0f;
    def_merge_vp_scissor->GetRef().viewport.maxDepth = 1.f;

    auto* merge_vert = framework.shaderFactory.GetShader("examples/common/shaders/merge.vert.spv");
    auto* merge_frag = framework.shaderFactory.GetShader("examples/common/shaders/merge.frag.spv");
    EWE::PipeLayout merge_layout(logicalDevice, std::initializer_list<EWE::Shader*>{ merge_vert, merge_frag });
    //passconfig should be using a full rendergraph setup
    //just a coincidence that object and passconfig are identical to the earlier pipe
    passConfig.pipelineRenderingCreateInfo.pColorAttachmentFormats = &engine.swapchain.swapCreateInfo.imageFormat;

    std::vector<EWE::GraphicsPipeline*> mergePipelines{};
    mergePipelines.reserve(engine.swapchain.available_surface_formats.size());
    {
        uint8_t whichMerge = 0;
        for (auto& format : engine.swapchain.available_surface_formats) {
            passConfig.colorAttachmentFormats[0] = format.format;
            mergePipelines.emplace_back(new EWE::GraphicsPipeline(logicalDevice, 0, &triangle_layout, passConfig, objectConfig, dynamicState));

#if EWE_DEBUG_NAMING
            std::string debugName = "merge" + std::to_string(whichMerge);
            mergePipelines[whichMerge]->SetDebugName("merge");
            whichMerge++;
#endif
        }
    }

    VkFormat currentSwapchainFormat = engine.swapchain.surface_format.format;
    {
        uint8_t swapchainFormatIndex = 0;
        for (uint8_t i = 0; i < engine.swapchain.available_surface_formats.size(); i++) {
            if (currentSwapchainFormat == engine.swapchain.available_surface_formats[i].format) {
                swapchainFormatIndex = i;
                break;
            }
        }
        deferred_merge_pipe->GetRef().pipe = mergePipelines[swapchainFormatIndex]->vkPipe;
    }
    deferred_merge_pipe->GetRef().bindPoint = merge_layout.bindPoint;
    deferred_merge_pipe->GetRef().layout = merge_layout.vkLayout;

    deferred_merge_push->GetRef().Reset();

    deferred_merge_draw->GetRef().firstInstance = 0;
    deferred_merge_draw->GetRef().firstVertex = 0;
    deferred_merge_draw->GetRef().vertexCount = 4;
    deferred_merge_draw->GetRef().instanceCount = 1;

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

    int colorAttOutputPin = renderTask.AddImagePin(nullptr, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    int merge_worldSrcPin = mergeTask.AddImagePin(&colorAttachmentImages[EWE::Global::frameIndex], VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    int merge_DstPin = mergeTask.AddImagePin(nullptr, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    int merge_imguiSrcPin = mergeTask.AddImagePin(&imguiHandler.colorAttachmentImages[EWE::Global::frameIndex], VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

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
    world_render_submission.full_workload = [&](EWE::CommandBuffer& cmdBuf) {
        EWE::TaskBridge renderPrefix(renderTask);
        renderPrefix.GenerateRightHandBarriers(renderTask.explicitBufferState, renderTask.explicitImageState, renderTask.queue);
        renderPrefix.Execute(cmdBuf);
        renderTask.Execute(cmdBuf);
        return true;
    };
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
        EWE::DescriptorImageInfo{logicalDevice, attachmentSampler, colorAttViews[0], EWE::DescriptorType::Combined, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
        EWE::DescriptorImageInfo{logicalDevice, attachmentSampler, colorAttViews[1], EWE::DescriptorType::Combined, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}
    );
    EWE::PerFlight<EWE::DescriptorImageInfo> imgui_attachment_descriptor(
        EWE::DescriptorImageInfo{logicalDevice, attachmentSampler, imguiHandler.colorAttachmentViews[0], EWE::DescriptorType::Combined, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
        EWE::DescriptorImageInfo{logicalDevice, attachmentSampler, imguiHandler.colorAttachmentViews[1], EWE::DescriptorType::Combined, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}
    );


    attachment_blit_submission.full_workload = [&](EWE::CommandBuffer& cmdBuf) {
        //i dont really know how to bridge tasks in different submisisons yet
        //fully explicit right now
        EWE::Image& presentImage = engine.swapchain.GetCurrentImage();
        mergeTask.SetResource(merge_DstPin, presentImage);

        EWE::TaskBridge renderBridge{ renderTask, mergeTask };
        renderBridge.RecreateBarriers();

        std::vector<EWE::Resource<EWE::Buffer>*> bufferResources{}; //fake, its a reference

        EWE::Resource<EWE::Image> imgui_attachment{
            .image = &imguiHandler.colorAttachmentImages[EWE::Global::frameIndex],
            .usage = EWE::UsageData<EWE::Image>{
                .stage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                .accessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
            }
        };
        std::vector<EWE::Resource<EWE::Image>*> imguiResources{
            &imgui_attachment
        };
        EWE::TaskBridge imguiBridge{
            logicalDevice, mergeTask.queue,
            bufferResources, bufferResources,
            imguiResources, mergeTask.explicitImageState,
            imguiHandler.queue, mergeTask.queue
        };

        EWE::Resource<EWE::Image> lh_resourceImage{
            .image = &presentImage,
            .usage = EWE::UsageData<EWE::Image>{
                .stage = VK_PIPELINE_STAGE_2_NONE,
                .accessMask = VK_ACCESS_2_NONE,
                .layout = VK_IMAGE_LAYOUT_UNDEFINED
            }
        };
        EWE::Resource<EWE::Image> rh_resourceImage{
            .image = &presentImage,
            .usage = EWE::UsageData<EWE::Image>{
                .stage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                .accessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
            }
        };

        deferred_merge_push->GetRef().texture_indices[0] = world_attachment_descriptor[EWE::Global::frameIndex].index;
        deferred_merge_push->GetRef().texture_indices[1] = imgui_attachment_descriptor[EWE::Global::frameIndex].index;

        std::vector<EWE::Resource<EWE::Image>*> lh_presentation_image{
            &lh_resourceImage
        };
        std::vector<EWE::Resource<EWE::Image>*> rh_presentation_image{
            &rh_resourceImage
        };

        EWE::TaskBridge mergePrefix{
            logicalDevice, mergeTask.queue,
            bufferResources, bufferResources,
            lh_presentation_image, rh_presentation_image,
            *renderQueue, *renderQueue
        };

        renderBridge.Execute(cmdBuf);
        imguiBridge.Execute(cmdBuf);

        mergePrefix.Execute(cmdBuf);

        EWE::ImageView present_view{ presentImage };
        mergeTask.renderTracker->compact.color_attachments.clear();
        mergeTask.renderTracker->vk_data.colorAttachmentInfo.clear();
        mergeTask.renderTracker->compact.color_attachments.push_back(
            EWE::SimplifiedAttachment{
                .imageView = {&present_view, &present_view},
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                .clearValue = {0.f, 0.f, 0.f, 0.f}
            }
        );
        mergeTask.renderTracker->compact.depth_attachment.imageView[EWE::Global::frameIndex] = nullptr;
        mergeTask.SetRenderInfo();
        mergeTask.UpdateFrameIndex(EWE::Global::frameIndex);

        
        if (currentSwapchainFormat != engine.swapchain.surface_format.format) {
            uint8_t swapchainFormatIndex = 0;
            currentSwapchainFormat = engine.swapchain.surface_format.format;
            for (uint8_t i = 0; i < engine.swapchain.available_surface_formats.size(); i++) {
                if (currentSwapchainFormat == engine.swapchain.available_surface_formats[i].format) {
                    swapchainFormatIndex = i;
                    break;
                }
            }
            deferred_merge_pipe->GetRef().pipe = mergePipelines[swapchainFormatIndex]->vkPipe;
        }

        mergeTask.Execute(cmdBuf);
        renderGraph.presentBridge.UpdateSrcData(&mergeTask.queue, mergeTask.explicitImageState[merge_DstPin]);
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
                    auto swapImage = engine.swapchain.GetCurrentImage();

                    renderTask.SetResource(colorAttOutputPin, colorAttachmentImages[EWE::Global::frameIndex]);
                    mergeTask.SetResource(merge_worldSrcPin, colorAttachmentImages[EWE::Global::frameIndex]);
                    mergeTask.SetResource(merge_imguiSrcPin, imguiHandler.colorAttachmentImages[EWE::Global::frameIndex]);

                    auto& waitSemInfos = attachment_blit_submission.submitInfo[EWE::Global::frameIndex].waitSemaphores;
                    waitSemInfos.back().semaphore = engine.swapchain.GetAcquireSemaphore(EWE::Global::frameIndex);
                    auto& signalSemInfos = attachment_blit_submission.submitInfo[EWE::Global::frameIndex].signalSemaphores;
                    signalSemInfos.back().semaphore = engine.swapchain.GetCurrentPresentSemaphore();

                    renderGraph.presentSubmission.incomingSemaphores[EWE::Global::frameIndex].back() = engine.swapchain.GetCurrentPresentSemaphore();

                    //i think i also want the previous frame's submit signal to be waited on here, but idk
                    //auto& secondBackSemInfo = waitSemInfos[waitSemInfos.size() - 2].semaphore = engine.swapchain.GetCurrentSemaphores().present

                    nodeGraph.UpdateRender(mouseData);

                    renderTask.UpdateFrameIndex(EWE::Global::frameIndex);
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