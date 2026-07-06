#include "EWEngine/EWEngine.h"

#include "EWEngine/EngineSettings.h"
#include "EightWinds/Reflect/Enum.h"

#include "EightWinds/Backend/DeviceSpecialization/Extensions.h"
#include "EightWinds/Backend/DeviceSpecialization/DeviceSpecialization.h"
#include "EightWinds/Backend/DeviceSpecialization/FeatureProperty.h"

#include "EWEngine/Debug/RenderGraph.h"

#include "GLFW/glfw3.h"

#ifdef USING_NVIDIA_AFTERMATH
#include "EightWinds/Backend/nvidia/Aftermath.h"
#endif

#ifdef EWE_IMGUI
#include "imgui.h"
#endif

#include "marl/event.h"
#include "marl/conditionvariable.h"

#include <filesystem>



namespace EWE{

    std::thread::id mainThreadID;
    void SetMainThread(){
        mainThreadID = std::this_thread::get_id();
    }

    bool CheckMainThread() {
        return std::this_thread::get_id() == mainThreadID;
    }


    constexpr ConstEvalStr swapchainExt{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    //https://docs.vulkan.org/refpages/latest/refpages/source/VK_EXT_extended_dynamic_state3.html
    constexpr ConstEvalStr dynState3Ext{ VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME };
    constexpr ConstEvalStr meshShaderExt{ VK_EXT_MESH_SHADER_EXTENSION_NAME };
    constexpr ConstEvalStr deviceFaultExt{ VK_EXT_DEVICE_FAULT_EXTENSION_NAME };
    //this requires the instance extension VK_EXT_debug_utils. i don't know how to make that association cleanly
    constexpr ConstEvalStr dabReportExt{ VK_EXT_DEVICE_ADDRESS_BINDING_REPORT_EXTENSION_NAME };

    using Example_ExtensionManager = ExtensionManager<application_wide_vk_version,
        ExtensionEntry<swapchainExt, true, 0>,
        ExtensionEntry<dynState3Ext, true, 0>,
        ExtensionEntry<meshShaderExt, false, 100000>,
        ExtensionEntry<deviceFaultExt, false, 10000>
#if EWE_DEBUG_BOOL
        , ExtensionEntry<dabReportExt, false, 0>
#endif
    >;


    using Example_FeatureManager = EWE::Backend::FeatureManager<rounded_down_vulkan_version,
        VkPhysicalDeviceExtendedDynamicState3FeaturesEXT,
        VkPhysicalDeviceMeshShaderFeaturesEXT,
        VkPhysicalDeviceFaultFeaturesEXT

#if EWE_DEBUG_BOOL
        ,
        VkPhysicalDeviceAddressBindingReportFeaturesEXT
#endif
    >;

    using Example_PropertyManager = EWE::Backend::PropertyManager<rounded_down_vulkan_version,
        VkPhysicalDeviceMeshShaderPropertiesEXT
    >;

    using DeviceSpec = EWE::DeviceSpecializer<
        rounded_down_vulkan_version,
        Example_ExtensionManager,
        Example_FeatureManager,
        Example_PropertyManager
    >;

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

    std::vector<const char*> GetGLFWExtensions(){

        std::vector<const char*> requiredExtensions{
    #if EWE_DEBUG_BOOL
            VK_EXT_DEBUG_UTILS_EXTENSION_NAME,

    #endif
            VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME
        };

#if _WIN32
        if (!glfwInit()) {
            Log::Debug("failed to init glfw on windows\n");
        }
#else
        glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_WAYLAND);
        if (!glfwInit()) {
#if EWE_DEBUG_BOOL
            Log::Debug("failed to init glfw wayland\n");
#endif
            glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
            if(!glfwInit()){
                throw std::runtime_error("failed to init glfw x11 or glfw WAYLAND on linux");
            }
        }
#endif
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        EWE_ASSERT(glfwExtensionCount > 0, "not supporting headless");
        EWE_ASSERT(glfwExtensions != nullptr);

        for (uint32_t i = 0; i < glfwExtensionCount; ++i) {
            requiredExtensions.push_back(glfwExtensions[i]);
        }

        return requiredExtensions;
    }

