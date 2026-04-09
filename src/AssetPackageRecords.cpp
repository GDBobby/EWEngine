#include "EWEngine/Assets/Tasks.h"
#include "EWEngine/Global.h"

namespace EWE{
namespace Asset{
    Manager<Command::PackageRecord>::Manager(LogicalDevice& _logicalDevice, std::filesystem::path const& root_path)
    : logicalDevice{_logicalDevice},
        files{root_path, std::vector<std::string>{".egt"}}
    {

    }

    void Manager<Command::PackageRecord>::Destroy(AssetHash hash){
        auto& rec = Get(hash);
        data_arena.DestroyElement(&rec);
        association_container.Remove(hash);
    }
    void Manager<Command::PackageRecord>::Destroy(Command::PackageRecord& rec){
        AssetHash hash = GetHash(rec);
        Destroy(hash);
    }

    Command::PackageRecord& Manager<Command::PackageRecord>::Get(AssetHash hash){
auto iter = association_container.find(hash);
        if(iter != association_container.end()){
            return iter->value;
        }
        else{
            auto path_hash_data = files.hashed_path.at(hash);
            auto const& fs_path = path_hash_data.value;
            auto full_load_path = files.root_directory / fs_path;

            return ReadFile(full_load_path);
        }
    }
    Command::PackageRecord Manager<Command::PackageRecord>::Get(std::filesystem::path const& name){
        //potentially enforce it exists in the file system, and enforce create is called for construction
        return Get(CrossPlatformPathHash(name));
    }

#ifdef EWE_IMGUI
    void Imgui(){

    }
#endif

    static constexpr uint64_t current_file_version = 0;

    bool Manager<Command::PackageRecord>::WriteToFile(Command::PackageRecord const& rec){
        //if it has the old record its invalid
        //if it doesnt have an instruction package, its special execution, aka hand coded, cant be written

        std::ofstream outFile{rec.name.c_str(), std::ios::binary};

        std::size_t temp_buffer = current_file_version;
        outFile.write(reinterpret_cast<char*>(&temp_buffer), sizeof(temp_buffer));
        
        auto queue_type = Global::stcManager->GetQueueType(*rec.queue);
        outFile.write(reinterpret_cast<char*>(&queue_type), sizeof(queue_type));

        temp_buffer = rec.packages.size();
        outFile.write(reinterpret_cast<char*>(&temp_buffer), sizeof(temp_buffer));

        for(auto* pkg : rec.packages){
            AssetHash hash_buffer = EWE::Global::instPackages->GetHash(*pkg);
            outFile.write(reinterpret_cast<char*>(&hash_buffer), sizeof(AssetHash));
        }
    }

    Command::PackageRecord& Manager<Command::PackageRecord>::ReadFile(std::filesystem::path const& name){
        std::ifstream inFile{name, std::ios::binary};

        std::size_t temp_buffer;
        inFile.read(reinterpret_cast<char*>(&temp_buffer), sizeof(temp_buffer));
        EWE_ASSERT(temp_buffer == current_file_version, "needs support otherwise");
        
        auto& ret = data_arena.AddElement();
        association_container.emplace_back(GetHash(name), &ret);
        Queue::Type queue_type;
        inFile.read(reinterpret_cast<char*>(&queue_type), sizeof(queue_type));
        ret.queue = &Global::stcManager->GetQueue(queue_type);

        inFile.read(reinterpret_cast<char*>(&temp_buffer), sizeof(temp_buffer));

        ret.packages.reserve(temp_buffer);
        for(std::size_t i = 0; i < temp_buffer; i++){
            AssetHash hash_buffer;
            inFile.read(reinterpret_cast<char*>(&hash_buffer), sizeof(AssetHash));
            ret.packages.push_back(EWE::Global::instPackages->Get(hash_buffer));
        }

        return ret;
    }

} //namepsace Asset
} //namespace EWE