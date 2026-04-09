#include "EWEngine/EWEngine.h"

#include "EightWinds/Backend/DeviceSpecialization/Extensions.h"
#include "EightWinds/Backend/DeviceSpecialization/DeviceSpecialization.h"
#include "EightWinds/Backend/DeviceSpecialization/FeatureProperty.h"

#include "EWEngine/Global.h"
#include "GLFW/glfw3.h"
#include <filesystem>

#ifdef USING_NVIDIA_AFTERMATH
#include "EightWinds/Backend/nvidia/Aftermath.h"
#endif

#ifdef EWE_IMGUI
#include "imgui.h"
#endif

#include "EightWinds/Reflect/Enum.h"

namespace EWE{
    constexpr ConstEvalStr swapchainExt{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    //https://docs.vulkan.org/refpages/latest/refpages/source/VK_EXT_extended_dynamic_state3.html
    constexpr ConstEvalStr dynState3Ext{ VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME };
    constexpr ConstEvalStr meshShaderExt{ VK_EXT_MESH_SHADER_EXTENSION_NAME };
    constexpr ConstEvalStr deviceFaultExt{ VK_EXT_DEVICE_FAULT_EXTENSION_NAME };
    //this requires the instance extension VK_EXT_debug_utils. i don't know how to make that association cleanly
    constexpr ConstEvalStr dabReportExt{ VK_EXT_DEVICE_ADDRESS_BINDING_REPORT_EXTENSION_NAME };
    constexpr ConstEvalStr scalarBlockExt{ VK_EXT_SCALAR_BLOCK_LAYOUT_EXTENSION_NAME };

    using Example_ExtensionManager = ExtensionManager<application_wide_vk_version,
        ExtensionEntry<swapchainExt, true, 0>,
        ExtensionEntry<dynState3Ext, true, 0>,
        ExtensionEntry<meshShaderExt, false, 100000>,
        ExtensionEntry<deviceFaultExt, false, 10000>,
        ExtensionEntry<scalarBlockExt, true, 0>
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

        glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_WAYLAND);
        if (!glfwInit()) {
    #if EWE_DEBUG_BOOL
            Logger::Print<Logger::Debug>("failed to init glfw wayland\n");
    #endif
            glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
            if(!glfwInit()){
                throw std::runtime_error("failed to init glfw x11 or glfw");
            }
        }
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
        
