#pragma once

#include "EWEngine/Scene.h"

#include <unordered_map>

namespace EWE {

	//this class does not handle destruction of scenes, left explicitly to user
	class SceneManager {
	public:
		[[nodiscard]] SceneManager();
		~SceneManager();

		void RunSceneLoop();

		void ChangeScene(SceneKey sceneKey);

		SceneKey AddScene(SceneBase* scene);

		void SetStartupScene(SceneKey sceneKey);

		const SceneKey scene_exit = 255;
	private:
		SceneKey currentScene;

		//SceneKey 0 is reserved for the loading_scene, which isn't technically a scene. 
		//It just represents the transition from loading to the main scene
		//this is necessary so that entry is called on the first scene from the game built on the engine
		SceneKey lastScene = 0;
		SceneBase* currentScenePtr{ nullptr };
		bool swappingScenes = false;

		bool gameRunning = true;

		std::unordered_map<SceneKey, SceneBase*> scenes;

		void SwapScenes();
	};
}//namespace EWE

