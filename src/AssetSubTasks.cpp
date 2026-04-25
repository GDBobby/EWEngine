#include "EWEngine/Assets/PackageRecords.h"
#include "EWEngine/Global.h"


#include "EWEngine/Imgui/DragDrop.h"
#include "EightWinds/Command/PackageRecord.h"
#include <fstream>

namespace EWE{
namespace Asset{
    static constexpr uint64_t current_file_version = 0;

    template<>
    bool WriteAssetToFile(SubmissionTask const& task, std::filesystem::path const& root_directory, std::filesystem::path const& path){
        //if it has the old record its invalid
        //if it doesnt have an instruction package, its special execution, aka hand coded, cant be written
        if(task.specializedSubmission){
            return false;
        }

        std::ofstream outFile{root_directory / path, std::ios::binary};

        std::size_t temp_buffer = current_file_version;
        outFile.write(reinterpret_cast<char*>(&temp_buffer), sizeof(temp_buffer));
        
        if(task.queue == nullptr){
            outFile.close();
            return false;
        }
        auto queue_type = Global::stcManager->GetQueueType(*task.queue);
        outFile.write(reinterpret_cast<char*>(&queue_type), sizeof(queue_type));

        temp_buffer = task.tasks.size();
        outFile.write(reinterpret_cast<char*>(&temp_buffer), sizeof(temp_buffer));

        for(auto* gpu_task : task.tasks){
            AssetHash hash_buffer = GetHash(*gpu_task->pkgRecord);
            outFile.write(reinterpret_cast<char*>(&hash_buffer), sizeof(AssetHash));
        }

        outFile.close();
        return true;
    }

    template<>
    bool LoadAssetFromFile(SubmissionTask* ptr_to_raw_mem, std::filesystem::path const& root_directory, std::filesystem::path const& name){
        std::ifstream inFile{root_directory / name, std::ios::binary};
        if(!inFile.is_open()){
            return false;
        }

        std::size_t temp_buffer;
        inFile.read(reinterpret_cast<char*>(&temp_buffer), sizeof(temp_buffer));
        EWE_ASSERT(temp_buffer == current_file_version, "needs support otherwise");
        
        auto& ret = *std::construct_at(ptr_to_raw_mem, name.string(), *Global::logicalDevice, Global::stcManager->renderQueue);
        Queue::Type queue_type;
        inFile.read(reinterpret_cast<char*>(&queue_type), sizeof(queue_type));
        ret.queue = &Global::stcManager->GetQueue(queue_type);

        inFile.read(reinterpret_cast<char*>(&temp_buffer), sizeof(temp_buffer));
        ret.tasks.reserve(temp_buffer);
        for(std::size_t i = 0; i < temp_buffer; i++){
            AssetHash hash_buffer;
            inFile.read(reinterpret_cast<char*>(&hash_buffer), sizeof(AssetHash));
            //create a gpu task from the inst package
            Command::PackageRecord* pkgRecord = EWE::Global::assetManager->pkgRecord.Get(hash_buffer);

            GPUTask* task = new GPUTask(pkgRecord->name, *Global::logicalDevice, *pkgRecord, false);
            ret.tasks.push_back(task);
        }

        inFile.close();
        return true;
    }

} //namepsace Asset
} //namespace EWE