#include "EWEngine/Global.h"

#include "EWEngine/EWEngine.h"

#include "../include/EWEngine/Graphics/TextOverlay.h"
#include "EightWinds/Data/Address.h"

namespace EWE{
namespace Global{

    struct GlobalEngineObj{
        AssetManager assetManager;
        TextOverlay textOverlay;
        SoundEngine soundEngine;
        SceneManager sceneManager;
        ImguiHandler imguiHandler;

        MergeTask mergeTask;
        ImguiTask imguiTask;
    };

    ConstructionDelayer<GlobalEngineObj> globalObj{};

    AssetManager* assetManager = nullptr;
    TextOverlay* textOverlay = nullptr;
    SoundEngine* soundEngine = nullptr;
    SceneManager* sceneManager = nullptr;
    ImguiHandler* imguiHandler = nullptr;

    MergeTask* mergeTask = nullptr;
    ImguiTask* imguiTask = nullptr;


    void InitEngineGlobal(std::filesystem::path const& root_directory){
        GlobalEngineObj& gref = globalObj.GetRef();
        assetManager = std::construct_at(&gref.assetManager, root_directory, engine->logicalDevice, engine->renderQueue);
        textOverlay = std::construct_at(&gref.textOverlay);
        soundEngine = std::construct_at(&gref.soundEngine);
        sceneManager = std::construct_at(&gref.sceneManager);
        imguiHandler = std::construct_at(&gref.imguiHandler, engine->renderQueue, engine->swapchain.images.Size(), VK_SAMPLE_COUNT_1_BIT);
    
        mergeTask = std::construct_at(&gref.mergeTask);
        imguiTask = std::construct_at(&gref.imguiTask);
    }


    float ConvertP2V_Width(int width){
        return ((static_cast<float>(width) / engine->window.screenDimensions.width) - 0.5f) * 2.f;
    }
    float ConvertP2V_Height(int height){
        return (0.5f - (static_cast<float>(height) / engine->window.screenDimensions.height)) * 2.f;
    }
    lab::vec2 ConvertP2V(lab::ivec2 dimensions){
        return lab::vec2{
            ConvertP2V_Width(dimensions.x),
            ConvertP2V_Height(dimensions.y)
        };
    }
} //namespace Global
} //namespace EWE