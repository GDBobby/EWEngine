#pragma once

#include "EightWinds/Command/InstructionPackage.h"
#include "EightWinds/Command/PackageRecord.h"
#include "EightWinds/VulkanHeader.h"
#include "marl/scheduler.h"


#include "EightWinds/LogicalDevice.h"
#include "EightWinds/Window.h"
#include "EWEngine/STC_Manager.h"
#include "EightWinds/Shader.h"

#include "EWEngine/Assets/Shaders.h"
#include "EWEngine/Assets/Samplers.h"
#include "EWEngine/Assets/Images.h"
#include "EWEngine/Assets/ImageViews.h"
#include "EWEngine/Assets/DII.h"
#include "EWEngine/Assets/Records.h"
#include "EWEngine/Assets/Buffers.h"
#include "EWEngine/Assets/InstPackages.h"
#include "EWEngine/Assets/PackageRecords.h"
#include "EWEngine/Assets/SubmissionTasks.h"

#include <cstdint>

namespace EWE{

    static constexpr uint32_t application_wide_vk_version = VK_MAKE_API_VERSION(0, 1, 4, 0);
    static constexpr uint32_t rounded_down_vulkan_version = EWE::RoundDownVkVersion(application_wide_vk_version);

    namespace Global{
        extern LogicalDevice* logicalDevice;
        extern PhysicalDevice* physicalDevice;
        extern Instance* instance;
        extern Window* window;
        extern uint8_t frameIndex;

        extern marl::Scheduler* scheduler;
        extern Asset::Manager<Shader>* shaders;
        extern Asset::Manager<Sampler>* samplers;
        extern Asset::Manager<Image>* images;
        extern Asset::Manager<ImageView>* views;
        extern Asset::Manager<DescriptorImageInfo>* diis;
        extern Asset::Manager<Command::Record>* records;
        extern Asset::Manager<Buffer>* buffers;
        extern Asset::Manager<Command::InstructionPackage>* instPackages;
        extern Asset::Manager<Command::PackageRecord>* pkgRecords;
        extern Asset::Manager<SubmissionTask>* subTasks;

        extern STC_Manager* stcManager;
        //dont move logicaldevice, just construct it in place
        bool Create(LogicalDevice& logicalDevice, Window& window, STC_Manager& stcManager, std::filesystem::path const& root_path);
    }

    bool CheckMainThread();
    void NameCurrentThread(std::string_view name);
} //namespace EWE