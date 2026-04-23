#include "EWEngine/Assets/Hash.h"
#include "EWEngine/Assets/BaseInstPackages.h"
#include "EWEngine/Global.h"
#include "EightWinds/Backend/Logger.h"
#include "EightWinds/Command/InstructionPackage.h"
#include "EightWinds/GlobalPushConstant.h"

#include "EightWinds/Command/ObjectInstructionPackage.h"
#include "EightWinds/VulkanHeader.h"

#include "EWEngine/Imgui/DragDrop.h"

#include <fstream>

namespace EWE{
namespace Asset{
    
    Manager<Command::InstructionPackage>::Manager(std::filesystem::path const& root_path)
    : files{root_path, std::vector<std::string>{".eip"}}
    {
    }

    void Manager<Command::InstructionPackage>::Destroy(AssetHash hash){
        Command::InstructionPackage& InstructionPackage = Get(hash);
        data_arena.DestroyElement(&InstructionPackage);
        association_container.Remove(hash);
    }
    void Manager<Command::InstructionPackage>::Destroy(Command::InstructionPackage& instPackage){
        //do I hash it first? idk
        AssetHash hash = GetHash(instPackage);
        Destroy(hash);
    }

    Command::InstructionPackage& Manager<Command::InstructionPackage>::Get(AssetHash hash) {
        auto iter = association_container.find(hash);
        if(iter != association_container.end()){
            return *iter->value;
        }
        else{
            auto path_hash_data = files.hashed_path.at(hash);
            auto const& fs_path = path_hash_data.value;
            auto full_load_path = files.root_directory / fs_path;

            auto& ret = data_arena.AddElement();
            if(ReadInstPkgFile(&ret, full_load_path)){
                association_container.push_back(hash, &ret);
            }
            else{
                data_arena.DestroyElement(&ret);
                EWE_ASSERT(false, "failed to construct");
            }

            return ret;
        }
    }
    Command::InstructionPackage& Manager<Command::InstructionPackage>::Get(std::filesystem::path const& name){
        return Get(CrossPlatformPathHash(name));
    }

#ifdef EWE_IMGUI
    void Manager<Command::InstructionPackage>::Imgui(){
        if(ImGui::Button("refresh files")){
            files.RefreshFiles();
        }

        for(auto& kvp : files.hashed_path){
            auto found = association_container.find(kvp.key);
            if(found == association_container.end()){
                if(ImGui::Button(kvp.value.string().c_str())){
                    Get(kvp.value);
                }
            }
        }
        for(auto& kvp : association_container){
            if(ImGui::TreeNode(kvp.value->name.c_str())){
                DragDropPtr::Source(*kvp.value);
                ImGui::TreePop();
            }
            //Logger::Print("%s\n", kvp.value->name.c_str());
            DragDropPtr::Source(*kvp.value);
        }
    }
#endif


    static constexpr std::size_t file_version = 0;
//

