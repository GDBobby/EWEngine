#include "EWEngine/Assets/InstPackages.h"

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
            Command::InstructionPackage& InstructionPackage = data_arena.AddElement(full_load_path.string());

            //potentially check for duplicates

            association_container.push_back(hash, &InstructionPackage);
            return &InstructionPackage;
        }
    }
    Command::InstructionPackage* Manager<Command::InstructionPackage>::Get(std::string_view name){
        return Get(CrossPlatformPathHash(name));
    }

#ifdef EWE_IMGUI
    void Manager<Command::InstructionPackage>::Imgui(){

    }
#endif
} //namespace Asset
} //namespace EWE