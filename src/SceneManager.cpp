#include "EWEngine/Systems/SceneManager.h"

#include "EWEngine/EWEngine.h"


namespace EWE {
	SceneManager::SceneManager() {

		//this prevents realignment
		scenes.reserve(255);
	}
	SceneManager::~SceneManager() {
		for (auto& scene : scenes) {
			delete scene.second;
		}
	}

	void SceneManager::RunSceneLoop() {
		auto& render_loop = engine->render_loop_timer;
		render_loop.last_time = std::chrono::high_resolution_clock::now();
		render_loop.SetLoopDuration();

		/*
		do { //having a simple while() may cause a race condition
			EWE_VK(vkDeviceWaitIdle, engine->logicalDevice.device);
		} while (ewEngine.GetLoadingScreenProgress());
		*/

		currentScenePtr->Entry();

		while (gameRunning) {
			glfwPollEvents();

			if (swappingScenes) [[unlikely]] {
				SwapScenes();
				render_loop.delta = render_loop.duration;
				Log::Debug("swapping scenes beginning \n");
				//stop loading screen here
			}
			else if (render_loop.ReadyForUpdate()) {
				//Log::Debug("currentScene at render : %u\n");
				//ewEngine.camera.PrintCameraPos();
				currentScenePtr->Render(render_loop.delta.count());
				render_loop.delta = LoopTimer::DurationType{0};
			}
		}
	}
	void SceneManager::ChangeScene(SceneKey sceneKey) {
#if EWE_DEBUG
		EWE_ASSERT(lastScene != currentScene && "changing scenes too quickly");
#endif
		currentScene = sceneKey;
		swappingScenes = true;
	}
	
	uint8_t SceneManager::AddScene(SceneBase* scene) {
		EWE_ASSERT(scene != nullptr);
		EWE_ASSERT((scenes.size() + 1) < scene_exit);
		
		SceneKey currentSize = static_cast<SceneKey>(scenes.size() + 1);
		scenes.emplace(currentSize, scene);

		return currentSize;
	}

	void SceneManager::SetStartupScene(SceneKey sceneKey) {
		currentScene = sceneKey;
		currentScenePtr = scenes.at(sceneKey);
	}

	void SceneManager::SwapScenes() {
		EWE_VK(vkDeviceWaitIdle, engine->logicalDevice);
		currentScenePtr->Exit();
		if (currentScene != scene_exit) {
			currentScenePtr = scenes.at(currentScene);
			currentScenePtr->Load();
			currentScenePtr->Entry();
			lastScene = currentScene;
			swappingScenes = false;
		}
		else {
			gameRunning = false;
		}
	}

}//namespace EWE
