#include "EWEngine/EngineSettings.h"

#include <include/rapidjson/document.h>
#include <include/rapidjson/prettywriter.h>// for stringify JSON
#include <include/rapidjson/error/en.h>

namespace EWE{

namespace EngineSettings {
	SettingsData settingsData;
	SettingsData tempSettings;

	std::vector<ScreenDimensions> ScreenDimensions::commonDimensions{
		{7680, 4320},
		{5120, 2160},
		{5120, 1440},
		{3840, 2160},
		{3840, 1080},
		{3440, 1440},
		{3440, 1440},
		{2560, 1440},
		{2560, 1080},
		{1920, 1080},
		{1600, 900},
		{1536, 864},
		{1440, 900},
		{1366, 768},
		{1280, 1024},
		{1280, 960},
		{1280, 720},
		{1024, 768},
		{800, 600},
		{800, 450},
		{640, 480},
		{640, 360}
	};
	void ScreenDimensions::FixCommonDimensionsToScreenSize(int width, int height) {
		for (uint16_t i = 0; i < commonDimensions.size(); i++) {
			if (width < commonDimensions[i].width || height < commonDimensions[i].height) {
				commonDimensions.erase(commonDimensions.begin() + i);
				i--;
			}
		}
	}
	ScreenDimensionsFloat ScreenDimensions::ConvertToFloat() {
		ScreenDimensionsFloat ret{};
		ret.width = static_cast<float>(width);
		ret.height = static_cast<float>(height);
		return ret;
	}

