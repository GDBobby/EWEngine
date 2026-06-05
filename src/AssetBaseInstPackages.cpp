#include "EWEngine/Assets/Hash.h"
#include "EWEngine/Assets/BaseInstPackages.h"

#include "EWEngine/EWEngine.h"

#include "EightWinds/Logger.h"
#include "EightWinds/Command/InstructionPackage.h"

#include "EightWinds/Command/ObjectInstructionPackage.h"
#include "EightWinds/VulkanHeader.h"

#include "EWEngine/Imgui/DragDrop.h"

#include <fstream>

namespace EWE{
namespace Asset{


    static constexpr std::size_t file_version = 0;
//

    bool WriteToInstPkgFile(Command::ParamPool const& param_pool, void const* payload, Command::InstructionPackage::Type pkg_type, std::filesystem::path const& root_directory, std::filesystem::path const& temp_path){
        
        const std::filesystem::path adjusted_path = root_directory / temp_path;
        
        std::ofstream outFile{adjusted_path, std::ios::binary};
        if(!outFile.is_open()){
            outFile.open(adjusted_path, std::ios::binary);
            if(!outFile.is_open()){
                return false;
            }
        }
        std::size_t temp_buffer = file_version;
        outFile.write(reinterpret_cast<char*>(&temp_buffer), sizeof(std::size_t));
        outFile.write(reinterpret_cast<const char*>(&pkg_type), sizeof(uint8_t));
        temp_buffer = param_pool.instructions.size();
        outFile.write(reinterpret_cast<char*>(&temp_buffer), sizeof(std::size_t));
        
        std::size_t written_param_size = 0;
        //just checking sizes now, and going to write that size to file
        for(std::size_t i = 0; i < param_pool.instructions.size(); i++){
            const auto current_param_size = Inst::GetParamSize(param_pool.instructions[i]);
            written_param_size += current_param_size;
        }
        Log::Debug("current write position, before written param size : %zu\n", outFile.tellp());
        outFile.write(reinterpret_cast<char*>(&written_param_size), sizeof(std::size_t));
        outFile.write(reinterpret_cast<const char*>(param_pool.instructions.data()), temp_buffer * sizeof(Inst::Type));
        Log::Debug("current write position, after writing instructions : %zu\n", outFile.tellp());
    
        //actually writign to file now
        for_each_frame {
            std::size_t param_index = 0;
            for(std::size_t i = 0; i < param_pool.instructions.size(); i++){
                const auto current_param_size = Inst::GetParamSize(param_pool.instructions[i]);
                if(current_param_size > 0){

                    //convert the buffer_address to a buffer hash
                    //convert the texture index to a dii hash
                    if(param_pool.instructions[i] == Inst::Push){
                        Log::Debug("writing push at [%zu] : frame[%u]\n", outFile.tellp(), frame);
                        auto* temp_push = reinterpret_cast<ParamPack<Inst::Push>*>(param_pool.param_data[param_index].data[frame]);
                        outFile.write(reinterpret_cast<char*>(&temp_push->buffer_count), sizeof(temp_push->buffer_count));
                        outFile.write(reinterpret_cast<char*>(&temp_push->texture_count), sizeof(temp_push->texture_count));
                        AssetHash hash_buffer;
                        
                        for(uint8_t j = 0; j < temp_push->buffer_count; j++){
                            //EWE_ASSERT(bda_array[j] != null_buffer);
                            auto const device_addr = temp_push->GetDeviceAddress(j);
                            if(device_addr == null_buffer){
                                Log::Warning("invalid push buffer being writen to file\n");
                                hash_buffer = INVALID_HASH;
                            }
                            else{
                                hash_buffer = engine->assetManager.buffer.ConvertBDAToHash(device_addr);
                            }
                            outFile.write(reinterpret_cast<char*>(&hash_buffer), sizeof(AssetHash));
                        }
                        for(uint8_t j = 0; j < temp_push->texture_count; j++){
                            if(temp_push->GetTextureIndex(j) == null_texture){
                                Log::Warning("invalid push texture being writen to file\n");
                                hash_buffer = INVALID_HASH;
                            }
                            else{
                                hash_buffer = engine->assetManager.dii.ConvertTextureIndexToHash(temp_push->GetTextureIndex(j));
                            }
                            outFile.write(reinterpret_cast<char*>(&hash_buffer), sizeof(AssetHash));
                        }
                    }
                    else{
                        outFile.write(reinterpret_cast<char*>(param_pool.param_data[param_index].data[frame]), current_param_size);
                    }
                    //written_param_size += current_param_size;
                    param_index++;
                }
            }
        }

        if(pkg_type == Command::InstructionPackage::Type::Object) {
            auto const* obj_payload = reinterpret_cast<Command::ObjectPackage::Payload const*>(payload);
            uint8_t shader_count = 0;
            for(auto const& shader : obj_payload->shaders){
                if(shader != nullptr){
                    shader_count++;
                }
            }
            outFile.write(reinterpret_cast<char const*>(&shader_count), sizeof(uint8_t));
            for(auto const& shader : obj_payload->shaders){
                if(shader != nullptr){
                    AssetHash const temp_shader_hash = GetHash(*shader);
                    outFile.write(reinterpret_cast<char const*>(&temp_shader_hash), sizeof(AssetHash));
                }
            }

            outFile.write(reinterpret_cast<char const*>(&obj_payload->config), sizeof(ObjectRasterConfig));
        }

        outFile.close();
        return true;
    }

