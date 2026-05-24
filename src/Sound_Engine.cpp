#include "EWEngine/Systems/Sound_Engine.h"

#include "EightWinds/Reflect/Enum.h"

#include <filesystem>

#define EFFECTS_PATH "sounds/effects/"
#define MUSIC_PATH "sounds/music/"
#define VOICE_PATH "sounds/voice/"

void ma_data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
	(void)pInput;

	/*
	Since we're managing the underlying device ourselves, we need to read from the engine directly.
	To do this we need access to the ma_engine object which we passed in to the user data. One
	advantage of this is that you could do your own audio processing in addition to the engine's
	standard processing.
	*/
	ma_engine_read_pcm_frames((ma_engine*)pDevice->pUserData, pOutput, frameCount, NULL);
}

namespace EWE {
	SoundEngine::SoundEngine() {
		ma_result result;

		resourceManagerConfig = ma_resource_manager_config_init();
		resourceManagerConfig.decodedFormat     = ma_format_f32;
		resourceManagerConfig.decodedChannels = 0; // Setting the channel count to 0 will cause sounds to use their native channel count
		resourceManagerConfig.decodedSampleRate = 48000;// Using a consistent sample rate is useful for avoiding expensive resampling in the audio thread. This will result in resampling being performed by the loading thread(s).

		result = ma_resource_manager_init(&resourceManagerConfig, &resourceManager);
		assert(result == MA_SUCCESS && "failed to initialize resource manager");
		result = ma_context_init(NULL, 0, NULL, &context);
		assert(result == MA_SUCCESS && "failed to initialize context");
		result = ma_context_get_devices(&context, &pPlaybackDeviceInfos, &playbackDeviceCount, NULL, NULL);
		assert(result == MA_SUCCESS && "failed to get devices");



		engines.resize(playbackDeviceCount + 1);
		devices.resize(engines.size());
		deviceNames.reserve(engines.size());

		InitEngines(pPlaybackDeviceInfos, playbackDeviceCount);

		effects.resize(engines.size());
		music.resize(engines.size());
		voices.resize(engines.size());

		LoadHowlingWind();
		InitVolume();
		if ((volumes[(uint8_t)SoundVolume::master] > 0.f) && (volumes[(uint8_t)SoundVolume::music] > 0.f)) {
			currentSong = howling_wind_index;
			ma_sound_start(&hwSound);
		}
	}
	SoundEngine::~SoundEngine() {
		for (auto& effectEngine : effects) {
			for (auto iter2 = effectEngine.begin(); iter2 != effectEngine.end(); iter2++) {
				ma_sound_uninit(&iter2->second);
			}
		}

		for (auto& engine : engines) {
			ma_engine_uninit(&engine);
		}
		for (auto& device : devices) {
			ma_device_uninit(&device);
		}
		ma_context_uninit(&context);

		ma_resource_manager_uninit(&resourceManager);
	}

	void SoundEngine::SwitchDevices(uint16_t deviceIterator) {
		if ((deviceIterator < 0) || (deviceIterator > devices.size())) {
#if EWE_DEBUG
			Log::Warning("failed to switch devices - %d:%zu \n", deviceIterator, devices.size());
#endif
			return;
		}
		if (deviceIterator == selectedEngine) {
			Log::Debug("trying to switch sound device to the currently active \n");
			return;
		}



		Log::Debug("switching device - from:to - %d:%d \n", selectedEngine, deviceIterator);
		ma_result result = ma_engine_start(&engines[deviceIterator]);
		if (result != MA_SUCCESS) {
			Log::Warning("failed to start engine on switch : %d \n", deviceIterator);
		}
		else {
			ma_engine_stop(&engines[selectedEngine]);
			for (auto iter = effects.at(selectedEngine).begin(); iter != effects.at(selectedEngine).end(); iter++) {
				ma_sound_stop(&iter->second);
				ma_sound_uninit(&iter->second);
			}
			for (auto iter = music.at(selectedEngine).begin(); iter != music.at(selectedEngine).end(); iter++) {
				if (iter->first == currentSong) {
					ma_sound_get_cursor_in_pcm_frames(&iter->second, &currentPCMFrames);
					Log::Debug("pcm cursor : %llu \n", currentPCMFrames);
				}
				ma_sound_stop(&iter->second);
				ma_sound_uninit(&iter->second);
			}
			effects.at(selectedEngine).clear();
			music.at(selectedEngine).clear();
			selectedEngine = deviceIterator;
			ReloadSounds();
		}
			
	}

