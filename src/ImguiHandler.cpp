#include "EWEngine/Tools/ImguiHandler.h"

#include "EightWinds/CommandBuffer.h"

#include "EWEngine/Global.h"

#include "EightWinds/Backend/STC_Helper.h"


#include "imgui.h"

#include "EightWinds/Window.h"

#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"

namespace EWE{

	std::array<VkFormat, 1> colorFormats{ VK_FORMAT_R8G8B8A8_UNORM };

	PerFlight<Image> CreateColorAttachmentImages(Queue& queue, VkSampleCountFlagBits sampleCount) {
		PerFlight<Image> ret{ *Global::logicalDevice };

		VmaAllocationCreateInfo vmaAllocCreateInfo{
			.flags = static_cast<VmaAllocationCreateFlags>(VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT) | static_cast<VmaAllocationCreateFlags>(VMA_ALLOCATOR_CREATE_EXTERNALLY_SYNCHRONIZED_BIT),
			.usage = VMA_MEMORY_USAGE_AUTO
		};
		for (auto& cai : ret) {
			cai.data.arrayLayers = 1;
			cai.data.extent = { EWE::Global::window->screenDimensions.width, EWE::Global::window->screenDimensions.height, 1 };
			cai.data.mipLevels = 1;
			cai.owningQueue = &queue;
			cai.data.samples = sampleCount;
			cai.data.tiling = VK_IMAGE_TILING_OPTIMAL;
			cai.data.type = VK_IMAGE_TYPE_2D;
			cai.data.format = colorFormats[0];
			cai.data.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

			cai.Create(vmaAllocCreateInfo);
		}
#if EWE_DEBUG_NAMING
		ret[0].SetName("cai imgui 0");
		ret[1].SetName("cai imgui 1");
#endif
		return ret;
	}

	//int current_imgui_handler_count = 0;


	ImguiHandler::ImguiHandler(
		Queue& _queue,
		uint32_t imageCount, VkSampleCountFlagBits sampleCount
	)
		: queue{ _queue },
		//cmdPool{ *Global::logicalDevice, queue, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT },
		//cmdBuffers{cmdPool.AllocateCommandsPerFlight(VK_COMMAND_BUFFER_LEVEL_PRIMARY)},
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
		image_count{imageCount},
		sample_count{sampleCount}
    {
#if EWE_DEBUG_NAMING
		for (uint8_t i = 0; i < EWE::max_frames_in_flight; i++) {
			//std::string debugName = std::string("imgui [") + std::to_string(i) + ']';
			//semaphores[i].SetName(debugName);
			//cmdBuffers[i].SetDebugName(debugName);
		}
#endif
		InitializeImages();

		IMGUI_CHECKVERSION();
		
		auto& main_vp = viewports.emplace_back();
		main_vp.context = InitializeContext();
    }

	ImguiHandler::~ImguiHandler() {
		//if(current_imgui_handler_count == 1){
			ImGui_ImplVulkan_Shutdown();
			ImGui_ImplGlfw_Shutdown();
		//}
		//current_imgui_handler_count--;
		for(auto& vp : viewports){
			ImGui::DestroyContext(vp.context);
		}
	}

	void ImguiHandler::InitializeImages() {

	}

	ImGuiContext* ImguiHandler::InitializeContext(){

		auto* prev_context = ImGui::GetCurrentContext();

		ImGuiContext* ret = ImGui::CreateContext();
		//if another context is currently set, CreateContext takes the liberty of returning the context
		ImGui::SetCurrentContext(ret); 

        ImGui_ImplVulkan_InitInfo vulkan_init_info{
			.ApiVersion = Global::instance->api_version,
			.Instance = Global::logicalDevice->instance,
			.PhysicalDevice = Global::logicalDevice->physicalDevice.device,
			.Device = Global::logicalDevice->device,
			.QueueFamily = queue.family.index,
			.Queue = queue,
			.DescriptorPoolSize = 1024,
			.MinImageCount = image_count,
			.ImageCount = image_count,
			.PipelineCache = nullptr,
			.PipelineInfoMain = ImGui_ImplVulkan_PipelineInfo{
				.RenderPass = VK_NULL_HANDLE,
				.MSAASamples = sample_count,
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


		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		ImGui::StyleColorsDark();
	
		ImGui_ImplGlfw_InitForVulkan(Global::window->window, prev_context == nullptr);

		ImGui_ImplVulkan_Init(&vulkan_init_info);

		if(prev_context != nullptr){
			ImGui::SetCurrentContext(prev_context);
		}
		return ret;
	}

	/*
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
		*/
	void ImguiHandler::Render(CommandBuffer& cmdBuf) {
		//auto& currentCmdBuf = cmdBuffers[Global::frameIndex];


		if(viewports.size() > 0){
			isRendering = true;

#if EWE_DEBUG_NAMING
			VkDebugUtilsLabelEXT labelUtil{
				.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
				.pNext = nullptr,
				.pLabelName = "imgui",
				.color = {0.f, 0.f, 0.f, 1.f}
			};
			Global::logicalDevice->BeginLabel(cmdBuf, &labelUtil);
#endif

			vkCmdBeginRendering(cmdBuf, &renderInfo.render_data.vk_info[Global::frameIndex]);

			//im doing a index based approached because i want to allow the creation/removal of viewports from within the exec_func
			for(std::size_t i = 0; i < viewports.size(); i++) {
				auto& vp = viewports[i];
				if(vp.exec_func != nullptr){
					ImGui::SetCurrentContext(vp.context);
					ImGui_ImplVulkan_NewFrame();
					ImGui_ImplGlfw_NewFrame();

					auto& io = ImGui::GetIO();

					//displaysize is already capped to window size, so we apply that same cap to the local extent
					io.DisplayPos.x = vp.current_viewport.offset.x;
					io.DisplayPos.y = vp.current_viewport.offset.y;

					io.DisplaySize.x = lab::Min(io.DisplaySize.x, static_cast<float>(vp.current_viewport.extent.width));
					vp.current_viewport.extent.width = io.DisplaySize.x;
					io.DisplaySize.y = lab::Min(io.DisplaySize.y, static_cast<float>(vp.current_viewport.extent.height));
					vp.current_viewport.extent.height = io.DisplaySize.y;


					ImGui::NewFrame();
					
					vp.exec_func(vp);

					ImGui::Render();
					//auto draw_data = ImGui::GetDrawData();
					ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmdBuf);
				}
			}


			isRendering = false;
			vkCmdEndRendering(cmdBuf);
#if EWE_DEBUG_NAMING
			Global::logicalDevice->EndLabel(cmdBuf);
#endif
		}



	}
}