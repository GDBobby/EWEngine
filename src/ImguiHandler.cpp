#include "EWEngine/Tools/ImguiHandler.h"

#include "EightWinds/CommandBuffer.h"

#include "EWEngine/Global.h"

#include "EightWinds/Backend/STC_Helper.h"


#include "GLFW/glfw3.h"
#include "imgui.h"

#include "EightWinds/Window.h"

#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"
#include "imgui_internal.h"

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

		IMGUI_CHECKVERSION();
		
		auto& main_vp = viewports.emplace_back();
		main_vp.context = InitializeContext();
		ImGui::CreateDragDropContext();

		TakeCallbackControl(Global::window->window);
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

	ImGuiContext* ImguiHandler::InitializeContext(){


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


		auto* prev_context = ImGui::GetCurrentContext();
		ImGuiContext* ret = ImGui::CreateContext();
		//if another context is currently set, CreateContext takes the liberty of returning the context to the previous
		ImGui::SetCurrentContext(ret); 

		ret->IO.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		ImGui::StyleColorsDark();
	
		ImGui_ImplGlfw_InitForVulkan(Global::window->window, false);
		ImGui_ImplVulkan_Init(&vulkan_init_info);

		if(prev_context != nullptr){
			ImGui::SetCurrentContext(prev_context);
		}
		return ret;
	}

	void ImguiHandler::Render(CommandBuffer& cmdBuf) {

		if(viewports.size() > 0){
			isRendering = true;

			//auto& main_io = ImGui::GetIO(viewports[0].context);



			vkCmdBeginRendering(cmdBuf, &renderInfo.render_data.vk_info[Global::frameIndex]);

			ImGui::NewFrameDD();
			//im doing a index based approached because i want to allow the creation/removal of viewports from within the exec_func
			for(std::size_t i = 0; i < viewports.size(); i++) {
				auto& vp = viewports[i];
				if(vp.exec_func != nullptr){

#if EWE_DEBUG_NAMING
					std::string label_name = std::string("imgui[") + std::to_string(i) + "]";
					VkDebugUtilsLabelEXT labelUtil{
						.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
						.pNext = nullptr,
						.pLabelName = label_name.c_str(),
						.color = {0.f, 0.f, 0.f, 1.f}
					};
					Global::logicalDevice->BeginLabel(cmdBuf, &labelUtil);
#endif

					ImGui::SetCurrentContext(vp.context);
					ImGui_ImplVulkan_NewFrame();
					ImGui_ImplGlfw_NewFrame();

					vp.context->Viewports[0]->Pos.x = vp.current_viewport.offset.x;
					vp.context->Viewports[0]->Pos.y = vp.current_viewport.offset.y;
					//vp.context->Viewports[0]->Size.x = vp.current_viewport.extent.width;
					//vp.context->Viewports[0]->Size.y = vp.current_viewport.extent.height;

					//displaysize is already capped to window size, so we apply that same cap to the local extent
					
					vp.context->IO.DisplaySize.x = lab::Min(vp.context->IO.DisplaySize.x, static_cast<float>(vp.current_viewport.extent.width));
					vp.context->IO.DisplaySize.y = lab::Min(vp.context->IO.DisplaySize.y, static_cast<float>(vp.current_viewport.extent.height));
					vp.current_viewport.extent.width = vp.context->IO.DisplaySize.x;
					vp.current_viewport.extent.height = vp.context->IO.DisplaySize.y;

					ImGui::NewFrame();
					
					bool always_open = true;
					ImGuiWindowFlags main_flags = ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;
					ImGui::SetNextWindowBgAlpha(0.f);
					ImGuiViewport* viewport = ImGui::GetMainViewport();
					if(viewport->Pos.x < 0.f){
						viewport->Pos.x = 0.f;
					}
					if(viewport->Pos.y < 0.f){
						viewport->Pos.y = 0.f;
					}
					if((viewport->Size.x + viewport->Pos.x) > Global::window->screenDimensions.width){
						viewport->Size.x = Global::window->screenDimensions.width - viewport->Pos.x;
					}
					if((viewport->Size.y + viewport->Pos.y) > Global::window->screenDimensions.height){
						viewport->Size.y = Global::window->screenDimensions.height - viewport->Pos.y;
					}

					//ImGui::FindBottomMostVisibleWindowWithinBeginStack(ImGuiWindow *window)

					ImGui::SetNextWindowPos(viewport->Pos);
					ImGui::SetNextWindowSize(viewport->Size);
					if(ImGui::Begin("##backgroundwindow", &always_open, main_flags)){
						vp.current_viewport.offset.x = ImGui::GetWindowPos().x;
						vp.current_viewport.offset.y = ImGui::GetWindowPos().y;

						vp.current_viewport.extent.width = ImGui::GetWindowSize().x;
						vp.current_viewport.extent.height = ImGui::GetWindowSize().y;
						ImGui::PushID(static_cast<int>(reinterpret_cast<std::size_t>(vp.context)));
						vp.exec_func(vp);
						ImGui::PopID();
					}
					ImGui::End();
					ImGui::Render();
					ImGui::EndFrame();
					//auto draw_data = ImGui::GetDrawData();
					ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmdBuf);


#if EWE_DEBUG_NAMING
			Global::logicalDevice->EndLabel(cmdBuf);
#endif
				}
			}
			ImGui::EndFrameDD();

			isRendering = false;
			vkCmdEndRendering(cmdBuf);
		}
	}

	void ImguiHandler::Begin(){
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
	}
	void ImguiHandler::End(){
		ImGui::EndFrame();
	}

	ImguiHandler* global_for_callbacks = nullptr;
	void ImguiHandler::TakeCallbackControl(GLFWwindow* window){
		global_for_callbacks = this;
		glfwSetWindowFocusCallback(window, ImguiHandler::WindowFocusCallback);
		glfwSetCursorEnterCallback(window, ImguiHandler::CursorEnterCallback);
		glfwSetCursorPosCallback(window, ImguiHandler::CursorPosCallback);
		glfwSetMouseButtonCallback(window, ImguiHandler::MouseButtonCallback);
		glfwSetScrollCallback(window, ImguiHandler::ScrollCallback);
		glfwSetKeyCallback(window, ImguiHandler::KeyCallback);
		glfwSetCharCallback(window, ImguiHandler::CharCallback);
	}

	void ImguiHandler::WindowFocusCallback(GLFWwindow* window, int focused){
		for(auto& vp : global_for_callbacks->viewports){
			ImGui_ImplGlfw_WindowFocusCallback_Context(vp.context, window, focused);
		}
	}
	void ImguiHandler::CursorEnterCallback(GLFWwindow* window, int entered){
		for(auto& vp : global_for_callbacks->viewports){
			ImGui_ImplGlfw_CursorEnterCallback_Context(vp.context, window, entered);
		}
	}
	void ImguiHandler::CursorPosCallback(GLFWwindow* window, double x, double y){
		for(auto& vp : global_for_callbacks->viewports){
			ImGui_ImplGlfw_CursorPosCallback_Context(vp.context, window, x, y);
		}
	}
	void ImguiHandler::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods){
		for(auto& vp : global_for_callbacks->viewports){
			ImGui_ImplGlfw_MouseButtonCallback_Context(vp.context, window, button, action, mods);
		}
	}
	void ImguiHandler::ScrollCallback(GLFWwindow* window, double xoffset, double yoffset){
		for(auto& vp : global_for_callbacks->viewports){
			ImGui_ImplGlfw_ScrollCallback_Context(vp.context, window, xoffset, yoffset);
		}
	}
	void ImguiHandler::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods){
		for(auto& vp : global_for_callbacks->viewports){
			ImGui_ImplGlfw_KeyCallback_Context(vp.context, window, key, scancode, action, mods);
		}
	}
	void ImguiHandler::CharCallback(GLFWwindow* window, unsigned int c){
		for(auto& vp : global_for_callbacks->viewports){
			ImGui_ImplGlfw_CharCallback_Context(vp.context, window, c);
		}
	}

} //namespace EWE