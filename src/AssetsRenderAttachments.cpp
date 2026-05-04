#include "EWEngine/Assets/RenderAttachments.h"

namespace EWE{
namespace Asset{

#define cast_write(data) reinterpret_cast<const char*>(&data), sizeof(data)
	void WriteAttachmentInfoToFile(std::ofstream& outFile, AttachmentSetInfo const& info){
		outFile.write(cast_write(info.width));
		outFile.write(cast_write(info.height));
		outFile.write(cast_write(info.renderingFlags));

		outFile.write(cast_write(info.using_depth));
		outFile.write(cast_write(info.depth));
		uint8_t temp_size = static_cast<uint8_t>(info.colors.Size());
		outFile.write(cast_write(temp_size));
		outFile.write(reinterpret_cast<const char*>(info.colors.Data()), sizeof(AttachmentInfo) * temp_size);
	}
#undef cast_write

	void ReadAttachmentInfoFromFile(std::ifstream& inFile, AttachmentSetInfo& info){
#define cast_read(data) inFile.read(reinterpret_cast<char*>(&data), sizeof(data))

		cast_read(info.width);
		cast_read(info.height);
		cast_read(info.renderingFlags);

		cast_read(info.using_depth);
		cast_read(info.depth);
		uint8_t temp_size;
		cast_read(temp_size);
		info.colors.ClearAndResize(temp_size);
		inFile.read(reinterpret_cast<char*>(info.colors.Data()), sizeof(AttachmentInfo) * temp_size);

#undef cast_read
	}

	template<>
	bool WriteAssetToFile(FullRenderInfo const& fri, std::filesystem::path const& root_directory, std::filesystem::path const& path){
		auto const full_path = root_directory / path;
		std::ofstream outFile{full_path, std::ios::binary};
		if(!outFile.is_open()){
			Logger::Print("failed to open file : %s / %s\n", root_directory.string().c_str(), path.string().c_str());
			return false;
		}

		WriteAttachmentInfoToFile(outFile, fri.full.setInfo);

		outFile.close();
		return true;
	}
	template<>
	bool LoadAssetFromFile(FullRenderInfo* ptr_to_raw_mem, std::filesystem::path const& root_directory, std::filesystem::path const& path){
		auto const full_path = root_directory / path;
		std::ifstream inFile{full_path, std::ios::binary};
		if(!inFile.is_open()){
			auto const full_path_string = full_path.string();
			Logger::Print("failed initial open attempt on file : %s\n", full_path_string.c_str());
			if(!std::filesystem::exists(full_path)){
				Logger::Print("attempting to open file that doesn't exist : %s\n", full_path_string.c_str());
				return false;
			}
			inFile.open(full_path, std::ios::binary);
			if(!inFile.is_open()){
				Logger::Print("failed explicit open : %s\n", full_path_string.c_str());
				return false;
			}
		}

		AttachmentSetInfo setInfo;
		ReadAttachmentInfoFromFile(inFile, setInfo);

		std::construct_at(ptr_to_raw_mem, path.string(), *Global::logicalDevice, Global::stcManager->renderQueue, setInfo);

		inFile.close();
		return true;
	}
} //namespace 
} //namespace EWE