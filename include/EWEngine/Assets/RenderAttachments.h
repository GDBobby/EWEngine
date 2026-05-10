#pragma once

#include "EWEngine/Assets/Base.h"
#include "EightWinds/Backend/RenderInfo.h"

namespace EWE{
namespace Asset{
    
    void GenerateViewPair(FullRenderInfo& fri, int8_t index);

    void WriteAttachmentInfoToFile(std::ofstream& outFile, AttachmentSetInfo const& info);
	void ReadAttachmentInfoFromFile(std::ifstream& inFile, AttachmentSetInfo& info);

    template<>
    bool WriteAssetToFile(FullRenderInfo const& fri, std::filesystem::path const& root_directory, std::filesystem::path const& path);
    template<>
    bool LoadAssetFromFile(FullRenderInfo* ptr_to_raw_mem, std::filesystem::path const& root_directory, std::filesystem::path const& name);


}//namespace Asset
}//namepsace EWE