    template<>
    bool WriteAssetToFile(Command::InstructionPackage const& pkg, std::filesystem::path const& root_directory, std::filesystem::path const& path){
        switch(pkg.type){
            case Command::InstructionPackage::Base: 
                return WriteToInstPkgFile(pkg.paramPool, nullptr, pkg.type, root_directory, path);
                break;
            case Command::InstructionPackage::Object:
                return WriteToInstPkgFile(pkg.paramPool, &reinterpret_cast<Command::ObjectPackage const&>(pkg).payload, pkg.type, root_directory, path);
                break;
            default: EWE_UNREACHABLE;
        }
        EWE_UNREACHABLE;
    }

    template<>
    bool LoadAssetFromFile(Command::InstructionPackage* ret, std::filesystem::path const& root_directory, std::filesystem::path const& path){
        
        auto const full_dir = root_directory / path;
        Log::Debug("asset full dir : %s\n", full_dir.string().c_str());
        std::ifstream inFile{root_directory / path, std::ios::binary};

        if(!inFile.is_open()){
            if(!std::filesystem::exists(root_directory / path)){
                Log::Error("atempting to open instruction pkg but path[%s] doesn't exist", path.string().c_str());
                return false;
            }
            inFile.open(root_directory / path, std::ios::binary);
            if(!inFile.is_open()){
                return false;
            }
        }

        std::size_t temp_buffer = file_version;
        inFile.read(reinterpret_cast<char*>(&temp_buffer), sizeof(std::size_t));
        Command::InstructionPackage::Type pkg_type;
        inFile.read(reinterpret_cast<char*>(&pkg_type), sizeof(Command::InstructionPackage::Type));
        switch(pkg_type){
            case Command::InstructionPackage::Base: 
                //ret = &engine->assetManager.instPkg.ConstructInto();
                ret = std::construct_at(ret);
                //EWE_ASSERT(dynamic_cast<
                break;
            case Command::InstructionPackage::Object: 
                //ret = static_cast<Command::InstructionPackage*>(&engine->assetManager.objPkg.ConstructInto());
                ret = static_cast<Command::InstructionPackage*>(std::construct_at(static_cast<Command::ObjectPackage*>(ret)));
                //EWE_ASSERT(dynamic_cast<Command::ObjectPackage*>(ret) != nullptr);
                break;
            default: break;
        }
        inFile.read(reinterpret_cast<char*>(&temp_buffer), sizeof(std::size_t));
        const std::size_t instruction_count = temp_buffer;
        ret->paramPool.instructions.resize(instruction_count);
        std::size_t written_param_size = 0;
        Log::Debug("current read position, before written param size : %zu\n", inFile.tellg());
        inFile.read(reinterpret_cast<char*>(&written_param_size), sizeof(std::size_t));
        inFile.read(reinterpret_cast<char*>(ret->paramPool.instructions.data()), instruction_count * sizeof(Inst::Type));
        Log::Debug("current read position, after writing instructions : %zu\n", inFile.tellg());
        
        for_each_frame{
            ret->paramPool.params[frame].Resize(written_param_size);
        }
        PerFlight<std::size_t> starting_addr{reinterpret_cast<std::size_t>(ret->paramPool.params[0].memory), reinterpret_cast<std::size_t>(ret->paramPool.params[1].memory)};
        
        std::size_t current_param_offset = 0;
        for(auto& inst : ret->paramPool.instructions){
            const auto current_param_size = Inst::GetParamSize(inst);
            if(current_param_size > 0){
                auto& back_param = ret->paramPool.param_data.emplace_back();

                for_each_frame{
                    back_param.data[frame] = starting_addr[frame] + current_param_offset;
                }
                current_param_offset += current_param_size;
            }
        }

        //std::size_t current_file_param_offset = 0; //theres a disparity between Push Param Size, and the written size
        for_each_frame {
            std::size_t param_index = 0;
            for(std::size_t i = 0; i < ret->paramPool.instructions.size(); i++){
                const auto current_param_size = Inst::GetParamSize(ret->paramPool.instructions[i]);
                if(current_param_size > 0){
                    //convert the buffer_address to a buffer hash
                    //convert the texture index to a dii hash
                    if(ret->paramPool.instructions[i] == Inst::Push){
                        Log::Debug("reading push at [%zu] : frame[%u]\n", inFile.tellg(), frame);
                        auto* temp_push = reinterpret_cast<ParamPack<Inst::Push>*>(ret->paramPool.param_data[param_index].data[frame]);
                        inFile.read(reinterpret_cast<char*>(&temp_push->buffer_count), sizeof(temp_push->buffer_count));
                        inFile.read(reinterpret_cast<char*>(&temp_push->texture_count), sizeof(temp_push->texture_count));

                        AssetHash hash_buffer;
                        for(uint8_t j = 0; j < temp_push->buffer_count; j++){
                            inFile.read(reinterpret_cast<char*>(&hash_buffer), sizeof(AssetHash));
                            if(hash_buffer != INVALID_HASH){
                                temp_push->GetDeviceAddress(j) = engine->assetManager.buffer.Get(hash_buffer)->deviceAddress;
                                
                            }
                            else{
                                temp_push->GetDeviceAddress(j) = null_buffer;
                            }
                        }
                        for(uint8_t j = 0; j < temp_push->texture_count; j++){
                            inFile.read(reinterpret_cast<char*>(&hash_buffer), sizeof(AssetHash));
                            if(hash_buffer != INVALID_HASH){
                                auto* temp_dii = engine->assetManager.dii.Get(hash_buffer);
                                temp_push->GetTextureIndex(j) = temp_dii->index;
                            }
                            else{
                                temp_push->GetTextureIndex(j) = null_texture;
                            }
                        }
                    }
                    else{
                        const std::size_t param_addr = ret->paramPool.param_data[param_index].data[frame];
                        inFile.read(reinterpret_cast<char*>(param_addr), current_param_size);
                    }
                    param_index++;
                }
            }
        }



        if(ret->type == Command::InstructionPackage::Type::Object) {
            uint8_t shader_count = 0;
            inFile.read(reinterpret_cast<char*>(&shader_count), sizeof(uint8_t));

            auto& obj_payload = reinterpret_cast<Command::ObjectPackage*>(ret)->payload;

            for(uint8_t i = 0; i < shader_count; i++){
                AssetHash temp_shader_hash;
                inFile.read(reinterpret_cast<char*>(&temp_shader_hash), sizeof(AssetHash));
                auto* shader_get = engine->assetManager.shader.Get(temp_shader_hash);
                auto& payload_shader = obj_payload.shaders[ShaderStage{shader_get->shaderStageCreateInfo.stage}.value];
                EWE_ASSERT(payload_shader == nullptr && shader_get != nullptr);
                payload_shader = shader_get;

            }

            inFile.read(reinterpret_cast<char*>(&obj_payload.config), sizeof(ObjectRasterConfig));
        }

        inFile.close();
        ret->name = path.string();
        return ret;
    }

} //namespace Asset
} //namespace EWE