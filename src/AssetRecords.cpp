#include "EWEngine/Assets/Records.h"

namespace EWE{
namespace Asset{
    
    Manager<Command::Record>::Manager(LogicalDevice& _logicalDevice, std::filesystem::path const& root_path)
    : logicalDevice{_logicalDevice},
        files{root_path, std::vector<std::string>{".eri"}}
    {
        
    }

    void Manager<Command::Record>::Destroy(AssetHash hash){
        Command::Record* record = Get(hash);
        EWE_ASSERT(record != nullptr);
        data_arena.DestroyElement(record);
        association_container.Remove(hash);
    }
    void Manager<Command::Record>::Destroy(Command::Record* record){
        //do I hash it first? idk
        AssetHash hash = GetHash(*record);
        Destroy(hash);
    }

    Command::Record* Manager<Command::Record>::Get(AssetHash hash){
        auto iter = association_container.find(hash);
        if(iter != association_container.end()){
            return iter->value;
        }
        else{
            auto path_hash_data = files.hashed_path.at(hash);
            auto const& fs_path = path_hash_data.value;
            auto full_load_path = files.root_directory / fs_path;
            Command::Record& record = data_arena.AddElement(full_load_path.string());

            //potentially check for duplicates

            association_container.push_back(hash, &record);
            return &record;
        }
    }
    Command::Record* Manager<Command::Record>::Get(std::string_view name){
        return Get(CrossPlatformPathHash(name));
    }

#ifdef EWE_IMGUI
    void Manager<Command::Record>::Imgui(){

    }
#endif
} //namespace Asset
} //namespace EWE