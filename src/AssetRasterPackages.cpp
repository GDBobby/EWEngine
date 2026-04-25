#include "EWEngine/Assets/BaseInstPackages.h"
#include "EWEngine/Assets/RasterPackages.h"
#include "EWEngine/Global.h"


#include "EWEngine/Imgui/DragDrop.h"
#include "EightWinds/Command/ObjectInstructionPackage.h"
#include "EightWinds/Pipeline/TaskRasterConfig.h"
#include <fstream>

namespace EWE{
namespace Asset{

    static constexpr uint64_t current_file_version = 0;

    template<>
    bool WriteAssetToFile(RasterPackage const& rt, std::filesystem::path const& root_directory, std::filesystem::path const& path){
        //if it has the old record its invalid
        //if it doesnt have an instruction package, its special execution, aka hand coded, cant be written

        std::ofstream outFile{root_directory / path, std::ios::binary};
        if(!outFile.is_open()){
            outFile.open(root_directory / path);
            if(!outFile.is_open()){
                return false;
            }
        }

        std::size_t temp_buffer = current_file_version;
        outFile.write(reinterpret_cast<char*>(&temp_buffer), sizeof(temp_buffer));
        if(temp_buffer != current_file_version){
            return false;
        }

        temp_buffer = rt.objectPackages.size();
        outFile.write(reinterpret_cast<char*>(&temp_buffer), sizeof(temp_buffer));

        for(auto* pkg : rt.objectPackages){
            AssetHash hash_buffer = GetHash(*pkg);
            WriteAssetToFile(static_cast<Command::InstructionPackage&>(*pkg), root_directory, pkg->name);
            outFile.write(reinterpret_cast<char*>(&hash_buffer), sizeof(AssetHash));
        }
        return true;
    }

    template<>
    bool LoadAssetFromFile(RasterPackage* ptr_to_raw_mem, std::filesystem::path const& root_directory, std::filesystem::path const& name){
        std::ifstream inFile{root_directory / name, std::ios::binary};
        if(!inFile.is_open()){
            return false;
        }

        std::size_t temp_buffer;
        inFile.read(reinterpret_cast<char*>(&temp_buffer), sizeof(temp_buffer));
        EWE_ASSERT(temp_buffer == current_file_version, "needs support otherwise");
        
        TaskRasterConfig tempConfig;
        auto& ret = *std::construct_at(ptr_to_raw_mem, name.string(), *Global::logicalDevice, Global::stcManager->renderQueue, tempConfig, nullptr);

        inFile.read(reinterpret_cast<char*>(&temp_buffer), sizeof(temp_buffer));

        ret.objectPackages.reserve(temp_buffer);
        for(std::size_t i = 0; i < temp_buffer; i++){
            AssetHash hash_buffer;
            inFile.read(reinterpret_cast<char*>(&hash_buffer), sizeof(AssetHash));
            //i need to make sure this is also written to file, or it will become invalid
            ret.objectPackages.push_back(Global::assetManager->objPkg.Get(hash_buffer));
        }

        inFile.close();
        return true;
    }

} //namepsace Asset
} //namespace EWE