	void SoundEngine::PlayMusic(uint16_t whichSong, bool repeat) {

 		if(selectedEngine > music.size()){
			Log::Warning("selected engine is out of range \n");
			return;
		}
		if((whichSong > music.at(selectedEngine).size()) && (whichSong != howling_wind_index)){
			Log::Warning("selected effect is out of range \n");
			return;
		}

		currentSong = whichSong;
		ma_result result;
		if(whichSong == howling_wind_index){
			ma_sound_set_looping(&hwSound, repeat);
			result = ma_sound_start(&hwSound);
		}
		else{
			ma_sound_set_looping(&music.at(selectedEngine).at(whichSong), repeat);
			result = ma_sound_start(&music.at(selectedEngine).at(whichSong));
		}
		if (result != MA_SUCCESS) {
			Log::Warning("Failed to play music %d", whichSong);
		}
	}
	void SoundEngine::StopMusic() {
		Log::Debug("stop the music pls \n");
		if (currentSong == howling_wind_index) {
			ma_sound_stop(&hwSound);
			return;
		}


		if (music.at(selectedEngine).find(currentSong) != music.at(selectedEngine).end()) {
			ma_sound_stop(&music.at(selectedEngine).at(currentSong));
		}
		else {
			Log::Warning("attempting to stop music, failed to find it \n");
		}
	}
	void SoundEngine::PlayEffect(uint16_t whichEffect, bool looping) {

 		if(selectedEngine > effects.size()){
			Log::Warning("selected engine is out of range \n");
			return;
		}
		if(whichEffect > effects.at(selectedEngine).size()){
			Log::Warning("selected effect is out of range \n");
			return;
		}

		if ((volumes[(uint8_t)SoundVolume::master] <= 0.f) && (volumes[(uint8_t)SoundVolume::effect] <= 0.f)) {
			Log::Debug("volume is 0\n");
			return;
		}

#if EWE_DEBUG
		//Log::Debug("selectedEngine : %d:%.2f - volume of sound : %.2f \n", selectedEngine, ma_engine_get_volume(&engines.at(selectedEngine)), ma_sound_get_volume(&effects.at(selectedEngine).at(whichEffect)));
#endif
		//ma_sound
		ma_result result = ma_sound_start(&effects.at(selectedEngine).at(whichEffect));
		ma_sound_set_looping(&effects.at(selectedEngine).at(whichEffect), looping);
		if (result != MA_SUCCESS) {
			Log::Warning("Failed to start sound \"%d\"", whichEffect);
			return;
		}
	}
	void SoundEngine::StopEffect(uint16_t whichEffect) {
		Log::Debug("stop effect : %d\n", ma_sound_stop(&effects.at(selectedEngine).at(whichEffect)));
		ma_sound_seek_to_pcm_frame(&effects.at(selectedEngine).at(whichEffect), 0);
	}

	void SoundEngine::RestartEffect(uint16_t whichEffect, bool looping)
	{
		ma_sound* soundAddr = &effects.at(selectedEngine).at(whichEffect);
		ma_sound_stop(soundAddr);
		ma_sound_seek_to_pcm_frame(soundAddr, 0);
		ma_result result = ma_sound_start(soundAddr);
		ma_sound_set_looping(soundAddr, looping);
		if (result != MA_SUCCESS) {
			Log::Warning("Failed to start sound \"%d\"", whichEffect);
			return;
		}
	}