	std::string ScreenDimensions::GetString() const {
		return std::to_string(width) + " x " + std::to_string(height);
	}


bool ReadFromJsonFile(rapidjson::Document& document) {
	if (!document.HasMember("version")) {
		return false;
	}
	if (!document["version"].IsInt()) {
		printf("version not int \n");
		return false;
	}
	else if (document["version"].GetInt() != CURRENT_SETTINGS_VERSION) {
		printf("icnorrect version \n");
		return false;
	}

	if (!document["windowMode"].IsInt()) {
		printf("wm not int \n");
		return false;
	}
	if (!document["screenDimensionsX"].IsInt()) {
		printf("SD not int \n");
		return false;
	}
	if (!document["screenDimensionsY"].IsInt()) {
		printf("SD not int \n");
		return false;
	}
	if (!document["masterVolume"].IsInt()) {
		printf("MaV not int \n");
		return false;
	}
	if (!document["effectsVolume"].IsInt()) {
		printf("EV not int \n");
		return false;
	}
	if (!document["musicVolume"].IsInt()) {
		printf("MuV not int \n");
		return false;
	}
	if (!document["voiceVolume"].IsInt()) {
		printf("VV not int \n");
		return false;
	}
	if (!document["selectedDevice"].IsString()) {
		printf("device not string \n");
		return false;
	}
	if (!document["FPS"].IsUint()) {
		printf("FPS not unsigned short \n");
		return false;
	}
	if (!document["pointLights"].IsBool()) {
		printf("PL not bool \n");
		return false;
	}
	if (!document["renderInfo"].IsBool()) {
		printf("render not bool \n");
		return false;
	}

	EngineSettings::settingsData.versionKey = CURRENT_SETTINGS_VERSION;

	int valueBuffer = document["windowMode"].GetInt();
	if (valueBuffer != 0 && valueBuffer != 1) {
		EngineSettings::settingsData.windowMode = EngineSettings::WindowMode::borderless;
	}
	else {
		EngineSettings::settingsData.windowMode = static_cast<EngineSettings::WindowMode>(valueBuffer);
	}

	valueBuffer = document["screenDimensionsX"].GetInt();
	EngineSettings::settingsData.screenDimensions.width = valueBuffer;
	valueBuffer = document["screenDimensionsY"].GetInt();
	EngineSettings::settingsData.screenDimensions.height = valueBuffer;
	
	EngineSettings::settingsData.masterVolume = document["masterVolume"].GetUint();

	EngineSettings::settingsData.effectsVolume = document["effectsVolume"].GetUint();
	EngineSettings::settingsData.musicVolume = document["musicVolume"].GetUint();
	EngineSettings::settingsData.voiceVolume = document["voiceVolume"].GetUint();

	//holma
	if (EngineSettings::settingsData.masterVolume < 0 || EngineSettings::settingsData.masterVolume > 100) {
		EngineSettings::settingsData.masterVolume = 50;
	}
	if (EngineSettings::settingsData.effectsVolume < 0 || EngineSettings::settingsData.effectsVolume > 100) {
		EngineSettings::settingsData.effectsVolume = 50;
	}
	if (EngineSettings::settingsData.musicVolume < 0 || EngineSettings::settingsData.musicVolume > 100) {
		EngineSettings::settingsData.musicVolume = 50;
	}
	if (EngineSettings::settingsData.voiceVolume < 0 || EngineSettings::settingsData.voiceVolume > 100) {
		EngineSettings::settingsData.voiceVolume = 50;
	}

	EngineSettings::settingsData.selectedDevice = document["selectedDevice"].GetString();
	EngineSettings::settingsData.FPS = document["FPS"].GetUint();
	if (EngineSettings::settingsData.FPS < 0) {
		EngineSettings::settingsData.FPS = 0;
	}

	EngineSettings::settingsData.pointLights = document["pointLights"].GetBool();
	EngineSettings::settingsData.renderInfo = document["renderInfo"].GetBool();

	EngineSettings::tempSettings = EngineSettings::settingsData;

	return true;
}


void EngineSettings::SettingsData::SetVolume(int8_t whichVolume, uint8_t value) {
	//ma_device_set_SoundVolume::master(&device, volume[whichVolume]);
	//printf("setting volume %d : %.2f \n", whichVolume, value);

	if (whichVolume == (uint8_t)SoundVolume::master) {
		masterVolume = value;
	}
	else if (whichVolume == (uint8_t)SoundVolume::effect) {
		effectsVolume = value;
	}
	else if (whichVolume == (uint8_t)SoundVolume::music) {
		musicVolume = value;
	}
	else if (whichVolume == (uint8_t)SoundVolume::voice) {
		voiceVolume = value;
	}
}
const uint8_t& EngineSettings::SettingsData::getVolume(int8_t whichVolume) {
	if (whichVolume == (uint8_t)SoundVolume::master) {
		return masterVolume;
	}
	else if (whichVolume == (uint8_t)SoundVolume::effect) {
		return effectsVolume;
	}
	else if (whichVolume == (uint8_t)SoundVolume::music) {
		return musicVolume;
	}
	else if (whichVolume == (uint8_t)SoundVolume::voice) {
		return voiceVolume;
	}
	Log::Error("invalid volume type");
	return masterVolume;
}




void InitializeSettings() {
	rapidjson::Document document;

	if (!std::filesystem::exists(SETTINGS_LOCATION)) {
		//no file exist
		std::ofstream tempFile{ SETTINGS_LOCATION };
		GenerateDefaultFile();
		tempFile.close();
	}
	else {
		std::ifstream inFile;
		inFile.open(SETTINGS_LOCATION, std::ios::binary);

		// get length of file:
		inFile.seekg(0, std::ios::end);
		size_t length = inFile.tellg();
		inFile.seekg(0, std::ios::beg);

		// allocate memory:
		char* buffer = new char[length + 1];

		// read data as a block:
		inFile.read(buffer, length);
		buffer[length] = '\0';

		document.Parse(buffer);
		if (document.HasParseError() || !document.IsObject()) {
			printf("error parsing settings at : %s \n", SETTINGS_LOCATION);
			printf("error at %d : %s \n", static_cast<int32_t>(document.GetErrorOffset()), rapidjson::GetParseError_En(document.GetParseError()));
			//assert(false);
			GenerateDefaultFile();
		}
		else {
			if (!ReadFromJsonFile(document)) {
				//failed to parse correctly
				printf("failed to read settings correctly \n");
				//assert(false);
				GenerateDefaultFile();
			}
		}
		// delete temporary buffer
		delete[] buffer;

		// close filestream
		inFile.close();
	}
}

void GenerateDefaultFile() {
	SettingsData settingsDefault{};
	settingsData = settingsDefault;
	tempSettings = settingsDefault;
	printf("generating default file \n");
	SaveToJsonFile();
}


void SaveToJsonFile() {
	rapidjson::StringBuffer sb;
	rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
	writer.StartObject();
	writer.Key("version");
	writer.Int(CURRENT_SETTINGS_VERSION);
	writer.Key("windowMode");
	writer.Int(static_cast<int>(settingsData.windowMode));
	writer.Key("screenDimensionsX");
	writer.Int(settingsData.screenDimensions.width);
	writer.Key("screenDimensionsY");
	writer.Int(settingsData.screenDimensions.height);
	writer.Key("masterVolume");
	writer.Uint(settingsData.masterVolume);
	writer.Key("effectsVolume");
	writer.Uint(settingsData.effectsVolume);
	writer.Key("musicVolume");
	writer.Uint(settingsData.musicVolume);
	writer.Key("voiceVolume");
	writer.Uint(settingsData.voiceVolume);
	writer.Key("selectedDevice");
	writer.String(settingsData.selectedDevice.c_str());
	writer.Key("FPS");
	writer.Uint(settingsData.FPS);
	writer.Key("pointLights");
	writer.Bool(settingsData.pointLights);
	writer.Key("renderInfo");
	writer.Bool(settingsData.renderInfo);
	writer.EndObject();
	std::ofstream file;
	file.open(SETTINGS_LOCATION);
	file << sb.GetString();
	file.close();

}
} //namespace EngineSettings
}//namespace EWE
