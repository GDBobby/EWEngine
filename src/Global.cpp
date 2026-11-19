#include "EWEngine/Global.h"

#include "EWEngine/EWEngine.h"

#include <memory>
#include <functional>

#include "../include/EWEngine/Graphics/TextOverlay.h"
#include "EightWinds/Data/Address.h"

namespace EWE {
    namespace Global {

        struct GlobalEngineObj {
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

        std::function<void()> OnShutdown = nullptr;

        void ShutdownEngineGlobal() {
            GlobalEngineObj& gref = globalObj.GetRef();

            std::destroy_at(&gref.sceneManager);
            sceneManager = nullptr;

            if (OnShutdown) {
                std::function<void()> callback = std::move(OnShutdown);
                OnShutdown = nullptr;
                callback();
            }
            else {
                Log::Debug("shutting down but shutdown callback is nullptr\n");
            }

            std::destroy_at(&gref.imguiTask);
            imguiTask = nullptr;
            std::destroy_at(&gref.mergeTask);
            mergeTask = nullptr;
            std::destroy_at(&gref.textOverlay);
            textOverlay = nullptr;
            std::destroy_at(&gref.imguiHandler);
            imguiHandler = nullptr;
            std::destroy_at(&gref.assetManager);
            assetManager = nullptr;
            std::destroy_at(&gref.soundEngine);
            soundEngine = nullptr;
        }

        void InitEngineGlobal(std::filesystem::path const& root_directory) {
            GlobalEngineObj& gref = globalObj.GetRef();
            assetManager =
                std::construct_at(&gref.assetManager, root_directory, engine->logicalDevice, engine->renderQueue);
            textOverlay = std::construct_at(&gref.textOverlay);
            soundEngine = std::construct_at(&gref.soundEngine);
            sceneManager = std::construct_at(&gref.sceneManager);
            imguiHandler = std::construct_at(
                &gref.imguiHandler,
                engine->renderQueue,
                engine->swapchain.images.Size(),
                VK_SAMPLE_COUNT_1_BIT
            );

            mergeTask = std::construct_at(&gref.mergeTask);
            imguiTask = std::construct_at(&gref.imguiTask);
        }
    } // namespace Global
} // namespace EWE