	void SoundEngine::LoadHowlingWind() {
		bin2cpp::File const* hWind = &bin2cpp::getHowlingWindFile();
		
		ma_decoder_config decoderConfig = ma_decoder_config_init(ma_format_f32, 2, 48000);
		decoderConfig.pCustomBackendUserData = NULL;
		decoderConfig.ppCustomBackendVTables = NULL;
		decoderConfig.customBackendCount = 0;
		decoderConfig.encodingFormat = ma_encoding_format_mp3;
		//ma_decoder_init();

		ma_result result = ma_decoder_init_memory(hWind->getBuffer(), hWind->getSize(), NULL, &hwDecoder);
		
		if (result != MA_SUCCESS) {
			Log::Warning("decoder init from memroy failed \n");
		}
		result = ma_sound_init_from_data_source(&engines[selectedEngine], &hwDecoder, MA_SOUND_FLAG_STREAM, NULL, &hwSound);

		if (result != MA_SUCCESS) {
			Log::Warning("init from data source failed : HOWLING WIND \n");
		}
	}
	void SoundEngine::InitEngines(ma_device_info* deviceInfos, uint32_t deviceCount) {

		ma_engine_config engineConfig = ma_engine_config_init();
		engineConfig.pResourceManager = &resourceManager;
		engineConfig.noAutoStart = MA_TRUE;    /* Don't start the engine by default - we'll do that manually below. */

		bool foundDesiredDevice = false;
		deviceNames.emplace_back("default");
		for(uint32_t i = 0; i < deviceCount; i++){
			Log::Debug("device name[%d] : %s\n", i, pPlaybackDeviceInfos[i].name);
			deviceNames.emplace_back(pPlaybackDeviceInfos[i].name);
		}

		for (int32_t i = 0; (i < deviceCount + 1) && (deviceCount > 0); i++) {
			ma_device_config deviceConfig;

			deviceConfig = ma_device_config_init(ma_device_type_playback);
			if (i == 0) {
				deviceConfig.playback.pDeviceID = NULL;
			}
			else {
				deviceConfig.playback.pDeviceID = &pPlaybackDeviceInfos[i - 1].id;
			}
			deviceConfig.playback.format = resourceManager.config.decodedFormat;
			deviceConfig.playback.channels = 0;
			deviceConfig.sampleRate = resourceManager.config.decodedSampleRate;
			deviceConfig.dataCallback = ma_data_callback;
			deviceConfig.pUserData = &engines[i];

			ma_result result = ma_device_init(&context, &deviceConfig, &devices[i]);
			if (result != MA_SUCCESS) {
				Log::Warning("failed to intialize sound device : %s\n", deviceNames[i].c_str());

				devices.erase(devices.begin() + i);
				engines.erase(engines.begin() + i);
				deviceNames.erase(deviceNames.begin() + i);
				deviceCount--;
				i--;
				continue;
			}

			auto& deviceName = deviceNames[i];
			if (i == 0) {
				// Now that we have the device we can initialize the engine. The device is passed into the engine's config
				ma_engine_config local_engineConfig;
				local_engineConfig = ma_engine_config_init();
				local_engineConfig.pDevice = &devices[i];
				local_engineConfig.pResourceManager = &resourceManager;
				local_engineConfig.noAutoStart = MA_TRUE;    /* Don't start the engine by default - we'll do that manually below. */
			
				result = ma_engine_init(&local_engineConfig, &engines[0]);
				if (result != MA_SUCCESS) {
					Log::Warning("Failed to init engine for %s\n", deviceName.c_str());
					ma_device_uninit(&devices[0]);
					//throw std exception or just cancel the swap?
				}
				else if (EngineSettings::settingsData.selectedDevice == deviceName) {
					foundDesiredDevice = true;
					Log::Debug("starting default device, matched with settings \n");
					result = ma_engine_start(&engines[0]);
					if (result != MA_SUCCESS) {
						Log::Debug("Failed to start engine for DEFAULT \n");
						ma_engine_uninit(&engines[0]);
						ma_device_uninit(&devices[0]);
						//throw std exception or just cancel the swap?
					}
					else {
						selectedEngine = 0;
					}
				}
			
			}
			else {
				// Now that we have the device we can initialize the engine. The device is passed into the engine's config
				ma_engine_config local_engineConfig;
				local_engineConfig = ma_engine_config_init();
				local_engineConfig.pDevice = &devices[i];
				local_engineConfig.pResourceManager = &resourceManager;
				local_engineConfig.noAutoStart = MA_TRUE;    /* Don't start the engine by default - we'll do that manually below. */

				result = ma_engine_init(&engineConfig, &engines[i]);
				if (result != MA_SUCCESS) {
					Log::Warning("Failed to init engine for %s\n", deviceName.c_str());
					ma_device_uninit(&devices[0]);
					//throw std exception or just cancel the swap?
				}
				else if (deviceName.find(EngineSettings::settingsData.selectedDevice) != deviceName.npos) {
					foundDesiredDevice = true;
					result = ma_engine_start(&engines[i]);
					Log::Debug("starting device from settings : %s \n", deviceName.c_str());
					if (result != MA_SUCCESS) {
						Log::Warning("Failed to start engine for %s\n", deviceName.c_str());
						ma_engine_uninit(&engines[i]);
						ma_device_uninit(&devices[i]);
						//throw std exception or just cancel the swap?
					}
					else {
						selectedEngine = i;
					}
				}
			}
			Log::Debug("device name - %d: %s\n", i, deviceNames[i].c_str());
		}
		if (!foundDesiredDevice) {
			Log::Debug("failed to find desired device in settings, starting default \n");
			if (engines.size() > 0) {
				ma_result result = ma_engine_init(&engineConfig, &engines[0]);
				if (result != MA_SUCCESS) {
					Log::Warning("Failed to init engine for DEFAULT \n");
					ma_device_uninit(&devices[0]);
					//throw std exception or just cancel the swap?
				}
				else {
					Log::Debug("starting default engine \n");
					result = ma_engine_start(&engines[0]);
					if (result != MA_SUCCESS) {
						Log::Warning("Failed to start engine for DEFAULT \n");
						ma_engine_uninit(&engines[0]);
						ma_device_uninit(&devices[0]);
						//throw std exception or just cancel the swap?
					}
					else {
						selectedEngine = 0;
					}
				}
			}
		}
		Log::Debug("after init engines, selected device : %d \n", selectedEngine);
	}

