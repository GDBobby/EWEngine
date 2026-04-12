#include "EWEngine/Global.h"


#include <thread>

namespace EWE{
    struct GlobalData{
        const std::thread::id mainThreadID;

        LogicalDevice& logicalDevice;
        Window& window;
        marl::Scheduler scheduler;
        STC_Manager& stcManager;

        AssetManager assetManager;

        [[nodiscard]] explicit GlobalData(LogicalDevice& _logicalDevice, Window& _window, STC_Manager& _stcManager, std::filesystem::path const& root_path)
        : logicalDevice{_logicalDevice},
            window{_window},
            scheduler{marl::Scheduler::Config::allCores()},
            stcManager{_stcManager},
            assetManager{root_path}
        {
            glfwSetDropCallback(window.window, GLFW_Drop_Callback);
        }
    };

    GlobalData* globalData = nullptr;



    namespace Global{
        LogicalDevice* logicalDevice;
        PhysicalDevice* physicalDevice;
        Instance* instance;
        Window* window;
        uint8_t frameIndex;
        
        marl::Scheduler* scheduler;
        STC_Manager* stcManager;

        AssetManager* assetManager;

        
        bool Create(LogicalDevice& _logicalDevice, Window& _window, STC_Manager& _stcManager, std::filesystem::path const& _root_path) {
            EWE_ASSERT(globalData == nullptr);
            globalData = new GlobalData{_logicalDevice, _window, _stcManager, _root_path};

            ::EWE::Global::logicalDevice = &globalData->logicalDevice;
            physicalDevice = &_logicalDevice.physicalDevice;
            instance = &_logicalDevice.instance;
            ::EWE::Global::window = &globalData->window;
            frameIndex = 0; 
            ::EWE::Global::scheduler = &globalData->scheduler;
            ::EWE::Global::stcManager = &globalData->stcManager;
            ::EWE::Global::assetManager = &globalData->assetManager;

            return true;
        }
    } //namespace Global

#ifdef _WIN32
#defien WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <processthreadsapi.h>
#elif defined(__linux__) || defined(__APPLE__)
#include <pthread.h>
#endif

    bool CheckMainThread() {
        return std::this_thread::get_id() == globalData->mainThreadID;
    }
    void NameCurrentThread(std::string_view name){
#ifdef _WIN32
        std::wstring wname(name.begin(), name.end());
        SetThreadDescription(GetCurrentThread(), wname.c_str());

#elif defined(__linux__)
        //linux has a 16 char limit for thread name?
        char buf[16];
        auto len = std::min(name.size(), (size_t)15);
        std::copy_n(name.begin(), len, buf);
        buf[len] = '\0';
        pthread_setname_np(pthread_self(), buf);
#endif
    }
} //namespace EWE