    bool WriteToInstPkgFile(Command::ParamPool const& param_pool, void* payload, Command::InstructionPackage::Type pkg_type, std::filesystem::path const& temp_path){
        
        std::filesystem::path adjusted_path = Global::assetManager->root_directory / temp_path;
        
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
        Logger::Print("current write position, before written param size : %zu\n", outFile.tellp());
        outFile.write(reinterpret_cast<char*>(&written_param_size), sizeof(std::size_t));
        outFile.write(reinterpret_cast<const char*>(param_pool.instructions.data()), temp_buffer * sizeof(Inst::Type));
        Logger::Print("current write position, after writing instructions : %zu\n", outFile.tellp());
    
        //actually writign to file now
        for(uint8_t frame = 0; frame < max_frames_in_flight; frame++) {
            std::size_t param_index = 0;
            for(std::size_t i = 0; i < param_pool.instructions.size(); i++){
                const auto current_param_size = Inst::GetParamSize(param_pool.instructions[i]);
                if(current_param_size > 0){

                    //convert the buffer_address to a buffer hash
                    //convert the texture index to a dii hash
                    if(param_pool.instructions[i] == Inst::Push){
                        Logger::Print("writing push at [%zu] : frame[%u]\n", outFile.tellp(), frame);
                        auto* temp_push = reinterpret_cast<ParamPack<Inst::Push>*>(param_pool.param_data[param_index].data[frame]);
                        outFile.write(reinterpret_cast<char*>(&temp_push->buffer_count), sizeof(temp_push->buffer_count));
                        outFile.write(reinterpret_cast<char*>(&temp_push->texture_count), sizeof(temp_push->texture_count));
                        AssetHash hash_buffer;
                        
                        for(uint8_t j = 0; j < temp_push->buffer_count; j++){
                            //EWE_ASSERT(bda_array[j] != null_buffer);
                            if(temp_push->GetDeviceAddress(j) == null_buffer){
                                Logger::Print<Logger::Warning>("invalid push buffer being writen to file\n");
                                hash_buffer = INVALID_HASH;
                            }
                            else{
                                hash_buffer = Global::assetManager->buffer.ConvertBDAToHash(temp_push->GetDeviceAddress(j));
                            }
                            outFile.write(reinterpret_cast<char*>(&hash_buffer), sizeof(AssetHash));
                        }
                        for(uint8_t j = 0; j < temp_push->texture_count; j++){
                            if(temp_push->GetTextureIndex(j) == null_texture){
                                Logger::Print<Logger::Warning>("invalid push texture being writen to file\n");
                                hash_buffer = INVALID_HASH;
                            }
                            else{
                                hash_buffer = Global::assetManager->dii.ConvertTextureIndexToHash(temp_push->GetTextureIndex(j));
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

        outFile.close();
        return true;
    }

    bool WriteToInstPkgFile(Command::InstructionPackage& pkg, std::filesystem::path const& path){
        switch(pkg.type){
            case Command::InstructionPackage::Base: 
                return WriteToInstPkgFile(pkg.paramPool, nullptr, pkg.type, path);
                break;
            case Command::InstructionPackage::Object:
                return WriteToInstPkgFile(pkg.paramPool, &reinterpret_cast<Command::ObjectPackage&>(pkg).payload, pkg.type, path);
                break;
            default: EWE_UNREACHABLE;
        }
        EWE_UNREACHABLE;
    }

    bool ReadInstPkgFile(Command::InstructionPackage* ret, std::filesystem::path const& path){
        std::ifstream inFile{path, std::ios::binary};

        if(!inFile.is_open()){
            if(!std::filesystem::exists(path)){
                Logger::Print<Logger::Error>("atempting to open instruction pkg but path[%s] doesn't exist", path.string().c_str());
                return false;
            }
            inFile.open(path, std::ios::binary);
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
                //ret = new Command::InstructionPackage(); 
                //EWE_ASSERT(dynamic_cast<
                break;
            case Command::InstructionPackage::Object: 
                //ret = reinterpret_cast<Command::InstructionPackage*>(new Command::ObjectPackage());
                //EWE_ASSERT(dynamic_cast<Command::ObjectPackage*>(ret) != nullptr);
                break;
            default: break;
        }
        inFile.read(reinterpret_cast<char*>(&temp_buffer), sizeof(std::size_t));
        const std::size_t instruction_count = temp_buffer;
        ret->paramPool.instructions.resize(instruction_count);
        std::size_t written_param_size = 0;
        Logger::Print("current read position, before written param size : %zu\n", inFile.tellg());
        inFile.read(reinterpret_cast<char*>(&written_param_size), sizeof(std::size_t));
        inFile.read(reinterpret_cast<char*>(ret->paramPool.instructions.data()), instruction_count * sizeof(Inst::Type));
        Logger::Print("current read position, after writing instructions : %zu\n", inFile.tellg());
        
        for(uint8_t frame = 0; frame < max_frames_in_flight; frame++){
            ret->paramPool.params[frame].Resize(written_param_size);
        }
        PerFlight<std::size_t> starting_addr{reinterpret_cast<std::size_t>(ret->paramPool.params[0].memory), reinterpret_cast<std::size_t>(ret->paramPool.params[1].memory)};
        
        std::size_t current_param_offset = 0;
        for(auto& inst : ret->paramPool.instructions){
            const auto current_param_size = Inst::GetParamSize(inst);
            if(current_param_size > 0){
                auto& back_param = ret->paramPool.param_data.emplace_back();

                for(uint8_t frame = 0; frame < max_frames_in_flight; frame++){
                    back_param.data[frame] = starting_addr[frame] + current_param_offset;
                }
                current_param_offset += current_param_size;
            }
        }

        //std::size_t current_file_param_offset = 0; //theres a disparity between Push Param Size, and the written size
        for(uint8_t frame = 0; frame < max_frames_in_flight; frame++) {
            std::size_t param_index = 0;
            for(std::size_t i = 0; i < ret->paramPool.instructions.size(); i++){
                const auto current_param_size = Inst::GetParamSize(ret->paramPool.instructions[i]);
                if(current_param_size > 0){
                    //convert the buffer_address to a buffer hash
                    //convert the texture index to a dii hash
                    if(ret->paramPool.instructions[i] == Inst::Push){
                        Logger::Print("reading push at [%zu] : frame[%u]\n", inFile.tellg(), frame);
                        auto* temp_push = reinterpret_cast<ParamPack<Inst::Push>*>(ret->paramPool.param_data[param_index].data[frame]);
                        inFile.read(reinterpret_cast<char*>(&temp_push->buffer_count), sizeof(temp_push->buffer_count));
                        inFile.read(reinterpret_cast<char*>(&temp_push->texture_count), sizeof(temp_push->texture_count));

                        AssetHash hash_buffer;
                        for(uint8_t j = 0; j < temp_push->buffer_count; j++){
                            inFile.read(reinterpret_cast<char*>(&hash_buffer), sizeof(AssetHash));
                            if(hash_buffer != INVALID_HASH){
                                temp_push->GetDeviceAddress(j) = Global::assetManager->buffer.Get(hash_buffer).deviceAddress;
                                
                            }
                            else{
                                temp_push->GetDeviceAddress(j) = null_buffer;
                            }
                        }
                        for(uint8_t j = 0; j < temp_push->texture_count; j++){
                            inFile.read(reinterpret_cast<char*>(&hash_buffer), sizeof(AssetHash));
                            if(hash_buffer != INVALID_HASH){
                                auto& temp_dii = Global::assetManager->dii.Get(hash_buffer);
                                temp_push->GetTextureIndex(j) = temp_dii.index;
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
            Command::ObjectPackage* objPkg = static_cast<Command::ObjectPackage*>(ret);
            inFile.read(reinterpret_cast<char*>(&objPkg->payload), sizeof(objPkg->payload));
        }

        inFile.close();
        ret->name = std::filesystem::proximate(path, Global::assetManager->root_directory).string();
        return ret;
    }

} //namespace Asset
} //namespace EWE