	void SoundEngine::LoadSoundMap(std::unordered_map<uint16_t, std::filesystem::path>& loadSounds, SoundVolume soundType) {
		if (selectedEngine > engines.size()) {
			Log::Error("selected engine isinvalid when loading effects \n");
			return;
		}

		std::unordered_map<uint16_t, std::filesystem::path>* locations{nullptr};
		std::unordered_map<uint16_t, ma_sound>* sounds{nullptr};
		switch (soundType) {
			case SoundVolume::effect: {
				locations = &effectLocations;
				sounds = &effects.at(selectedEngine);
				break;
			}
			case SoundVolume::music: {
				locations = &musicLocations;
				sounds = &music.at(selectedEngine);
				break;
			}
			case SoundVolume::voice: {
				locations = &voiceLocations;
				sounds = &voices.at(selectedEngine);
				break;
			}
			default: {
				Log::Warning("loading an unsupported soudn type? : %s \n", Reflect::Enum::ToString(soundType).data());
				return;
				break;
			}
		}

		ma_result result;
		for (auto& sound : loadSounds) {
			if (locations->find(sound.first) == locations->end()) {
				if (!std::filesystem::exists(sound.second))
				{
					Log::Warning("failed to find sound file location : %s\n", sound.second.c_str());
				}
				locations->emplace(sound);
			}
			else {
				//updates the vlaue?? should probably just be ignored
				Log::Debug("overwriting a saved effect location : %d \n", sound.first);
				Log::Debug("overwritign wont take effect until the device is swapped. i could support this but not doing it currently \n");
				locations->at(sound.first) = sound.second;
			}
		}

		for (auto& soundPath : *locations) {
			if (sounds->find(soundPath.first) == sounds->end()) {
				sounds->emplace(soundPath.first, ma_sound{});
				switch (soundType) {
					case SoundVolume::effect: {
						if (std::filesystem::exists(soundPath.second)) {
							result = ma_sound_init_from_file(&engines[selectedEngine], soundPath.second.c_str(),
								MA_RESOURCE_MANAGER_DATA_SOURCE_FLAG_DECODE | MA_RESOURCE_MANAGER_DATA_SOURCE_FLAG_ASYNC | MA_RESOURCE_MANAGER_DATA_SOURCE_FLAG_STREAM,
								NULL, NULL, &sounds->at(soundPath.first));
							ma_sound_set_volume(&sounds->at(soundPath.first), volumes[(uint8_t)SoundVolume::effect]);

							if (result != MA_SUCCESS) {
								Log::Warning("Failed to load effect \"%s\"", soundPath.second.c_str());
								throw std::runtime_error("failed to load sound");
							}
						}
						else {
							Log::Warning("effect path doesnt exist \n");
						}
						break;
					}
					case SoundVolume::voice:
					case SoundVolume::music: {

						result = ma_sound_init_from_file(&engines[selectedEngine], soundPath.second.c_str(), MA_SOUND_FLAG_STREAM, NULL, NULL, &sounds->at(soundPath.first));

						ma_sound_set_volume(&sounds->at(soundPath.first), volumes[(uint8_t)SoundVolume::music]);
						if (result != MA_SUCCESS) {
							Log::Warning("Failed to load music or voice \"%s\"", soundPath.second.c_str());
							throw std::runtime_error("failed to load sound");
						}
						break;
					}
				}
			}
			else {
				Log::Debug("trying to emplace a sound into a map key that already has a sounnd. this is being ignored - %d:%d \n", soundPath.first, (int)soundType);
			}
		}

	}