    LogicalDevice CreateLogicalDevice(Instance& instance, Window& window){
        DeviceSpec specDev{};

        auto& dynState3 = specDev.GetFeature<VkPhysicalDeviceExtendedDynamicState3FeaturesEXT>();
        dynState3.extendedDynamicState3ColorBlendEnable = VK_TRUE;
        dynState3.extendedDynamicState3ColorBlendEquation = VK_TRUE;
        dynState3.extendedDynamicState3ColorWriteMask = VK_TRUE;

        auto& meshShaderFeatures = specDev.GetFeature<VkPhysicalDeviceMeshShaderFeaturesEXT>();
        meshShaderFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT;
        meshShaderFeatures.meshShader = VK_TRUE;
        meshShaderFeatures.taskShader = VK_TRUE;

        auto& features2 = specDev.GetFeature<VkPhysicalDeviceFeatures2>();
        features2.features.samplerAnisotropy = VK_TRUE;
        features2.features.wideLines = VK_TRUE;
        features2.features.shaderInt64 = VK_TRUE;
        features2.features.multiDrawIndirect = VK_TRUE;

        auto& features11 = specDev.GetFeature<VkPhysicalDeviceVulkan11Features>();
        features11.shaderDrawParameters = VK_TRUE;

        
        auto& features12 = specDev.GetFeature<VkPhysicalDeviceVulkan12Features>();
        features12.scalarBlockLayout = VK_TRUE;
        features12.bufferDeviceAddress = VK_TRUE;
        features12.bufferDeviceAddressCaptureReplay = EWE_DEBUG_BOOL ? VK_TRUE : VK_FALSE;
        features12.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
        features12.runtimeDescriptorArray = VK_TRUE;
        features12.descriptorBindingVariableDescriptorCount = VK_TRUE;
        features12.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
        features12.descriptorBindingUpdateUnusedWhilePending = VK_TRUE;
        features12.timelineSemaphore = VK_TRUE;
        features12.drawIndirectCount = VK_TRUE;

        //features12.descriptorBindingUniformBufferUpdateAfterBind;
        //features12.descriptorBindingStorageBufferUpdateAfterBind;
        features12.descriptorBindingPartiallyBound = VK_TRUE;

        features12.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
        features12.descriptorBindingStorageImageUpdateAfterBind = VK_TRUE;
        features12.descriptorBindingUniformTexelBufferUpdateAfterBind = VK_TRUE;
        features12.descriptorBindingStorageTexelBufferUpdateAfterBind = VK_TRUE;


        auto& features13 = specDev.GetFeature<VkPhysicalDeviceVulkan13Features>();
        features13.dynamicRendering = VK_TRUE;
        features13.synchronization2 = VK_TRUE;

        auto& features14 = specDev.GetFeature<VkPhysicalDeviceVulkan14Features>();
        features14.pushDescriptor = VK_TRUE;

        auto& devFaultFeatures = specDev.GetFeature<VkPhysicalDeviceFaultFeaturesEXT>();
        devFaultFeatures.deviceFault = VK_TRUE;
        devFaultFeatures.deviceFaultVendorBinary = VK_TRUE;

#if EWE_DEBUG_BOOL
        auto& dabReportFeatures = specDev.GetFeature<VkPhysicalDeviceAddressBindingReportFeaturesEXT>();
        dabReportFeatures.reportAddressBinding = VK_TRUE;
#endif

        auto evaluatedDevices = specDev.ScorePhysicalDevices(instance);

#if EWE_DEBUG_BOOL
        Log::Debug("evaluated %zu devices\n", evaluatedDevices.size());
        Log::Debug("if the first device failed, none are available\n");
        for(auto& dev : evaluatedDevices){
            Log::Debug("device name : %s\n", dev.name.c_str());
            Log::Debug("\trequirements : score : %d:%zu\n", dev.passedRequirements, dev.score);
            auto& props12 = dev.properties.properties.Get<VkPhysicalDeviceVulkan12Properties>();
            Log::Debug("\tdriver\n");
            Log::Debug("\t\tid:%zu\n", props12.driverID);
            Log::Debug("\t\tname:%s\n", props12.driverName);
            Log::Debug("\t\tinfo:%s\n", props12.driverInfo);

            const uint32_t variant_version = dev.api_version >> 29;
            const uint32_t major_version = (dev.api_version - (variant_version << 29)) >> 22;
            const uint32_t minor_version = (dev.api_version - (variant_version << 29) - (major_version << 22)) >> 12;
            const uint32_t patch_version = (dev.api_version - (variant_version << 29) - (major_version << 22) - (minor_version << 12));

            Log::Debug("api version - %d.%d.%d.%d\n", variant_version, major_version, minor_version, patch_version);
            if (dev.failureReport.size() > 0){
                Log::Debug("\tfailure report-\n");
                for(auto& fp : dev.failureReport){
                    Log::Debug("\t\tfp - %s\n", fp.c_str());
                }
            }
        }
#endif

        EWE::PhysicalDevice physicalDevice{ instance, evaluatedDevices[0].device, window.surface };
        VkBaseInStructure* pNextChain = reinterpret_cast<VkBaseInStructure*>(&specDev.features.base);
#ifdef USING_NVIDIA_AFTERMATH
        InitializeAftermath();

        VkDeviceDiagnosticsConfigCreateInfoNV nvDiagInfo{
            .sType = VK_STRUCTURE_TYPE_DEVICE_DIAGNOSTICS_CONFIG_CREATE_INFO_NV,
            .pNext = pNextChain,
            .flags = VK_DEVICE_DIAGNOSTICS_CONFIG_ENABLE_RESOURCE_TRACKING_BIT_NV |
                VK_DEVICE_DIAGNOSTICS_CONFIG_ENABLE_AUTOMATIC_CHECKPOINTS_BIT_NV |
                VK_DEVICE_DIAGNOSTICS_CONFIG_ENABLE_SHADER_DEBUG_INFO_BIT_NV,
        };
#endif



        return specDev.ConstructDevice(
            evaluatedDevices[0],
            std::forward<EWE::PhysicalDevice>(physicalDevice),
#ifdef USING_NVIDIA_AFTERMATH
            reinterpret_cast<VkBaseInStructure*>(&nvDiagInfo),
#else
            pNextChain,
#endif
            application_wide_vk_version,
            VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT
        );
    }

