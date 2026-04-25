#include "EWEngine/Assets/PackageRecords.h"
#include "EWEngine/Global.h"

#include "EWEngine/Imgui/DragDrop.h"
#include <fstream>

namespace EWE{
namespace Asset{

    static constexpr uint64_t current_file_version = 0;

    template<>
    bool WriteAssetToFile(::EWE::Command::PackageRecord const& rec, std::filesystem::path const& root_directory, std::filesystem::path const& path){
        //if it has the old record its invalid
        //if it doesnt have an instruction package, its special execution, aka hand coded, cant be written

        std::ofstream outFile{root_directory / path, std::ios::binary};
        if(!outFile.is_open()){
            return false;
        }

        std::size_t temp_buffer = current_file_version;
        outFile.write(reinterpret_cast<char*>(&temp_buffer), sizeof(temp_buffer));
        
        if(rec.queue == nullptr){
            outFile.close();
            return false;
        }
        auto queue_type = Global::stcManager->GetQueueType(*rec.queue);
        outFile.write(reinterpret_cast<char*>(&queue_type), sizeof(queue_type));

        temp_buffer = rec.packages.size();
        outFile.write(reinterpret_cast<char*>(&temp_buffer), sizeof(temp_buffer));

        for(auto* pkg : rec.packages){
            AssetHash hash_buffer = GetHash(*pkg);
            outFile.write(reinterpret_cast<char*>(&hash_buffer), sizeof(AssetHash));
        }
        outFile.close();
        return true;
    }

    template<>
    bool LoadAssetFromFile(Command::PackageRecord* ptr_to_raw_mem, std::filesystem::path const& root_directory, std::filesystem::path const& name){
        std::ifstream inFile{root_directory / name, std::ios::binary};
        if(!inFile.is_open()){
            return false;
        }

        std::size_t temp_buffer;
        inFile.read(reinterpret_cast<char*>(&temp_buffer), sizeof(temp_buffer));
        if(temp_buffer != current_file_version){
            inFile.close();
            return false;
        }
        
        auto& ret = *std::construct_at(ptr_to_raw_mem);
        ret.name = name;
        Queue::Type queue_type;
        inFile.read(reinterpret_cast<char*>(&queue_type), sizeof(queue_type));
        ret.queue = &Global::stcManager->GetQueue(queue_type);

        inFile.read(reinterpret_cast<char*>(&temp_buffer), sizeof(temp_buffer));

        ret.packages.reserve(temp_buffer);
        for(std::size_t i = 0; i < temp_buffer; i++){
            AssetHash hash_buffer;
            inFile.read(reinterpret_cast<char*>(&hash_buffer), sizeof(AssetHash));
            ret.packages.push_back(Global::assetManager->objPkg.Get(hash_buffer));
        }

        inFile.close();
        return true;
    }

} //namepsace Asset
} //namespace EWE