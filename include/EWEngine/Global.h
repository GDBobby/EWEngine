#pragma once

#include "EightWinds/VulkanHeader.h"
#include "marl/scheduler.h"

#include "EightWinds/LogicalDevice.h"
#include "EightWinds/Window.h"
#include "EWEngine/STC_Manager.h"

#include "EWEngine/Assets/Manager.h"

#include "EWEngine/Systems/Sound_Engine.h"

#include <cstdint>

namespace EWE{

    static constexpr uint32_t application_wide_vk_version = VK_MAKE_API_VERSION(0, 1, 4, 0);
    static constexpr uint32_t rounded_down_vulkan_version = EWE::RoundDownVkVersion(application_wide_vk_version);

    void SetMainThread();
    bool CheckMainThread();
    void NameCurrentThread(std::string_view name);
} //namespace EWE