#include "EWEngine/Assets/BaseInstPackages.h"
#include "EWEngine/Assets/RasterPackages.h"
#include "EWEngine/EWEngine.h"


#include "EWEngine/Imgui/DragDrop.h"
#include "EightWinds/Command/ObjectInstructionPackage.h"
#include "EightWinds/Pipeline/TaskRasterConfig.h"
#include <fstream>

namespace EWE{
namespace Asset{

    static constexpr uint64_t current_file_version = 0;

    void WriteAttachmentSetInfo(AttachmentSetInfo const& info, std::ofstream& outFile){
#define cast_write(data) outFile.write(reinterpret_cast<char const*>(&data), sizeof(data))
        cast_write(info.relative_size);
        cast_write(info.width);
        cast_write(info.height);
        cast_write(info.renderingFlags);
        uint8_t color_size = info.colors.Size();
        cast_write(color_size);
        outFile.write(reinterpret_cast<char const*>(info.colors.Data()), sizeof(AttachmentInfo) * color_size);
        cast_write(info.using_depth);
        cast_write(info.depth);
#undef cast_write
    }

    void ReadAttachmentSetInfo(AttachmentSetInfo& info, std::ifstream& inFile){
#define cast_read(data) inFile.read(reinterpret_cast<char*>(&data), sizeof(data))
        cast_read(info.relative_size);
        cast_read(info.width);
        cast_read(info.height);
        cast_read(info.renderingFlags);
        uint8_t color_size;
        cast_read(color_size);
        info.colors.ClearAndResize(color_size);
        inFile.read(reinterpret_cast<char*>(info.colors.Data()), sizeof(AttachmentInfo) * color_size);
        cast_read(info.using_depth);
        cast_read(info.depth);
#undef cast_read
    }

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

        WriteAttachmentSetInfo(config.attachment_info, outFile);

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

        ReadAttachmentSetInfo(config.attachment_info, inFile);
		
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
        Log::Debug("writing rt to : %s\n", full_save_path.string().c_str());
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
            WriteAssetToFile(static_cast<Command::InstructionPackage&>(*pkg), engine->assetManager.objPkg.files.root_directory, pkg->name);
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
        auto& ret = *std::construct_at(ptr_to_raw_mem, name.string(), engine->logicalDevice, engine->renderQueue, tempConfig);

        inFile.read(reinterpret_cast<char*>(&temp_buffer), sizeof(temp_buffer));

        ret.objectPackages.reserve(temp_buffer);
        for(std::size_t i = 0; i < temp_buffer; i++){
            AssetHash hash_buffer;
            inFile.read(reinterpret_cast<char*>(&hash_buffer), sizeof(AssetHash));
            //i need to make sure this is also written to file, or it will become invalid
            ret.objectPackages.push_back(engine->assetManager.objPkg.Get(hash_buffer));
        }

        inFile.close();
        return true;
    }

} //namepsace Asset
} //namespace EWE