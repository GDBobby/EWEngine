#pragma once

#include "EightWinds/Reflect/Enum.h"

#include <GLFW/glfw3.h>

#include <vector>
#include <string>
#include <filesystem>
#include <fstream>
#include <type_traits>



namespace EWE{
static constexpr uint64_t CURRENT_SETTINGS_VERSION = 12;
#define SETTINGS_LOCATION "settings.json"


enum class SoundVolume {
	master,
	effect,
	music,
	voice,

	//overflow,
};

namespace EngineSettings {
	struct ScreenDimensionsFloat {
		float width;
		float height;

		bool operator==(ScreenDimensionsFloat const other) const {
			return (width == other.width) && (height == other.height);
		}
		bool operator!=(ScreenDimensionsFloat const other) const {
			return !this->operator==(other);
		}
	};

	struct ScreenDimensions {
		uint32_t width;
		uint32_t height;

		static std::vector<ScreenDimensions> commonDimensions;
		static void FixCommonDimensionsToScreenSize(int width, int height);

		ScreenDimensionsFloat ConvertToFloat();
		bool operator==(ScreenDimensions const other) const {
			return (width == other.width) && (height == other.height);
		}
		bool operator!=(ScreenDimensions const other) const {
			return !this->operator==(other);
		}

		std::string GetString() const;
	};

	enum class WindowMode {
		windowed,
		borderless,
		//fullscreen,


		COUNT,
	};
	std::vector<std::string> GetWindowModeStringVector();
	std::string GetWindowModeString(WindowMode wm);

	struct SettingsData {
		int versionKey = CURRENT_SETTINGS_VERSION; //i dont think i can use periods, maybe somethin else like X

		EngineSettings::WindowMode windowMode{ EngineSettings::WindowMode::borderless };
		EngineSettings::ScreenDimensions screenDimensions{1920, 1080};

		//swapping to uint8_t, with no 0 - 100
		uint8_t masterVolume{ 50 }; //volume is 0 ~ 1, 1 being 100%
		uint8_t effectsVolume{ 50 };
		uint8_t musicVolume{ 50 };
		uint8_t voiceVolume{ 50 };
		std::string selectedDevice{ "default" };
		uint16_t FPS = 144;
		bool pointLights = false;
		bool renderInfo = false;

		void SetVolume(int8_t whichVolume, uint8_t value);
		const uint8_t& getVolume(int8_t whichVolume);

		std::string GetDimensionsString() {
			return screenDimensions.GetString();
		}
		std::string_view GetWindowModeString() {
			return Reflect::Enum::ToString(windowMode);
		}
	};
	extern SettingsData settingsData;
	extern SettingsData tempSettings;

	static void InitializeSettings();

	static void GenerateDefaultFile();
	static void SaveToJsonFile();

} //namespace EngineSettings
} //namespace EWE