//example
#include "EightWinds/VulkanHeader.h"

#include "EWEngine/EWEngine.h"
#include "EWEngine/Global.h"

#include "EightWinds/Framework.h"

#include "EightWinds/Pipeline/Layout.h"
#include "EightWinds/Pipeline/PipelineBase.h"
#include "EightWinds/Pipeline/Graphics.h"
#include "EightWinds/Shader.h"

#include "EightWinds/RenderGraph/Command/Record.h"
#include "EightWinds/RenderGraph/Command/Execute.h"
#include "EightWinds/RenderGraph/GPUTask.h"
#include "EightWinds/RenderGraph/TaskBridge.h"

#include "EightWinds/GlobalPushConstant.h"

#include "EightWinds/Image.h"
#include "EightWinds/ImageView.h"

#include "EWEngine/NodeGraph/Graph.h"

#include "EWEngine/Tools/ImguiHandler.h"

#include <cstdint>
#if EWE_DEBUG_BOOL
#include <cstdio>
#include <cassert>
#endif
#include <filesystem>
#include <chrono>
#include <array>

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


struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;

    [[nodiscard]] explicit SwapChainSupportDetails(VkPhysicalDevice device, VkSurfaceKHR surface) noexcept {
        EWE::EWE_VK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR, device, surface, &capabilities);
        uint32_t formatCount;
        EWE::EWE_VK(vkGetPhysicalDeviceSurfaceFormatsKHR, device, surface, &formatCount, nullptr);

        if (formatCount != 0) {
            formats.resize(formatCount);
            EWE::EWE_VK(vkGetPhysicalDeviceSurfaceFormatsKHR, device, surface, &formatCount, formats.data());
        }

        uint32_t presentModeCount;
        EWE::EWE_VK(vkGetPhysicalDeviceSurfacePresentModesKHR, device, surface, &presentModeCount, nullptr);

        if (presentModeCount != 0) {
            presentModes.resize(presentModeCount);
            EWE::EWE_VK(vkGetPhysicalDeviceSurfacePresentModesKHR, device, surface, &presentModeCount, presentModes.data());
        }
    }


    [[nodiscard]] bool Adequate() const { return !formats.empty() && !presentModes.empty(); }
};

