#include "EWEngine/Global.h"


#include <thread>

namespace EWE{
    struct GlobalData{
        const std::thread::id mainThreadID;

        LogicalDevice& logicalDevice;
        Window& window;
        marl::Scheduler scheduler;
        STC_Manager& stcManager;

        Asset::Manager<Shader> shaderManager;
        Asset::Manager<Sampler> samplerManager;
        Asset::Manager<Image> images;
        Asset::Manager<ImageView> views;
        Asset::Manager<DescriptorImageInfo> diis;

        [[nodiscard]] explicit GlobalData(LogicalDevice& logicalDevice, Window& window, STC_Manager& stcManager, std::filesystem::path const& root_path)
        : logicalDevice{logicalDevice},
            window{window},
            scheduler{marl::Scheduler::Config::allCores()},
            stcManager{stcManager},
            shaderManager{logicalDevice, root_path / "shaders"},
            samplerManager{logicalDevice},
            images{logicalDevice, root_path},
            views{logicalDevice, images},
            diis{logicalDevice, samplerManager, views, root_path}
        {

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

        Asset::Manager<Shader>* shaders;
        Asset::Manager<Sampler>* samplers;
        Asset::Manager<Image>* images;
        Asset::Manager<ImageView>* views;
        Asset::Manager<DescriptorImageInfo>* diis;

        
        bool Create(LogicalDevice& logicalDevice, Window& window, STC_Manager& stcManager, std::filesystem::path const& root_path) {
            EWE_ASSERT(globalData == nullptr);
            globalData = new GlobalData{logicalDevice, window, stcManager, root_path};

            ::EWE::Global::logicalDevice = &globalData->logicalDevice;
            physicalDevice = &logicalDevice.physicalDevice;
            instance = &logicalDevice.instance;
            ::EWE::Global::window = &globalData->window;
            frameIndex = 0; 
            ::EWE::Global::scheduler = &globalData->scheduler;
            ::EWE::Global::stcManager = &globalData->stcManager;
            ::EWE::Global::shaders = &globalData->shaderManager;
            ::EWE::Global::samplers = &globalData->samplerManager;
            ::EWE::Global::images = &globalData->images;
            ::EWE::Global::views = &globalData->views;
            ::EWE::Global::diis = &globalData->diis;

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