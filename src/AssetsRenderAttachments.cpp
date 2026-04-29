#include "EWEngine/Assets/RenderAttachments.h"

namespace EWE{
namespace Assets{

#define cast_write(data) reinterpret_cast<const char*>(&data), sizeof(data)
	bool WriteAttachmentInfoToFile(std::ofstream& outFile, AttachmentSetInfo const& info){
		outFile.write(cast_write(info.width));
		outFile.write(cast_write(info.height));
		outFile.write(cast_write(info.renderingFlags));
		outFile.write(cast_write(info.depth));
		uint8_t temp_size = static_cast<uint8_t>(colors.Size());
		outFile.write(cast_write(temp_size));
		outFile.write(reinterpret_cast<const char*>(colors.Data()), sizeof(AttachmentInfo) * temp_size);
	}
#undef cast_write

	bool RenderAttachments::WriteToFile(RenderAttachments const& attachments, std::filesystem::path const& path){
		std::ofstream outFile{path, std::ios::binary};
		if(!outFile.is_open()){
			return false;
		}

		WriteAttachmentInfoToFile(outFile, attachments.setInfo);

		outFile.close();
		return true;
	}
	bool RenderAttachments::ReadFromFile(RenderAttachments* construction_ptr, std::filesystem::path const& path){

	}
} //namespace Assets
} //namespace EWE