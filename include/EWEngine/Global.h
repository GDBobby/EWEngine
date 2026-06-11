#pragma once

#include "EWEngine/TextOverlay.h"
#include "EWEngine/Imgui/ImguiHandler.h"
#include "EWEngine/Systems/SceneManager.h"
#include "EWEngine/Assets/Manager.h"
#include "EWEngine/Systems/Sound_Engine.h"

namespace EWE{
namespace Global{
    extern AssetManager* assetManager;
    extern TextOverlay* textOverlay;
    extern SoundEngine* soundEngine;
    extern SceneManager* sceneManager;
    extern ImguiHandler* imguiHandler;

    void InitEngineGlobal(std::filesystem::path const& path);
} //namespace Global
} //namepsace EWE