	int16_t SoundEngine::AddMusicToBack(std::string const& musicLocation)
	{
		for (auto& loc : musicLocations)
		{
			if (loc.second == musicLocation)
			{
				return loc.first;
			}
		}
		int16_t ret = musicLocations.size();
		musicLocations.try_emplace(ret, musicLocation);

		ma_result result;
		music[selectedEngine].emplace(ret, ma_sound{});
		result = ma_sound_init_from_file(&engines[selectedEngine], musicLocation.c_str(), MA_SOUND_FLAG_STREAM, NULL, NULL, &music[selectedEngine].at(ret));

		ma_sound_set_volume(&music[selectedEngine].at(ret), volumes[(uint8_t)SoundVolume::music]);
		if (result != MA_SUCCESS) {
			Log::Warning("Failed to load music or voice \"%s\"", musicLocation.c_str());
			throw std::runtime_error("failed to load sound");
			return -1;
		}
		Log::Debug("successfully added music to back\n");
		return ret;
	}


	void SoundEngine::ReloadSounds() {
		ma_result result;
		//effects
		Log::Debug("before reloading effects \n");
		for (auto& effectPath : effectLocations) {
			effects.at(selectedEngine).emplace(effectPath.first, ma_sound{});
			result = ma_sound_init_from_file(&engines[selectedEngine], effectPath.second.c_str(),
				MA_RESOURCE_MANAGER_DATA_SOURCE_FLAG_DECODE | MA_RESOURCE_MANAGER_DATA_SOURCE_FLAG_ASYNC | MA_RESOURCE_MANAGER_DATA_SOURCE_FLAG_STREAM,
				NULL, NULL, &effects.at(selectedEngine).at(effectPath.first));

			if (result != MA_SUCCESS) {
				Log::Error("Failed to load effect \"%s\"", effectPath.second.c_str());
				throw std::runtime_error("failed to load sound");
			}
		}

		Log::Debug("before reloading music \n");
		for (auto& musicPath : musicLocations) {
			music.at(selectedEngine).emplace(musicPath.first, ma_sound{});
			Log::Debug("imemdiately befroe : %s \n", musicPath.second.c_str());
			result = ma_sound_init_from_file(&engines[selectedEngine], musicPath.second.c_str(), MA_SOUND_FLAG_STREAM, NULL, NULL, &music.at(selectedEngine).at(musicPath.first));
			if (currentSong == musicPath.first) {
				ma_sound_seek_to_pcm_frame(&music.at(selectedEngine).at(musicPath.first), currentPCMFrames);
				if (!ma_sound_at_end(&music.at(selectedEngine).at(musicPath.first))) {
					ma_sound_set_volume(&music.at(selectedEngine).at(musicPath.first), volumes[(uint8_t)SoundVolume::music]);
					ma_sound_start(&music.at(selectedEngine).at(musicPath.first));
				}
			}

			if (result != MA_SUCCESS) {
				Log::Warning("Failed to load music \"%s\"", musicPath.second.c_str());
				throw std::runtime_error("failed to load sound");
			}
		}
		InitVolume();
		Log::Debug("after reloading music \n");
	}
	void SoundEngine::SetVolume(SoundVolume whichVolume, uint8_t value) {
		auto& volume = this->volumes[(uint8_t)whichVolume];
		volume = static_cast<float>(value) / 100.f;

		switch (whichVolume) {
		case SoundVolume::master: {
			for (auto& engine : engines) {
				ma_engine_set_volume(&engine, volume);
			}
			break;
		}
		case SoundVolume::effect: {
			for (auto& sound : effects.at(selectedEngine)) {
				ma_sound_set_volume(&sound.second, volume);
			}
			break;
		}
		case SoundVolume::music: {
			for (auto& sound : music.at(selectedEngine)) {
				ma_sound_set_volume(&sound.second, volume);
			}
			ma_sound_set_volume(&hwSound, volume);
			break;
		}
		case SoundVolume::voice: {
			for (auto& sound : voices.at(selectedEngine)) {
				ma_sound_set_volume(&sound.second, volume);
			}
			break;
		}
		}
	}
}