    Queue& GetPresentQueue(LogicalDevice& logicalDevice){    
        for (auto& queue : logicalDevice.queues) {
            if (queue.family.SupportsSurfacePresent() && queue.family.SupportsGraphics()) {
                return queue;
            }
        }
        EWE_UNREACHABLE;
    }

	Queue& GetComputeQueue(LogicalDevice& logicalDevice, Queue& renderQueue) {
		for (std::size_t i = 0; i < logicalDevice.queues.Size(); i++) {
			if (logicalDevice.queues[i].family.SupportsCompute() && (logicalDevice.queues[i] != renderQueue)) {
				return logicalDevice.queues[i];
			}
		}
		EWE_ASSERT(renderQueue.family.SupportsCompute());
		return renderQueue;
	}
	Queue& GetTransferQueue(LogicalDevice& logicalDevice, Queue& renderQueue, Queue& computeQueue) {
		for (std::size_t i = 0; i < logicalDevice.queues.Size(); i++) {
			if (logicalDevice.queues[i].family.SupportsTransfer() && (logicalDevice.queues[i] != computeQueue) && (logicalDevice.queues[i] != renderQueue)) {
				return logicalDevice.queues[i];
			}
		}
		if (computeQueue.family.SupportsTransfer()) {
			return computeQueue;
		}
		EWE_ASSERT(renderQueue.family.SupportsTransfer());
		return renderQueue;
	}

    std::unordered_map<std::string, bool> optionalExtensions{};

    EWEngine* engine;

