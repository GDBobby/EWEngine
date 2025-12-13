#include "EWEngine/Tools/ImguiHandler.h"

#include "EightWinds/ImageView.h"
#include "EightWinds/Command/CommandBuffer.h"
#include "EightWinds/Command/CommandPool.h"
#include "EightWinds/Backend/Fence.h"

#include "EWEngine/Global.h"


namespace EWE{

	std::array<VkFormat, 1> colorFormats{ VK_FORMAT_R8G8B8A8_UNORM };

	PerFlight<Image> CreateColorAttachmentImages(Queue& queue, VkSampleCountFlagBits sampleCount) {
		PerFlight<Image> ret{ *Global::logicalDevice };

		VmaAllocationCreateInfo vmaAllocCreateInfo{};
		vmaAllocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
		vmaAllocCreateInfo.flags = static_cast<VmaAllocationCreateFlags>(VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT) | static_cast<VmaAllocationCreateFlags>(VMA_ALLOCATOR_CREATE_EXTERNALLY_SYNCHRONIZED_BIT);

		for (auto& cai : ret) {
			cai.arrayLayers = 1;
			cai.extent = { EWE::Global::window->screenDimensions.width, EWE::Global::window->screenDimensions.height, 1 };
			cai.mipLevels = 1;
			cai.owningQueue = &queue;
			cai.samples = sampleCount;
			cai.tiling = VK_IMAGE_TILING_OPTIMAL;
			cai.type = VK_IMAGE_TYPE_2D;
			cai.format = colorFormats[0];
			cai.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

			cai.Create(vmaAllocCreateInfo);
		}
#if EWE_DEBUG_NAMING
		ret[0].SetName("cai imgui 0");
		ret[1].SetName("cai imgui 1");
#endif
		return ret;
	}

	PerFlight<CommandBuffer> CreateCommandBuffers(CommandPool& cmdPool) {
		VkCommandBufferAllocateInfo cmdBufAllocInfo{};
		cmdBufAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmdBufAllocInfo.pNext = nullptr;
		cmdBufAllocInfo.commandBufferCount = max_frames_in_flight;
		cmdBufAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		cmdBufAllocInfo.commandPool = cmdPool.commandPool;

		VkCommandBuffer cmdBufTemp[max_frames_in_flight];
		EWE::EWE_VK(vkAllocateCommandBuffers, cmdPool.logicalDevice.device, &cmdBufAllocInfo, cmdBufTemp);
		cmdPool.allocatedBuffers += max_frames_in_flight;

		PerFlight<CommandBuffer> cmdBufs{ cmdPool, VK_NULL_HANDLE };
		for (uint8_t i = 0; i < max_frames_in_flight; i++) {
			cmdBufs[i].cmdBuf = cmdBufTemp[i];
		}
		return cmdBufs;
	}

	ImguiHandler::ImguiHandler(
		Queue& queue,
		uint32_t imageCount, VkSampleCountFlagBits sampleCount
	)
		: queue{ queue },
		colorAttachmentImages{CreateColorAttachmentImages(queue, sampleCount)},
		colorAttachmentViews{ colorAttachmentImages },
		cmdPool{ *Global::logicalDevice, queue, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT },
		cmdBuffers{CreateCommandBuffers(cmdPool)},
		semaphores{ *Global::logicalDevice, false}
    {
		renderTracker.compact.color_attachments.resize(1);
		renderTracker.compact.color_attachments[0].imageView[0] = &colorAttachmentViews[0];
		renderTracker.compact.color_attachments[0].imageView[1] = &colorAttachmentViews[1];
		renderTracker.compact.color_attachments[0].clearValue.color.float32[0] = 0.f;
		renderTracker.compact.color_attachments[0].clearValue.color.float32[1] = 0.f;
		renderTracker.compact.color_attachments[0].clearValue.color.float32[2] = 0.f;
		renderTracker.compact.color_attachments[0].clearValue.color.float32[3] = 0.f;
		renderTracker.compact.color_attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		renderTracker.compact.color_attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

		renderTracker.compact.depth_attachment.imageView[0] = nullptr;
		renderTracker.compact.depth_attachment.imageView[1] = nullptr;

		renderTracker.compact.flags = 0;

		InitializeImages();


		IMGUI_CHECKVERSION();
		ImGui::CreateContext();

		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		ImGui::StyleColorsDark();
        
		ImGui_ImplGlfw_InitForVulkan(Global::window->window, true);
		ImGui_ImplVulkan_InitInfo init_info{};
		init_info.Instance = Global::logicalDevice->instance;
		init_info.PhysicalDevice = Global::logicalDevice->physicalDevice.device;
		init_info.Device = Global::logicalDevice->device;
		init_info.QueueFamily = queue.family.index;
		init_info.Queue = queue;
		init_info.PipelineCache = nullptr;
		init_info.Allocator = nullptr;
		init_info.MinImageCount = imageCount;
		init_info.ImageCount = imageCount;
		init_info.CheckVkResultFn = EWE_VK_RESULT;
		init_info.DescriptorPoolSize = 1024;
		init_info.UseDynamicRendering = true;
		init_info.PipelineInfoMain.RenderPass = VK_NULL_HANDLE;
		init_info.PipelineInfoMain.MSAASamples = sampleCount;
		init_info.PipelineInfoMain.PipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
		init_info.PipelineInfoMain.PipelineRenderingCreateInfo.pNext = nullptr;
		init_info.PipelineInfoMain.PipelineRenderingCreateInfo.viewMask = 0;
		init_info.PipelineInfoMain.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
		init_info.PipelineInfoMain.PipelineRenderingCreateInfo.pColorAttachmentFormats = colorFormats.data();
		init_info.PipelineInfoMain.PipelineRenderingCreateInfo.depthAttachmentFormat = VK_FORMAT_UNDEFINED;
		init_info.PipelineInfoMain.PipelineRenderingCreateInfo.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;

		ImGui_ImplVulkan_Init(&init_info);


    }

