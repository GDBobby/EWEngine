#pragma once

#include "EWEngine/Data/ConstructionDelayer.h"

#include "EWEngine/Graphics/TextOverlay.h"
#include "EWEngine/Imgui/ImguiHandler.h"
#include "EWEngine/Systems/SceneManager.h"
#include "EWEngine/Assets/Manager.h"
#include "EWEngine/Systems/Sound_Engine.h"

#include "EWEngine/Graphics/SpecializedTasks/MergeTask.h"
#include "EWEngine/Graphics/SpecializedTasks/ImguiTask.h"

namespace EWE{

namespace Global{
    extern AssetManager* assetManager;
    extern TextOverlay* textOverlay;
    extern SoundEngine* soundEngine;
    extern SceneManager* sceneManager;
    extern ImguiHandler* imguiHandler;

    extern MergeTask* mergeTask;
    extern ImguiTask* imguiTask;

    void InitEngineGlobal(std::filesystem::path const& path);
} //namespace Global
} //namepsace EWE