        auto& features12 = specDev.GetFeature<VkPhysicalDeviceVulkan12Features>();
        features12.scalarBlockLayout = VK_TRUE;
        features12.bufferDeviceAddress = VK_TRUE;
        features12.bufferDeviceAddressCaptureReplay = EWE_DEBUG_BOOL ? VK_TRUE : VK_FALSE;
        features12.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
        features12.descriptorBindingPartiallyBound = VK_TRUE;
        features12.runtimeDescriptorArray = VK_TRUE;
        features12.descriptorBindingVariableDescriptorCount = VK_TRUE;
        features12.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
        features12.descriptorBindingUpdateUnusedWhilePending = VK_TRUE;
        features12.timelineSemaphore = VK_TRUE;

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
        Logger::Print<Logger::Debug>("evaluated %zu devices\n", evaluatedDevices.size());
        Logger::Print<Logger::Debug>("if the first device failed, none are available\n");
        for(auto& dev : evaluatedDevices){
            Logger::Print<Logger::Debug>("device name : %s\n", dev.name.c_str());
            Logger::Print<Logger::Debug>("\trequirements : score : %d:%zu\n", dev.passedRequirements, dev.score);
            auto& props12 = dev.properties.properties.Get<VkPhysicalDeviceVulkan12Properties>();
            Logger::Print<Logger::Debug>("\tdriver\n");
            Logger::Print<Logger::Debug>("\t\tid:%zu\n", props12.driverID);
            Logger::Print<Logger::Debug>("\t\tname:%s\n", props12.driverName);
            Logger::Print<Logger::Debug>("\t\tinfo:%s\n", props12.driverInfo);

            const uint32_t variant_version = dev.api_version >> 29;
            const uint32_t major_version = (dev.api_version - (variant_version << 29)) >> 22;
            const uint32_t minor_version = (dev.api_version - (variant_version << 29) - (major_version << 22)) >> 12;
            const uint32_t patch_version = (dev.api_version - (variant_version << 29) - (major_version << 22) - (minor_version << 12));

            Logger::Print<Logger::Debug>("api version - %d.%d.%d.%d\n", variant_version, major_version, minor_version, patch_version);
            if (dev.failureReport.size() > 0){
                Logger::Print<Logger::Debug>("\tfailure report-\n");
                for(auto& fp : dev.failureReport){
                    Logger::Print<Logger::Debug>("\t\tfp - %s\n", fp.c_str());
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
        throw std::runtime_error("failed to find a present queue");
    }

	Queue& GetComputeQueue(LogicalDevice& logicalDevice, Queue& renderQueue) {
		for (std::size_t i = 0; i < logicalDevice.queues.Size(); i++) {
			if (logicalDevice.queues[i].family.SupportsCompute() && (logicalDevice.queues[i] != renderQueue)) {
				return logicalDevice.queues[i];
			}
		}
		EWE_ASSERT(renderQueue.family.SupportsTransfer());
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

    EWEngine::EWEngine(std::string_view application_name)
        : instance{application_wide_vk_version, GetGLFWExtensions(), optionalExtensions },
        window{instance, 1920, 1080, application_name},
        logicalDevice{CreateLogicalDevice(instance, window)},
        renderQueue{GetPresentQueue(logicalDevice)}, computeQueue{GetComputeQueue(logicalDevice, renderQueue)}, transferQueue{GetTransferQueue(logicalDevice, renderQueue, computeQueue)}, 
        swapchain{logicalDevice, window, renderQueue},
        renderGraph{logicalDevice, swapchain, renderQueue, computeQueue},
        stcManager{logicalDevice, transferQueue, renderGraph}//,
        /*
        textOverlay{ 
            logicalDevice, 
            static_cast<float>(window.screenDimensions.width), 
            static_cast<float>(window.screenDimensions.height) 
        }
        */
    {
        Global::Create(logicalDevice, window, stcManager, std::filesystem::current_path());
    }

#if EWE_IMGUI

    bool DrawPresentModes(Swapchain& swapchain) {

        std::string_view currentName = Reflect::Enum::ToString(swapchain.swapCreateInfo.presentMode);

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
            Logger::Print<Logger::Debug>("present mode changed\n");

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

    void EWEngine::Imgui() {
        if (ImGui::Begin("engine")) {

            ImGui::Text(logicalDevice.physicalDevice.name.c_str());
            
            switch (glfwGetPlatform()) {
                case GLFW_PLATFORM_WIN32:   ImGui::Text("glfw platform : Win32\n"); break;
                case GLFW_PLATFORM_COCOA:   ImGui::Text("glfw platform : Cocoa (macOS)\n"); break;
                case GLFW_PLATFORM_WAYLAND: ImGui::Text("glfw platform : Wayland\n"); break;
                case GLFW_PLATFORM_X11:     ImGui::Text("glfw platform : X11\n"); break;
                default:                    ImGui::Text("glfw platform : Unknown\n"); break;
            }
            if(ImGui::Checkbox("window resizeable", &window.resizeable)) {
                window.SetResizeable(window.resizeable);
            }

            if (DrawPresentModes(swapchain) || DrawSurfaceFormats(swapchain)) {
                Logger::Print<Logger::Debug>("recreating swap at frame : %zu\n", totalFramesSubmitted);
            }

            ImGui::Text("working directory : %s", std::filesystem::current_path().string().c_str());


            static bool draw_demo = false;
            ImGui::Checkbox("draw demo", &draw_demo);
            if (draw_demo) {
                ImGui::ShowDemoWindow();
            }

        }
        ImGui::End();
    }
#endif

}//namespace EWE