	ImguiHandler::~ImguiHandler() {
		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}

	void ImguiHandler::InitializeImages() {
		//i need the automatic transfer manager again, it'll help simplify this
		EWE::CommandPool stc_cmdPool{ *Global::logicalDevice, queue, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT };

		VkCommandBufferAllocateInfo cmdBufAllocInfo{};
		cmdBufAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmdBufAllocInfo.pNext = nullptr;
		cmdBufAllocInfo.commandBufferCount = 1;
		cmdBufAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		cmdBufAllocInfo.commandPool = stc_cmdPool.commandPool;

		VkCommandBuffer temp_stc_cmdBuf;
		EWE::EWE_VK(vkAllocateCommandBuffers, Global::logicalDevice->device, &cmdBufAllocInfo, &temp_stc_cmdBuf);
		stc_cmdPool.allocatedBuffers++;

		EWE::CommandBuffer transition_stc(stc_cmdPool, temp_stc_cmdBuf);
		VkCommandBufferBeginInfo beginSTCInfo{};
		beginSTCInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginSTCInfo.pNext = nullptr;
		beginSTCInfo.pInheritanceInfo = nullptr;
		beginSTCInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		transition_stc.Begin(beginSTCInfo);


		std::vector<VkImageMemoryBarrier2> transition_barriers(2);

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
		EWE::Fence stc_fence{ *Global::logicalDevice, fenceCreateInfo };

		VkSubmitInfo stc_submit_info{};
		stc_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		stc_submit_info.pNext = nullptr;
		stc_submit_info.commandBufferCount = 1;
		stc_submit_info.pCommandBuffers = &temp_stc_cmdBuf;
		stc_submit_info.signalSemaphoreCount = 0;
		stc_submit_info.waitSemaphoreCount = 0;
		stc_submit_info.pWaitDstStageMask = nullptr;

		queue.Submit(1, &stc_submit_info, stc_fence);

		EWE_VK(vkWaitForFences, Global::logicalDevice->device, 1, &stc_fence.vkFence, VK_TRUE, 5 * static_cast<uint64_t>(1.0e9));

		for (auto& cai : colorAttachmentImages) {
			cai.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		}
		renderTracker.compact.Expand(&renderTracker.vk_data);
	}

	void ImguiHandler::BeginRender() {
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.pNext = nullptr;
		beginInfo.pInheritanceInfo = nullptr;
		beginInfo.flags = 0;

		auto& currentCmdBuf = cmdBuffers[Global::frameIndex];
		currentCmdBuf.Reset();
		currentCmdBuf.Begin(beginInfo);


		renderTracker.compact.Update(&renderTracker.vk_data, Global::frameIndex);

		isRendering = true;
		vkCmdBeginRendering(currentCmdBuf, &renderTracker.vk_data.renderingInfo);
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

	}

	void ImguiHandler::EndRender() {
		auto& currentCmdBuf = cmdBuffers[Global::frameIndex];
		ImGui::Render();
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), currentCmdBuf);
		isRendering = false;
		vkCmdEndRendering(currentCmdBuf);
		currentCmdBuf.End();
		currentCmdBuf.state = CommandBuffer::State::Pending;
	}

	void ImguiHandler::SetSubmissionData(PerFlight<Backend::SubmitInfo>& subInfo) {
		uint8_t cmdBufIndex = 0;
		for (auto& sub : subInfo) {
			sub.AddCommandBuffer(&cmdBuffers[cmdBufIndex]);

			VkSemaphoreSubmitInfo semSubmitInfo{
				.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
				.pNext = nullptr,
				.semaphore = semaphores[cmdBufIndex].vkSemaphore,
				.value = 0,
				.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
				.deviceIndex = 0
			};
			sub.signalSemaphores.push_back(semSubmitInfo);
			cmdBufIndex++;
		}
	}
}