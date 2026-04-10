#include "EWEngine/Assets/PackageRecords.h"
#include "EWEngine/Global.h"


#include "EWEngine/Imgui/DragDrop.h"
#include "EightWinds/Command/PackageRecord.h"
#include <fstream>

namespace EWE{
namespace Asset{
    Manager<SubmissionTask>::Manager(LogicalDevice& _logicalDevice, std::filesystem::path const& root_path)
    : logicalDevice{_logicalDevice},
        files{root_path, std::vector<std::string>{".epr"}}
    {

    }

    void Manager<SubmissionTask>::Destroy(AssetHash hash){
        auto& rec = Get(hash);
        data_arena.DestroyElement(&rec);
        association_container.Remove(hash);
    }
    void Manager<SubmissionTask>::Destroy(SubmissionTask& rec){
        AssetHash hash = GetHash(rec);
        Destroy(hash);
    }

    SubmissionTask& Manager<SubmissionTask>::Get(AssetHash hash){
        auto iter = association_container.find(hash);
        if(iter != association_container.end()){
            return *iter->value;
        }
        else{
            auto path_hash_data = files.hashed_path.at(hash);
            auto const& fs_path = path_hash_data.value;
            auto full_load_path = files.root_directory / fs_path;

            auto& ret = ReadFile(full_load_path);
            association_container.push_back(hash, &ret);
            return ret;
        }
    }
    SubmissionTask& Manager<SubmissionTask>::Get(std::filesystem::path const& name){
        //potentially enforce it exists in the file system, and enforce create is called for construction
        return Get(CrossPlatformPathHash(name));
    }

#ifdef EWE_IMGUI
    void Manager<SubmissionTask>::Imgui(){
        if(ImGui::Button("refresh files")){
            files.RefreshFiles();
        }
        
        for(auto& kvp : files.hashed_path){
            auto found = association_container.find(kvp.key);
            if(found == association_container.end()){
                if(ImGui::Button(kvp.value.string().c_str())){
                    Get(kvp.key);
                }
            }
        }
        for(auto& kvp : association_container){
            if(ImGui::TreeNode(kvp.value->name.c_str())) {
                DragDropPtr::Source(*kvp.value);
                //ImGuiExtension::Imgui(*kvp.value);
                ImGui::TreePop();
            }
            else{
                DragDropPtr::Source(*kvp.value);
            }
            
        }
    }
#endif

    static constexpr uint64_t current_file_version = 0;

    bool Manager<SubmissionTask>::WriteToFile(SubmissionTask const& task){
        //if it has the old record its invalid
        //if it doesnt have an instruction package, its special execution, aka hand coded, cant be written
        if(task.specializedSubmission){
            return false;
        }

        std::ofstream outFile{task.name.c_str(), std::ios::binary};

        std::size_t temp_buffer = current_file_version;
        outFile.write(reinterpret_cast<char*>(&temp_buffer), sizeof(temp_buffer));
        
        if(task.queue == nullptr){
            return false;
        }
        auto queue_type = Global::stcManager->GetQueueType(*task.queue);
        outFile.write(reinterpret_cast<char*>(&queue_type), sizeof(queue_type));

        temp_buffer = task.tasks.size();
        outFile.write(reinterpret_cast<char*>(&temp_buffer), sizeof(temp_buffer));

        for(auto* gpu_task : task.tasks){
            AssetHash hash_buffer = Global::pkgRecords->GetHash(*gpu_task->pkgRecord);
            outFile.write(reinterpret_cast<char*>(&hash_buffer), sizeof(AssetHash));
        }
        return true;
    }

    SubmissionTask& Manager<SubmissionTask>::ReadFile(std::filesystem::path const& name){
        std::ifstream inFile{name, std::ios::binary};

        std::size_t temp_buffer;
        inFile.read(reinterpret_cast<char*>(&temp_buffer), sizeof(temp_buffer));
        EWE_ASSERT(temp_buffer == current_file_version, "needs support otherwise");
        
        auto& ret = data_arena.AddElement(name.string(), logicalDevice, Global::stcManager->renderQueue);
        association_container.push_back(GetHash(ret), &ret);
        Queue::Type queue_type;
        inFile.read(reinterpret_cast<char*>(&queue_type), sizeof(queue_type));
        ret.queue = &Global::stcManager->GetQueue(queue_type);

        inFile.read(reinterpret_cast<char*>(&temp_buffer), sizeof(temp_buffer));
        ret.tasks.reserve(temp_buffer);
        for(std::size_t i = 0; i < temp_buffer; i++){
            AssetHash hash_buffer;
            inFile.read(reinterpret_cast<char*>(&hash_buffer), sizeof(AssetHash));
            //create a gpu task from the inst package
            Command::PackageRecord& pkgRecord = EWE::Global::pkgRecords->Get(hash_buffer);
            GPUTask* task = new GPUTask(pkgRecord.name, logicalDevice, pkgRecord);
            ret.tasks.push_back(task);
        }

        return ret;
    }

} //namepsace Asset
} //namespace EWE