#include "EWEngine/Global.h"

#include "EWEngine/EWEngine.h"

#include "EWEngine/TextOverlay.h"
#include "EightWinds/Data/Address.h"

#include <type_traits>

namespace EWE{
namespace Global{

    struct GlobalEngineObj{
        AssetManager assetManager;
        TextOverlay textOverlay;
        SoundEngine soundEngine;
        SceneManager sceneManager;
        ImguiHandler imguiHandler;

        GlobalEngineObj(std::filesystem::path const& root_directory)
        : assetManager{root_directory, engine->logicalDevice, engine->renderQueue},
            textOverlay{},
            soundEngine{},
            sceneManager{},
            imguiHandler{engine->renderQueue, engine->swapchain.images.Size(), VK_SAMPLE_COUNT_1_BIT}
        {}
    };

    template<typename T>
    struct ConstructionDelayer{
        alignas(T) std::byte data[sizeof(T)];

        bool initialized = false;

        T* GetPtr(){
            return reinterpret_cast<T*>(data);
        }
        T& GetRef(){
            return *GetPtr();
        }

        Address GetAddress(){
            return Address{data};
        }
    };

    ConstructionDelayer<GlobalEngineObj> globalObj{};

    AssetManager* assetManager;
    TextOverlay* textOverlay;
    SoundEngine* soundEngine;
    SceneManager* sceneManager;
    ImguiHandler* imguiHandler;


    void InitEngineGlobal(std::filesystem::path const& root_directory){
        GlobalEngineObj& gref = globalObj.GetRef();
        assetManager = std::construct_at(&gref.assetManager, root_directory, engine->logicalDevice, engine->renderQueue);
        textOverlay = std::construct_at(&gref.textOverlay);
        soundEngine = std::construct_at(&gref.soundEngine);
        sceneManager = std::construct_at(&gref.sceneManager);
        imguiHandler = std::construct_at(&gref.imguiHandler, engine->renderQueue, engine->swapchain.images.Size(), VK_SAMPLE_COUNT_1_BIT);
    }
}
} //namespace EWE