    void TimeSemaphoreRelinquisher(TimelineSemaphore& sem){
        marl::Event event{ marl::Event::Mode::Manual };
        Log::Debug("marl wait for sem begin - %zu:%zu\n", sem.value, sem.GetCurrentValue());
        while (!sem.Check(sem.value)) {
            event.wait_for(std::chrono::microseconds(1)); //the poitn is to just relinquish control, we don't want to wait for a long time
        }
        Log::Debug("marl wait for sem end\n");
    }

    //the scheduler is unrelated, it's just the first thing I initialize
    // so it's packaged into this function. 
    // im taking advantage of the construction to insert the global pointer assignment
    marl::Scheduler::Config PointlessFunctionJustToSetTheGlobalVariable(EWEngine* this_ref){

        if (volkInitialize() != VK_SUCCESS) {
            throw std::runtime_error("Failed to initialize volk or find Vulkan loader");
        }

        EWE_ASSERT(engine == nullptr);
        engine = this_ref;

        EngineSettings::InitializeSettings();

        TimelineSemaphore::RelinquishThreadControl = TimeSemaphoreRelinquisher;

        STC_Manager::use_custom_yield = [&]{
            return marl::Scheduler::get() != nullptr; 
        };
        STC_Manager::custom_yield_func = [&](RingBuffer<TimelineSemaphore, 8>& ringBuffer, std::mutex& mut) -> TimelineSemaphore& {
            TimelineSemaphore* ret;

            marl::Event event{ marl::Event::Mode::Manual };
            while (true) {
                {
                    std::unique_lock<std::mutex> lock(mut);
                    if (!ringBuffer.Full()) {
                        ret = ringBuffer.GetNext();
                        break;
                    }
                }
                event.wait_for(std::chrono::microseconds(1));
            }
            return *ret;
        };

        return marl::Scheduler::Config::allCores();
    }

    EWEngine::EWEngine(std::string_view application_name, std::filesystem::path const& _root_directory)
    : root_directory{_root_directory},
        scheduler{PointlessFunctionJustToSetTheGlobalVariable(this)},
        instance{application_wide_vk_version, GetGLFWExtensions(), optionalExtensions },
        window{instance, 1280, 720, application_name},
        logicalDevice{CreateLogicalDevice(instance, window)},
        renderQueue{GetPresentQueue(logicalDevice)}, computeQueue{GetComputeQueue(logicalDevice, renderQueue)}, transferQueue{GetTransferQueue(logicalDevice, renderQueue, computeQueue)}, 
        swapchain{logicalDevice, window, renderQueue},
        stcManager{logicalDevice, renderQueue, transferQueue, computeQueue},
        current_renderGraph{stcManager.current_renderGraph},
        frameIndex{0},
        graphics_stc_task{"graphics STC task", logicalDevice, renderQueue},
        compute_stc_task{"compute STC task", logicalDevice, computeQueue},
        leafSystem{"models/leaf.obj"}
    {

        EWE_ASSERT(std::filesystem::is_directory(root_directory));

        graphics_stc_task.specializedSubmission = true;
        compute_stc_task.specializedSubmission = true;

        glfwSetDropCallback(window.window, GLFW_Drop_Callback);

		renderQueue.SetName("render queue");
		if(computeQueue == transferQueue){
			computeQueue.SetName("C&T queue, compute handle");
			transferQueue.SetName("C&T queue, transfer handle");
		}
		else{
			computeQueue.SetName("compute queue");
			transferQueue.SetName("transfer queue");
		}
    }

#if EWE_IMGUI

    bool DrawPresentModes(Swapchain& swapchain) {

        //std::string_view currentName = Reflect::Enum::ToString(swapchain.swapCreateInfo.presentMode);

        static std::vector<const char*> available_presents{};
        if (available_presents.size() == 0) {
            for (auto& present : swapchain.available_presentModes) {
                available_presents.push_back(Reflect::Enum::ToString(present).data());
            }
        }

        int index = 0;
        for (; index < swapchain.available_presentModes.size(); index++) {
            if (swapchain.swapCreateInfo.presentMode == swapchain.available_presentModes[index]) {
                break;
            }
        }

        if (ImGui::Combo("present mode", &index, available_presents.data(), available_presents.size())) {
            Log::Debug("present mode changed\n");

            swapchain.swapCreateInfo.presentMode = swapchain.available_presentModes[index];
            swapchain.wantsToRecreate = true;

            return true;
        }
        return false;
    }