int main() {

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

#if EWE_DEBUG_BOOL
    printf("current dir - %s\n", std::filesystem::current_path().string().c_str());
#endif

    //need to fix htis. its something with my windows debugger
    auto current_working_directory = std::filesystem::current_path();
    auto parentStem = current_working_directory.parent_path().parent_path().stem();
    if (parentStem == "build") {
        current_working_directory = current_working_directory.parent_path().parent_path().parent_path();
#if EWE_DEBUG_BOOL
        printf("build redacted working dir - %s\n", current_working_directory.string().c_str());
#endif
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
        cai.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        cai.Create(vmaAllocCreateInfo);
    }
    for (auto& dai : depthAttachmentImages) {
        dai.format = depthFormat;
        dai.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        dai.Create(vmaAllocCreateInfo);
    }
#if EWE_DEBUG_NAMING
    colorAttachmentImages[0].SetName("cai 0");
    colorAttachmentImages[1].SetName("cai 1");
    depthAttachmentImages[0].SetName("dai 0");
    depthAttachmentImages[1].SetName("dai 1");
#endif

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

        VkFenceCreateInfo fenceCreateInfo{};
        fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceCreateInfo.pNext = nullptr;
        fenceCreateInfo.flags = 0;
        EWE::Fence stc_fence{ logicalDevice, fenceCreateInfo };

        VkSubmitInfo stc_submit_info{};
        stc_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        stc_submit_info.pNext = nullptr;
        stc_submit_info.commandBufferCount = 1;
        stc_submit_info.pCommandBuffers = &temp_stc_cmdBuf;
        stc_submit_info.signalSemaphoreCount = 0;
        stc_submit_info.waitSemaphoreCount = 0;
        stc_submit_info.pWaitDstStageMask = nullptr;

        renderQueue->Submit(1, &stc_submit_info, stc_fence);

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
    EWE::PipelinePassConfig passConfig;
    passConfig.SetDefaults();
    passConfig.depthStencilInfo.depthTestEnable = VK_FALSE;
    passConfig.depthStencilInfo.depthWriteEnable = VK_FALSE;
    passConfig.depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
    passConfig.depthStencilInfo.stencilTestEnable = VK_FALSE;

    EWE::PipelineObjectConfig objectConfig;
    objectConfig.SetDefaults();
    objectConfig.cullMode = VK_CULL_MODE_NONE;
    objectConfig.depthClamp = false;
    objectConfig.rasterizerDiscard = false;

    std::vector<VkDynamicState> dynamicState{ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    EWE::GraphicsPipeline triangle_pipeline{ logicalDevice, 0, &triangle_layout, passConfig, objectConfig, dynamicState };


    EWE::Node::Graph nodeGraph{};

    EWE::CommandRecord renderRecord{};
    renderRecord.BeginRender();
    auto* def_pipe = renderRecord.BindPipeline();
    auto* def_vp_scissor = renderRecord.SetViewportScissor();
    //auto* def_desc = cmdRecord.BindDescriptor();
    auto* def_push = renderRecord.Push();
    auto* def_draw = renderRecord.Draw();
    nodeGraph.Record(renderRecord);
#
    renderRecord.EndRender();

    EWE::RenderGraph& renderGraph = engine.renderGraph;
    EWE::GPUTask& renderTask = renderGraph.tasks.AddElement(logicalDevice, *renderQueue, renderRecord, "render");

    def_pipe->GetRef().pipe = triangle_pipeline.vkPipe;
    def_pipe->GetRef().layout = triangle_pipeline.pipeLayout->vkLayout;
    def_pipe->GetRef().bindPoint = triangle_pipeline.pipeLayout->bindPoint;

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

    def_vp_scissor->GetRef().scissor = EWE::Global::window->screenDimensions;
    def_vp_scissor->GetRef().viewport.x = 0.f;
    def_vp_scissor->GetRef().viewport.y = static_cast<float>(EWE::Global::window->screenDimensions.height);
    def_vp_scissor->GetRef().viewport.width = static_cast<float>(EWE::Global::window->screenDimensions.width);
    def_vp_scissor->GetRef().viewport.height = -static_cast<float>(EWE::Global::window->screenDimensions.height);
    def_vp_scissor->GetRef().viewport.minDepth = 0.0f;
    def_vp_scissor->GetRef().viewport.maxDepth = 1.f;

    VmaAllocationCreateInfo vmaAllocInfo{};
    vmaAllocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
    vmaAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    vmaAllocInfo.memoryTypeBits = 0;
    vmaAllocInfo.pool = VK_NULL_HANDLE;
    vmaAllocInfo.preferredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    vmaAllocInfo.requiredFlags = 0;
    vmaAllocInfo.priority = 1.f;
    vmaAllocInfo.pUserData = nullptr;

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
    {
        def_draw->GetRef().firstInstance = 0;
        def_draw->GetRef().firstVertex = 0;
        def_draw->GetRef().instanceCount = 1;
        def_draw->GetRef().vertexCount = 3;
    }
    EWE::CommandRecord blitRecord{};
    auto* deferredBlit = blitRecord.Blit();

    //i think the rendergraph creates and defines the present task
    uint32_t swapchainImageIndex = 0;
    EWE::GPUTask& blitTask = renderGraph.tasks.AddElement(logicalDevice, *renderQueue, blitRecord, "present blit");


    VkSubmitInfo submitInfo{};
    submitInfo.commandBufferCount = 1;
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext = nullptr;
    submitInfo.pSignalSemaphores = nullptr;
    submitInfo.signalSemaphoreCount = 0;
    submitInfo.waitSemaphoreCount = 1;
    VkPipelineStageFlags waitDstStageMask[2] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    };
    submitInfo.pWaitDstStageMask = waitDstStageMask;

    //VkImageBlit imageBlit{};
    //imageBlit.srcSubresource

    VkCommandBufferBeginInfo cmdBeginInfo{};
    cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBeginInfo.pNext = nullptr;
    cmdBeginInfo.pInheritanceInfo = nullptr;
    cmdBeginInfo.flags = 0;

    deferredBlit->GetRef().filter = VK_FILTER_LINEAR;
    VkImageBlit& blitParams = deferredBlit->GetRef().blitParams;
    blitParams.srcOffsets[0].x = 0;
    blitParams.srcOffsets[0].y = 0;
    blitParams.srcOffsets[0].z = 0;

    blitParams.dstOffsets[0].x = 0;
    blitParams.dstOffsets[0].y = 0;
    blitParams.dstOffsets[0].z = 0;

    blitParams.srcOffsets[1].x = colorAttViews[0].image.extent.width;
    blitParams.srcOffsets[1].y = colorAttViews[0].image.extent.height;
    blitParams.srcOffsets[1].z = colorAttViews[0].image.extent.depth;

    blitParams.dstOffsets[1].x = engine.swapchain.swapCreateInfo.imageExtent.width;
    blitParams.dstOffsets[1].y = engine.swapchain.swapCreateInfo.imageExtent.height;
    blitParams.dstOffsets[1].z = 1;

    blitParams.srcSubresource.aspectMask = colorAttViews[0].subresource.aspectMask;
    blitParams.srcSubresource.baseArrayLayer = 0;
    blitParams.srcSubresource.layerCount = colorAttViews[0].subresource.layerCount;
    blitParams.srcSubresource.mipLevel = 0;

    blitParams.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blitParams.dstSubresource.baseArrayLayer = 0;
    blitParams.dstSubresource.layerCount = 1;
    blitParams.dstSubresource.mipLevel = 0;

    //, VkCommandPoolCreateFlags createFlag
    EWE::CommandPool renderCmdPool{ logicalDevice, *renderQueue, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT};
    std::vector<VkCommandBuffer> cmdBufVector(2, VK_NULL_HANDLE);

    VkCommandBufferAllocateInfo cmdBufAllocInfo{};
    cmdBufAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdBufAllocInfo.pNext = nullptr;
    cmdBufAllocInfo.commandBufferCount = 2;
    cmdBufAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdBufAllocInfo.commandPool = renderCmdPool.commandPool;

    EWE::EWE_VK(vkAllocateCommandBuffers, logicalDevice.device, &cmdBufAllocInfo, cmdBufVector.data());
    renderCmdPool.allocatedBuffers += 2;

    std::vector<EWE::CommandBuffer> commandBuffers{};
    commandBuffers.emplace_back(renderCmdPool, cmdBufVector[0]);
    commandBuffers.emplace_back(renderCmdPool, cmdBufVector[1]);

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
    int blitSrcPin = blitTask.AddImagePin(&colorAttachmentImages[EWE::Global::frameIndex], VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    int blitDstPin = blitTask.AddImagePin(nullptr, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    renderGraph.presentBridge.SetSubresource(
        VkImageSubresourceRange{
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    );
    EWE::ImguiHandler imguiHandler{*renderQueue, 3, VK_SAMPLE_COUNT_1_BIT};

    EWE::SubmissionTask world_render_submission{ *EWE::Global::logicalDevice, *renderQueue, true };
    EWE::SubmissionTask imgui_submission{ *EWE::Global::logicalDevice, *renderQueue, true };
    EWE::TaskSubmissionWorkload render_workload{};
    render_workload.ordered_gpuTasks.push_back(&renderTask);
    world_render_submission.full_workload = render_workload.PackIntoTask();
    imguiHandler.SetSubmissionData(imgui_submission.submitInfo);
    EWE::SubmissionTask attachment_blit_submission{ *EWE::Global::logicalDevice, *renderQueue, true };
    EWE::TaskSubmissionWorkload blit_workload{};
    blit_workload.ordered_gpuTasks.push_back(&blitTask);
    attachment_blit_submission.full_workload = blit_workload.PackIntoTask();
    renderGraph.execution_order = {
        std::vector<EWE::SubmissionTask*>{&imgui_submission, &world_render_submission},
        std::vector<EWE::SubmissionTask*>{&attachment_blit_submission}
    };

    renderGraph.GenerateSubmissionsBridges();



    try { //beginning of render loop
        uint64_t totalFrames = 0;
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
                    auto& swapPackage = engine.swapchain.GetImagePackage();
                    EWE::Image swapImage{ logicalDevice };
                    swapImage.image = swapPackage.image;
                    swapImage.layout = VK_IMAGE_LAYOUT_UNDEFINED;
                    swapImage.owningQueue = renderQueue;//i dont know how to logically set this, but this will work
                    swapImage.mipLevels = 1;
                    swapImage.arrayLayers = 1;
#if EWE_DEBUG_BOOL
                    swapImage.SetName("swap chain image");
#endif

                    deferredBlit->GetRef().srcImage = colorAttachmentImages[EWE::Global::frameIndex].image;
                    deferredBlit->GetRef().srcLayout = colorAttachmentImages[EWE::Global::frameIndex].layout;
                    deferredBlit->GetRef().dstImage = swapPackage.image;
                    deferredBlit->GetRef().dstLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

                    renderTask.SetResource(colorAttOutputPin, colorAttachmentImages[EWE::Global::frameIndex]);
                    blitTask.SetResource(blitSrcPin, colorAttachmentImages[EWE::Global::frameIndex]);
                    blitTask.SetResource(blitDstPin, swapImage);

                    nodeGraph.UpdateRender(mouseData);

                    renderTask.UpdateFrameIndex(EWE::Global::frameIndex);

                    renderGraph.presentBridge.UpdateSrcData(&blitTask.queue, blitTask.explicitImageState[blitDstPin]);

                    EWE::CommandBuffer& currentCmdBuf = commandBuffers[EWE::Global::frameIndex];
                    currentCmdBuf.Reset();
                    currentCmdBuf.Begin(cmdBeginInfo);
#ifdef EWE_IMGUI
                    printf("ewe imgui\n");
                    imguiHandler.BeginRender();
                    nodeGraph.Imgui();
                    imguiHandler.EndRender();
#endif
                    renderGraph.Execute(currentCmdBuf, EWE::Global::frameIndex);
                    renderGraph.PresentBridge(currentCmdBuf);
                    currentCmdBuf.End();
                    submitInfo.pCommandBuffers = &currentCmdBuf.cmdBuf;
                    //EWE::EWE_VK(vkQueueSubmit, *renderQueue, 1, &submitInfo, VK_NULL_HANDLE);
                    submitInfo.signalSemaphoreCount = 1;
                    std::vector<VkSemaphore> waitSemaphores{
                        swapPackage.acquire_semaphore.vkSemaphore,
#ifdef EWE_IMGUI
                        imguiHandler.semaphores[EWE::Global::frameIndex].vkSemaphore
#endif
                    };
                    submitInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
                    submitInfo.pWaitSemaphores = waitSemaphores.data();
                    submitInfo.pSignalSemaphores = &swapPackage.present_semaphore.vkSemaphore;
                    renderQueue->Submit(1, &submitInfo, engine.swapchain.inFlightFences[EWE::Global::frameIndex].vkFence);
#if EWE_DEBUG_BOOL
                    currentCmdBuf.state = EWE::CommandBuffer::State::Pending;
#endif
                    renderGraph.Present();

                    EWE::Global::frameIndex = (EWE::Global::frameIndex + 1) % EWE::max_frames_in_flight;
                    totalFrames++;
                }
                else {
                    //need to recreate swapchain, thats handled internally
                    //any other Swapchain::Recreate 'callbacks' will be put here
                    blitParams.dstOffsets[1].x = engine.swapchain.swapCreateInfo.imageExtent.width;
                    blitParams.dstOffsets[1].y = engine.swapchain.swapCreateInfo.imageExtent.height;
                    blitParams.dstOffsets[1].z = 1;
                }
                elapsedTime = std::chrono::nanoseconds(0);
            }
        }
    }
    catch (EWE::EWEException& except) {
        framework.HandleVulkanException(except);
    }


#if EWE_DEBUG_BOOL
    printf("returning successfully\n");
#endif

    delete triangle_vert;
    delete triangle_frag;


    std::this_thread::sleep_for(std::chrono::seconds(5)); 
    return 0;
}