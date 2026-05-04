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


	void WriteConfig(TaskRasterConfig const& config, std::ofstream& outFile){
#define cast_write(data) reinterpret_cast<char const*>(&data), sizeof(data)
		outFile.write(cast_write(config.viewportCount));
		outFile.write(cast_write(config.scissorCount));
		outFile.write(cast_write(config.rastSamples));
		outFile.write(cast_write(config.enable_sampleShading));
		outFile.write(cast_write(config.minSampleShading));
		outFile.write(cast_write(config.alphaToCoverageEnable));
		outFile.write(cast_write(config.alphaToOneEnable));
		outFile.write(cast_write(config.depthStencilInfo));

        AssetHash fri_hash = GetHash(*config.renderInfo);
        outFile.write(cast_write(fri_hash));

		uint8_t size_buffer = config.dynamicState.size();
		outFile.write(cast_write(size_buffer));
		if(size_buffer > 0){
			outFile.write(reinterpret_cast<const char*>(config.dynamicState.data()), sizeof(VkDynamicState) * size_buffer);
		}
#undef cast_write
	}

	void ReadConfig(TaskRasterConfig& config, std::ifstream& inFile){
#define cast_read(data) inFile.read(reinterpret_cast<char*>(&data), sizeof(data))

		cast_read(config.viewportCount);
		cast_read(config.scissorCount);
		cast_read(config.rastSamples);
		cast_read(config.enable_sampleShading);
		cast_read(config.minSampleShading);
		cast_read(config.alphaToCoverageEnable);
		cast_read(config.alphaToOneEnable);
		cast_read(config.depthStencilInfo);

        AssetHash fri_hash;
        cast_read(fri_hash);
        config.renderInfo = Global::assetManager->attachment_info.Get(fri_hash);
		
		uint8_t size_buffer = static_cast<uint8_t>(config.dynamicState.size());
		cast_read(size_buffer);
        config.dynamicState.resize(size_buffer);
		if(size_buffer > 0){
			inFile.read(reinterpret_cast<char*>(config.dynamicState.data()), sizeof(VkDynamicState) * size_buffer);
		}
#undef cast_read
	}



    template<>
    bool WriteAssetToFile(RasterPackage const& rt, std::filesystem::path const& root_directory, std::filesystem::path const& path){
        //if it has the old record its invalid
        //if it doesnt have an instruction package, its special execution, aka hand coded, cant be written

        auto const full_save_path = root_directory / path;
        Logger::Print("writing rt to : %s\n", full_save_path.string().c_str());
        std::ofstream outFile{full_save_path, std::ios::binary};
        if(!outFile.is_open()){
            outFile.open(full_save_path);
            if(!outFile.is_open()){
                return false;
            }
        }

        std::size_t temp_buffer = current_file_version;
        outFile.write(reinterpret_cast<char*>(&temp_buffer), sizeof(temp_buffer));
        if(temp_buffer != current_file_version){
            return false;
        }

        WriteConfig(rt.task_config, outFile);

        temp_buffer = rt.objectPackages.size();
        outFile.write(reinterpret_cast<char*>(&temp_buffer), sizeof(temp_buffer));

        for(auto* pkg : rt.objectPackages){
            AssetHash hash_buffer = GetHash(*pkg);
            WriteAssetToFile(static_cast<Command::InstructionPackage&>(*pkg), Global::assetManager->objPkg.files.root_directory, pkg->name);
            outFile.write(reinterpret_cast<char*>(&hash_buffer), sizeof(AssetHash));
        }

        outFile.close();
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
        ReadConfig(tempConfig, inFile);
        auto& ret = *std::construct_at(ptr_to_raw_mem, name.string(), *Global::logicalDevice, Global::stcManager->renderQueue, tempConfig);

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