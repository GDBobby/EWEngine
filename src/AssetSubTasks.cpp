#include "EWEngine/Assets/Base.h"
#include "EWEngine/Assets/Hash.h"
#include "EWEngine/Assets/PackageRecords.h"

#include "EWEngine/EWEngine.h"


#include "EWEngine/Imgui/DragDrop.h"
#include "EightWinds/Command/PackageRecord.h"

#include "EWEngine/Assets/RenderAttachments.h"

#include <fstream>

#include "EightWinds/Data/StreamHelper.h"

namespace EWE{
namespace Asset{
    static constexpr uint64_t current_file_version = 1;

    template<typename S>
    void WriteRenderInfo(FullRenderInfo& ri, std::ofstream& outFile){
        WriteAttachmentInfoToFile(outFile, ri.full.setInfo);
    }

    template<>
    bool WriteAssetToFile(SubmissionTask const& task, std::filesystem::path const& root_directory, std::filesystem::path const& path){
        //if it has the old record its invalid
        //if it doesnt have an instruction package, its special execution, aka hand coded, cant be written
        if(task.specializedSubmission){
            return false;
        }

        auto const combined_path = root_directory / path;
        Log::Debug("writing sub task : %s\n", combined_path.string().c_str());
        std::ofstream outFile{combined_path, std::ios::binary};
        if(!outFile.is_open()){
            outFile.open(combined_path, std::ios::binary);
            if(!outFile.is_open()){
                Log::Warning("failed to open sub task asset file twice : %s / %s\n", root_directory.string().c_str(), path.string().c_str());
            }
            return false;
        }

        std::size_t temp_buffer = current_file_version;
        outFile.write(reinterpret_cast<char*>(&temp_buffer), sizeof(temp_buffer));
        
        if(task.queue == nullptr){
            outFile.close();
            Log::Warning("queue was nullptr in written subtask\n");
            return false;
        }
        auto queue_type = engine->GetQueueType(*task.queue);
        outFile.write(reinterpret_cast<char*>(&queue_type), sizeof(queue_type));

        temp_buffer = task.tasks.size();
        outFile.write(reinterpret_cast<char*>(&temp_buffer), sizeof(temp_buffer));

        for(auto* gpu_task : task.tasks){
            AssetHash hash_buffer = GetHash(*gpu_task->pkgRecord);
            EWE_ASSERT(hash_buffer != INVALID_HASH);
            outFile.write(reinterpret_cast<char*>(&hash_buffer), sizeof(AssetHash));
        }

        outFile.close();
        Log::Debug("written size : %zu\n", std::filesystem::file_size(combined_path));
        return true;
    }

    template<>
    bool LoadAssetFromFile(SubmissionTask* ptr_to_raw_mem, std::filesystem::path const& root_directory, std::filesystem::path const& name){
        const std::filesystem::path combined_path = root_directory / name;
        std::ifstream inFile{combined_path, std::ios::binary};
        if(!inFile.is_open()){
            if(!std::filesystem::exists(combined_path)){
                Log::Debug("attempting to open a file that doesnt' exist : %s\n", combined_path.string().c_str());
            }
            return false;
        }
        Log::Debug("file size : %zu\n", std::filesystem::file_size(combined_path));

        std::size_t temp_buffer;
        inFile.read(reinterpret_cast<char*>(&temp_buffer), sizeof(temp_buffer));
        EWE_ASSERT(temp_buffer == current_file_version, "needs support otherwise");
        
        auto& ret = *std::construct_at(ptr_to_raw_mem, name.string(), engine->logicalDevice, engine->renderQueue);
        Queue::Type queue_type;
        inFile.read(reinterpret_cast<char*>(&queue_type), sizeof(queue_type));
        ret.queue = &engine->GetQueue(queue_type);

        inFile.read(reinterpret_cast<char*>(&temp_buffer), sizeof(temp_buffer));
        ret.tasks.reserve(temp_buffer);
        for(std::size_t i = 0; i < temp_buffer; i++){
            AssetHash hash_buffer;
            inFile.read(reinterpret_cast<char*>(&hash_buffer), sizeof(AssetHash));
            //create a gpu task from the inst package
            Command::PackageRecord* pkgRecord = EWE::Global::assetManager->pkgRecord.Get(hash_buffer);

            GPUTask* task = new GPUTask(pkgRecord->name, engine->logicalDevice, *pkgRecord, false);
            ret.tasks.push_back(task);
        }

        inFile.close();
        return true;
    }

} //namepsace Asset
} //namespace EWE