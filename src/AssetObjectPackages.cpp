#include "EWEngine/Assets/Hash.h"
#include "EWEngine/Assets/ObjectPackages.h"
#include "EWEngine/Assets/BaseInstPackages.h"
#include "EWEngine/Global.h"
#include "EightWinds/Backend/Logger.h"
#include "EightWinds/Command/InstructionPackage.h"
#include "EightWinds/GlobalPushConstant.h"

#include "EightWinds/ObjectRasterConfig.h"
#include "EightWinds/VulkanHeader.h"

#include "EWEngine/Imgui/DragDrop.h"

#include <fstream>

namespace EWE{
namespace Asset{
    template<>
    bool WriteAssetToFile(Command::ObjectPackage const& rt, std::filesystem::path const& root_directory, std::filesystem::path const& path){
        return WriteAssetToFile(static_cast<Command::InstructionPackage const&>(rt), root_directory, path);
    }
    template<>
    bool LoadAssetFromFile(Command::ObjectPackage* ptr_to_raw_mem, std::filesystem::path const& root_directory, std::filesystem::path const& path){
        return LoadAssetFromFile(static_cast<Command::InstructionPackage*>(ptr_to_raw_mem), root_directory, path);
    }
} //namespace Asset
} //namespace EWE