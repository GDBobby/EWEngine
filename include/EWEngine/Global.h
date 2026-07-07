#pragma once

#include "EWEngine/Data/ConstructionDelayer.h"

#include "EWEngine/Graphics/TextOverlay.h"
#include "EWEngine/Imgui/ImguiHandler.h"
#include "EWEngine/Systems/SceneManager.h"
#include "EWEngine/Assets/Manager.h"
#include "EWEngine/Systems/Sound_Engine.h"

#include "EWEngine/Graphics/SpecializedTasks/MergeTask.h"
#include "EWEngine/Graphics/SpecializedTasks/ImguiTask.h"

#include "LAB/Vector.h"

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

    constexpr float DefaultHeight_PixelRatio(uint32_t pixel){
        return (0.5f - (static_cast<float>(pixel) / 1080.f)) * 2.f;
    }
} //namespace Global
} //namepsace EWE