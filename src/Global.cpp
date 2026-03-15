#include "EWEngine/Global.h"

#include "EightWinds/LogicalDevice.h"
#include "EightWinds/Window.h"
#include "EWEngine/STC_Manager.h"

#include "EWEngine/Tools/FileResource.h"

#include <thread>

namespace EWE{
    struct GlobalData{
        const std::thread::id mainThreadID;

        LogicalDevice& logicalDevice;
        PhysicalDevice& physicalDevice;
        Instance& instance;
        Window& window;
        marl::Scheduler scheduler;
        STC_Manager& stcManager;

        ShaderFileSystem shaderFS;
    };

    GlobalData* globalData = nullptr;


    bool CheckMainThread() {
        return std::this_thread::get_id() == globalData->mainThreadID;
    }

    namespace Global{
        LogicalDevice* logicalDevice;
        PhysicalDevice* physicalDevice;
        Instance* instance;
        Window* window;
        uint8_t frameIndex;
        
        marl::Scheduler* scheduler;
        STC_Manager* stcManager;

        ShaderFileSystem* shaderFS;

        
        bool Create(LogicalDevice& logicalDevice, Window& window, STC_Manager& stcManager) {
#if EWE_DEBUG_BOOL
            assert(globalData == nullptr);
#endif
            globalData = new GlobalData{
                .mainThreadID = std::this_thread::get_id(),
                .logicalDevice = logicalDevice,
                .physicalDevice = logicalDevice.physicalDevice,
                .instance = logicalDevice.instance,
                .window = window,
                .scheduler{marl::Scheduler::Config::allCores()},
                .stcManager = stcManager,
                .shaderFS{logicalDevice, "/home/Projects/EWEngine/examples/common/shaders"}
            };

            ::EWE::Global::logicalDevice = &globalData->logicalDevice;
            physicalDevice = &globalData->physicalDevice;
            instance = &globalData->instance;
            ::EWE::Global::window = &globalData->window;
            frameIndex = 0; 
            ::EWE::Global::scheduler = &globalData->scheduler;
            ::EWE::Global::shaderFS = &globalData->shaderFS;

            return true;
        }
    } //namespace Global
} //namespace EWE