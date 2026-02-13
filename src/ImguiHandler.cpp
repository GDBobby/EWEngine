#include "EWEngine/Tools/ImguiHandler.h"

#include "EightWinds/ImageView.h"
#include "EightWinds/CommandBuffer.h"
#include "EightWinds/CommandPool.h"
#include "EightWinds/Backend/Fence.h"

#include "EWEngine/Global.h"

#include "EightWinds/Backend/STC_Helper.h"

namespace EWE{

	std::array<VkFormat, 1> colorFormats{ VK_FORMAT_R8G8B8A8_UNORM };

	PerFlight<Image> CreateColorAttachmentImages(Queue& queue, VkSampleCountFlagBits sampleCount) {
		PerFlight<Image> ret{ *Global::logicalDevice };

		VmaAllocationCreateInfo vmaAllocCreateInfo{
			.flags = static_cast<VmaAllocationCreateFlags>(VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT) | static_cast<VmaAllocationCreateFlags>(VMA_ALLOCATOR_CREATE_EXTERNALLY_SYNCHRONIZED_BIT),
			.usage = VMA_MEMORY_USAGE_AUTO
		};
		for (auto& cai : ret) {
			cai.arrayLayers = 1;
			cai.extent = { EWE::Global::window->screenDimensions.width, EWE::Global::window->screenDimensions.height, 1 };
			cai.mipLevels = 1;
			cai.owningQueue = &queue;
			cai.samples = sampleCount;
			cai.tiling = VK_IMAGE_TILING_OPTIMAL;
			cai.type = VK_IMAGE_TYPE_2D;
			cai.format = colorFormats[0];
			cai.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

			cai.Create(vmaAllocCreateInfo);
		}
#if EWE_DEBUG_NAMING
		ret[0].SetName("cai imgui 0");
		ret[1].SetName("cai imgui 1");
#endif
		return ret;
	}

	ImguiHandler::ImguiHandler(
		Queue& queue,
		uint32_t imageCount, VkSampleCountFlagBits sampleCount
	)
		: queue{ queue },
		cmdPool{ *Global::logicalDevice, queue, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT },
		cmdBuffers{cmdPool.AllocateCommandsPerFlight(VK_COMMAND_BUFFER_LEVEL_PRIMARY)},
		renderInfo{
			"imgui render info",
			*Global::logicalDevice, queue,
			AttachmentSetInfo{
				.width = Global::window->screenDimensions.width, 
				.height = Global::window->screenDimensions.height,
				.renderingFlags = 0,
				.colors = {
					AttachmentInfo{
						.format = VK_FORMAT_R8G8B8A8_UNORM,
						.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
						.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
						.clearValue = {0.f, 0.f, 0.f, 0.f}

					}
				},
				.depth = AttachmentInfo{
					.format = VK_FORMAT_D16_UNORM,
					.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
					.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
					.clearValue = {0.f, 0.f, 0.f, 0.f}
				}
			}
		},
		semaphores{ *Global::logicalDevice}
    {
#if EWE_DEBUG_NAMING
		for (uint8_t i = 0; i < EWE::max_frames_in_flight; i++) {
			std::string debugName = std::string("imgui [") + std::to_string(i) + ']';
			semaphores[i].SetName(debugName);
			cmdBuffers[i].SetDebugName(debugName);
		}
#endif

		InitializeImages();


		IMGUI_CHECKVERSION();
		ImGui::CreateContext();

		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		ImGui::StyleColorsDark();
        
		ImGui_ImplGlfw_InitForVulkan(Global::window->window, true);
		ImGui_ImplVulkan_InitInfo init_info{
			.ApiVersion = Global::instance->api_version,
			.Instance = Global::logicalDevice->instance,
			.PhysicalDevice = Global::logicalDevice->physicalDevice.device,
			.Device = Global::logicalDevice->device,
			.QueueFamily = queue.family.index,
			.Queue = queue,
			.DescriptorPoolSize = 1024,
			.MinImageCount = imageCount,
			.ImageCount = imageCount,
			.PipelineCache = nullptr,
			.PipelineInfoMain = ImGui_ImplVulkan_PipelineInfo{
				.RenderPass = VK_NULL_HANDLE,
				.MSAASamples = sampleCount,
				.PipelineRenderingCreateInfo = VkPipelineRenderingCreateInfo{
					.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
					.pNext = nullptr,
					.viewMask = 0,
					.colorAttachmentCount = 1,
					.pColorAttachmentFormats = colorFormats.data(),
					.depthAttachmentFormat = VK_FORMAT_D16_UNORM,
					.stencilAttachmentFormat = VK_FORMAT_UNDEFINED
				}
			},
			.UseDynamicRendering = true,
			.Allocator = nullptr,
			.CheckVkResultFn = EWE_VK_RESULT,
		};

		ImGui_ImplVulkan_Init(&init_info);


    }

	ImguiHandler::~ImguiHandler() {
		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}

	void ImguiHandler::InitializeImages() {

	}

	void ImguiHandler::BeginCommandBuffer() {
		VkCommandBufferBeginInfo beginInfo{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.pNext = nullptr,
			.flags = 0,
			.pInheritanceInfo = nullptr
		};

		auto& currentCmdBuf = cmdBuffers[Global::frameIndex];
		currentCmdBuf.Reset();
		currentCmdBuf.Begin(beginInfo);
	}
	void ImguiHandler::BeginRender() {
		auto& currentCmdBuf = cmdBuffers[Global::frameIndex];

#if EWE_DEBUG_NAMING
		VkDebugUtilsLabelEXT labelUtil{
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
			.pNext = nullptr,
			.pLabelName = "imgui",
			.color = {0.f, 0.f, 0.f, 1.f}
		};
		Global::logicalDevice->BeginLabel(cmdBuffers[Global::frameIndex], &labelUtil);
#endif

		isRendering = true;
		vkCmdBeginRendering(currentCmdBuf, &renderInfo.render_data.vk_info[Global::frameIndex]);
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
#if EWE_DEBUG_NAMING
		Global::logicalDevice->EndLabel(currentCmdBuf);
#endif
		currentCmdBuf.End();
		currentCmdBuf.state = CommandBuffer::State::Pending;
	}

	void ImguiHandler::SetSubmissionData(PerFlight<Backend::SubmitInfo>& subInfo) {
		for (uint8_t i = 0; i < max_frames_in_flight; i++) {
			subInfo[i].AddCommandBuffer(cmdBuffers[i]);

			VkSemaphoreSubmitInfo semSubmitInfo{
				.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
				.pNext = nullptr,
				.semaphore = semaphores[i].vkSemaphore,
				.value = 0,
				.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
				.deviceIndex = 0
			};
			subInfo[i].signalSemaphores.push_back(semSubmitInfo);
		}
	}
}