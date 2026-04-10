#include "EWEngine/Assets/Tasks.h"
#include "EWEngine/Global.h"
#include "EWEngine/STC_Manager.h"

namespace EWE{
namespace Asset{
    Manager<GPUTask>::Manager(LogicalDevice& _logicalDevice, std::filesystem::path const& root_path)
    : logicalDevice{_logicalDevice},
        files{root_path, std::vector<std::string>{".egt"}}
    {

    }

    void Manager<GPUTask>::Destroy(AssetHash hash){
        auto& task = Get(hash);
        data_arena.DestroyElement(&task);
        association_container.Remove(hash);
    }
    void Manager<GPUTask>::Destroy(GPUTask& task){
        AssetHash hash = GetHash(task);
        Destroy(hash);
    }

    GPUTask& Manager<GPUTask>::Get(AssetHash hash){
auto iter = association_container.find(hash);
        if(iter != association_container.end()){
            return *iter->value;
        }
        else{
            auto path_hash_data = files.hashed_path.at(hash);
            auto const& fs_path = path_hash_data.value;
            auto full_load_path = files.root_directory / fs_path;

            //return ReadFile(full_load_path);

            return data_arena.AddElement("invalid", logicalDevice, Global::stcManager->renderQueue);
        }
    }
    GPUTask& Manager<GPUTask>::Get(std::filesystem::path const& name){
        //potentially enforce it exists in the file system, and enforce create is called for construction
        return Get(CrossPlatformPathHash(name));
    }

#ifdef EWE_IMGUI
    void Imgui(){

    }
#endif

    bool Manager<GPUTask>::WriteToFile(GPUTask& task){
        //if it has the old record its invalid
        //if it doesnt have an instruction package, its special execution, aka hand coded, cant be written

        std::ofstream outFile{task.name.c_str(), std::ios::binary};

        return true;
    }

} //namepsace Asset
} //namespace EWE