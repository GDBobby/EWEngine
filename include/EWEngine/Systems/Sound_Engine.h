#pragma once

//#include "EWEngine/Graphics/Preprocessor.h"
#include "EWEngine/EngineSettings.h"
#include "EWEngine/resources/howlingWind.h"


#define MINIAUDIO_IMPLEMENTATION
#if defined(MA_WIN32) //miniaudio brings in <windows> (clenches fist)
#define NOMINMAX
#endif

// https://miniaud.io/docs/examples/data_source_chaining.html
//for making the end of a song play the next, seems straightforward
//i would imagine this is less than 15min to implement, i just dont have multiple sources to loop currently
#include "EWEngine/miniaudio/miniaudio.h"

#include <string>
#include <vector>
#include <array>
#include <unordered_map>
#include <filesystem>

namespace EWE {
	class SoundEngine {

	public:
		SoundEngine();
		//~RigidRenderingSystem() = default;
		SoundEngine(const SoundEngine&) = delete;
		SoundEngine& operator=(const SoundEngine&) = delete;

		~SoundEngine();

		void PlayMusic(uint16_t whichSong, bool repeat = false);
		void PlayEffect(uint16_t whichEffect, bool looping = false);
		void StopEffect(uint16_t whichEffect);
		void RestartEffect(uint16_t whichEffect, bool looping = false);
		void PlayVoice(uint16_t whichVoice) {} //idk

		void PlayNextSong() {}
		void StopMusic();

		float GetVolume(SoundVolume whichVolume) { return volumes[(uint8_t)whichVolume]; }
		float GetVolume(int8_t whichVolume) { return volumes[whichVolume]; }

		//0 is the default device
		void SwitchDevices(uint16_t deviceIterator);
		void SetVolume(SoundVolume whichVolume, uint8_t value);
		//void loadEffects(std::unordered_map<uint16_t, std::string>& loadEffects);
		void LoadSoundMap(std::unordered_map<uint16_t, std::filesystem::path>& loadSounds, SoundVolume soundType);

		int16_t AddMusicToBack(std::string const& musicLocation);

		std::vector<std::string> deviceNames;
		uint16_t GetSelectedDevice() {
			return selectedEngine;
		}

		void InitVolume() {
			SetVolume(SoundVolume::master, EngineSettings::settingsData.masterVolume);
			SetVolume(SoundVolume::effect, EngineSettings::settingsData.effectsVolume);
			SetVolume(SoundVolume::music, EngineSettings::settingsData.musicVolume);
			SetVolume(SoundVolume::voice, EngineSettings::settingsData.voiceVolume);
		}


	private:
		//ma_sound_group effectGroup;
		//ma_sound_group musicGroup;
		//ma_sound_group voiceGroup;
		std::vector<std::unordered_map<uint16_t, ma_sound>> effects;
		std::vector<std::unordered_map<uint16_t, ma_sound>> music;
		std::vector<std::unordered_map<uint16_t, ma_sound>> voices;
		std::unordered_map<uint16_t, std::filesystem::path> effectLocations;
		std::unordered_map<uint16_t, std::filesystem::path> musicLocations;
		std::unordered_map<uint16_t, std::filesystem::path> voiceLocations;

	public:
		static constexpr uint16_t howling_wind_index = 65534;
	private:
		ma_decoder hwDecoder;
		ma_sound hwSound;

		std::array<float, 4> volumes{ 0.5f, 0.5f, 0.5f, 0.5f };
		
		ma_context context;
		ma_resource_manager_config resourceManagerConfig;
		ma_resource_manager resourceManager;
		std::vector<ma_engine> engines{};
		uint16_t selectedEngine{UINT16_MAX};

		std::vector<ma_device> devices{ 0 };

		ma_device_info* pPlaybackDeviceInfos; 
		ma_uint32 playbackDeviceCount{ 0 };
		//std::vector<ma_sound>sounds{};
		//ma_sound* selectedSound{ nullptr };

		void LoadHowlingWind();
		void InitEngines(ma_device_info* deviceInfos, uint32_t deviceCount);
		void ReloadSounds();

		uint16_t currentSong = UINT16_MAX;
		ma_uint64 currentPCMFrames = 0;

	
	};
} //namespace EWE