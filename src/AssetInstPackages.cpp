#include "EWEngine/Assets/InstPackages.h"
#include "EightWinds/GlobalPushConstant.h"

namespace EWE{
namespace Asset{
    
    Manager<Command::InstructionPackage>::Manager(LogicalDevice& _logicalDevice, std::filesystem::path const& root_path)
    : logicalDevice{_logicalDevice},
        files{root_path, std::vector<std::string>{".eri"}}
    {
    }

    void Manager<Command::InstructionPackage>::Destroy(AssetHash hash){
        Command::InstructionPackage* InstructionPackage = Get(hash);
        EWE_ASSERT(InstructionPackage != nullptr);
        data_arena.DestroyElement(InstructionPackage);
        association_container.Remove(hash);
    }
    void Manager<Command::InstructionPackage>::Destroy(Command::InstructionPackage* InstructionPackage){
        //do I hash it first? idk
        AssetHash hash = GetHash(*InstructionPackage);
        Destroy(hash);
    }

    Command::InstructionPackage* Manager<Command::InstructionPackage>::Get(AssetHash hash){
        auto iter = association_container.find(hash);
        if(iter != association_container.end()){
            return iter->value;
        }
        else{
            auto path_hash_data = files.hashed_path.at(hash);
            auto const& fs_path = path_hash_data.value;
            auto full_load_path = files.root_directory / fs_path;
            //Command::InstructionPackage& InstructionPackage = data_arena.AddElement(full_load_path.string());

            //potentially check for duplicates

            //association_container.push_back(hash, &InstructionPackage);
            return nullptr;//&InstructionPackage;
        }
    }
    Command::InstructionPackage* Manager<Command::InstructionPackage>::Get(std::string_view name){
        return Get(CrossPlatformPathHash(name));
    }

#ifdef EWE_IMGUI
    void Manager<Command::InstructionPackage>::Imgui(){

    }
#endif


    static constexpr std::size_t file_version = 0;
//


    bool Manager<Command::InstructionPackage>::WriteToFile(Command::InstructionPackage& pkg, std::filesystem::path const& path){
        std::ofstream outFile{path, std::ios::binary};
        if(!outFile.is_open()){
            outFile.open(path, std::ios::binary);
            if(!outFile.is_open()){
                return false;
            }
        }
        std::size_t temp_buffer = file_version;
        outFile.write(reinterpret_cast<char*>(&temp_buffer), sizeof(std::size_t));
        outFile.write(reinterpret_cast<const char*>(&pkg.type), sizeof(uint8_t));
        temp_buffer = pkg.paramPool.instructions.size();
        outFile.write(reinterpret_cast<char*>(&temp_buffer), sizeof(std::size_t));
        
        std::size_t written_param_size = 0;
        //just checking sizes now, and going to write that size to file
        //for(uint8_t frame = 0; frame < max_frames_in_flight; frame++) {
            //std::size_t param_index = 0;
            for(std::size_t i = 0; i < pkg.paramPool.instructions.size(); i++){
                const auto current_param_size = Instruction::GetParamSize(pkg.paramPool.instructions[i]);
                if(current_param_size > 0){
                    if(pkg.paramPool.instructions[i] == Inst::Push){
                        //the globalpushconstant_raw is 96 bytes, but giving it some padding will help keep the file version stable
                        //im expecting ^ something there in the future to make push larger (April 2nd, 2026)
                        written_param_size += 128;
                    }
                    else{
                        written_param_size += current_param_size;
                    }
                    //param_index++;
                }
            }
            outFile.write(reinterpret_cast<char*>(&written_param_size), sizeof(std::size_t));
        //}
        outFile.write(reinterpret_cast<char*>(pkg.paramPool.instructions.data()), temp_buffer * sizeof(Inst::Type));
    
        //actually writign to file now
        for(uint8_t frame = 0; frame < max_frames_in_flight; frame++) {
            std::size_t param_index = 0;
            for(std::size_t i = 0; i < pkg.paramPool.instructions.size(); i++){
                const auto current_param_size = Instruction::GetParamSize(pkg.paramPool.instructions[i]);
                if(current_param_size > 0){

                    //convert the buffer_address to a buffer hash
                    //convert the texture index to a dii hash
                    if(pkg.paramPool.instructions[i] == Inst::Push){
                        written_param_size += 1;
                        GlobalPushConstant_Raw* temp_push = reinterpret_cast<GlobalPushConstant_Raw*>(pkg.paramPool.param_data[param_index].data[frame]);
                        AssetHash hash_buffer;
                        
                        for(uint8_t j = 0; j < GlobalPushConstant_Raw::buffer_count; j++){
                            if(temp_push->buffer_addr[j] == null_buffer){
                                break;
                            }
                            hash_buffer = Global::buffers->ConvertBDAToHash(temp_push->buffer_addr[j]);
                            outFile.write(reinterpret_cast<char*>(&hash_buffer), sizeof(AssetHash));
                        }
                        for(uint8_t j = 0; j < GlobalPushConstant_Raw::texture_count; j++){
                            if(temp_push->texture_indices[j] == null_texture){
                                break;
                            }
                            hash_buffer = Global::diis->ConvertTextureIndexToHash(temp_push->texture_indices[j]);
                            outFile.write(reinterpret_cast<char*>(&hash_buffer), sizeof(AssetHash));
                        }
                    }
                    written_param_size += current_param_size;
                    param_index++;
                }
            }
        }


        //what I need to do is expand push constants
        temp_buffer = pkg.paramPool.params[0].Size();
        //EWE_ASSERT(paramPool.params[0].Size() == paramPool.params[1].Size());
        //outFile.write(&temp_buffer, sizeof(std::size_t));
        //and get a stable reference point to the buffers and DIIs
        //outFile.write(paramPool.params[0].memory, paramPool.params[0].Size());
        //outFile.write(paramPool.params[1].memory, paramPool.params[1].Size());
        if(pkg.type == Command::InstructionPackage::Type::Raster){
            
        }
    }

    /*
    Command::InstructionPackage ReadFile(std::filesystem::path const& path){

}
    */

} //namespace Asset
} //namespace EWE