    bool DrawSurfaceFormats(Swapchain& swapchain) {

        int index = 0;
        static std::vector<std::string> available_surfaces{};
        static std::vector<const char*> available_surfaces_c_str{};
        if (available_surfaces.size() == 0) {
            for (auto& surface_format : swapchain.available_surface_formats) {
                available_surfaces.push_back(
                    std::string(Reflect::Enum::ToString(surface_format.format).data()) + " : " +
                    Reflect::Enum::ToString(surface_format.colorSpace).data()
                );
            }
            for (auto const& cstr : available_surfaces) {
                available_surfaces_c_str.push_back(cstr.c_str());
            }
        }

        for (; index < swapchain.available_surface_formats.size(); index++) {
            if (swapchain.surface_format.format == swapchain.available_surface_formats[index].format
                && swapchain.surface_format.colorSpace == swapchain.available_surface_formats[index].colorSpace
                ) {
                break;
            }
        }
        if (ImGui::Combo("surface formats", &index, available_surfaces_c_str.data(), available_surfaces_c_str.size())) {

            swapchain.surface_format = swapchain.available_surface_formats[index];
            swapchain.swapCreateInfo.imageColorSpace = swapchain.available_surface_formats[index].colorSpace;
            swapchain.swapCreateInfo.imageFormat = swapchain.available_surface_formats[index].format;
            swapchain.wantsToRecreate = true;
            return true;
        }
        return false;
    }


	Queue::Type EWEngine::GetQueueType(Queue& queue) {
		if(queue == engine->renderQueue){
			return Queue::Graphics;
		}
		else if (queue == engine->computeQueue){
			return Queue::Compute;
		}
		else if (queue == engine->transferQueue){
			return Queue::Transfer;
		}
		EWE_UNREACHABLE;
	}

	Queue& EWEngine::GetQueue(Queue::Type type) {
		switch (type) {
			case Queue::Graphics: return engine->renderQueue;
			case Queue::Compute: return engine->computeQueue;
			case Queue::Transfer: return engine->transferQueue;
			default: EWE_UNREACHABLE;
		}
		EWE_UNREACHABLE;
	}

    lab::vec2 EWEngine::GetCursorWindowPos() {
        lab::vec2d ret;
        glfwGetCursorPos(engine->window.window, &ret.x, &ret.y);
        return lab::vec2{
            static_cast<float>(ret.x),
            static_cast<float>(ret.y)
        };
    }

    void EWEngine::Imgui() {
        static bool draw_demo = true;
        if (ImGui::Begin("engine")) {

            ImGui::Text(logicalDevice.physicalDevice.name.c_str());
            
            switch (glfwGetPlatform()) {
                case GLFW_PLATFORM_WIN32:   ImGui::Text("glfw platform : Win32\n"); break;
                case GLFW_PLATFORM_WAYLAND: ImGui::Text("glfw platform : Wayland\n"); break;
                case GLFW_PLATFORM_X11:     ImGui::Text("glfw platform : X11\n"); break;
                default:                    ImGui::Text("glfw platform : Unknown\n"); break;
            }
            if(ImGui::Checkbox("window resizeable", &window.resizeable)) {
                window.SetResizeable(window.resizeable);
            }

            if (DrawPresentModes(swapchain) || DrawSurfaceFormats(swapchain)) {
                Log::Debug("recreating swap at frame : %zu\n", totalFramesSubmitted);
            }

            ImGui::Text("working directory : %s", std::filesystem::current_path().string().c_str());
            ImGui::Checkbox("draw demo", &draw_demo);

            if(ImGui::Button("print out rendergraph debug")){
                EWEException ewe_except{VK_RESULT_MAX_ENUM, "fake error"};
                Debug_RenderGraph_DEVICE_LOST(*engine->current_renderGraph, ewe_except);
            }

        }
        ImGui::End();
        if (draw_demo) {
            ImGui::ShowDemoWindow();
        }
    }
#endif

}//namespace EWE