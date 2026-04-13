#include "EWEngine/Assets/BaseInstPackages.h"
#include "EWEngine/Assets/RasterPackages.h"
#include "EWEngine/Global.h"


#include "EWEngine/Imgui/DragDrop.h"
#include "EightWinds/Command/ObjectInstructionPackage.h"
#include "EightWinds/Pipeline/TaskRasterConfig.h"
#include <fstream>

namespace EWE{
namespace Asset{
    Manager<RasterPackage>::Manager(std::filesystem::path const& root_path)
    : files{root_path, std::vector<std::string>{".ert"}}
    {

    }

    void Manager<RasterPackage>::Destroy(AssetHash hash){
        auto& rec = Get(hash);
        data_arena.DestroyElement(&rec);
        association_container.Remove(hash);
    }
    void Manager<RasterPackage>::Destroy(RasterPackage& rt){
        AssetHash hash = GetHash(rt);
        Destroy(hash);
    }

    RasterPackage& Manager<RasterPackage>::Get(AssetHash hash){
auto iter = association_container.find(hash);
        if(iter != association_container.end()){
            return *iter->value;
        }
        else{
            auto path_hash_data = files.hashed_path.at(hash);
            auto const& fs_path = path_hash_data.value;
            auto full_load_path = files.root_directory / fs_path;

            auto& ret = ReadFile(full_load_path);
            //association_container.push_back(hash, &ret);
            return ret;
        }
    }
    RasterPackage& Manager<RasterPackage>::Get(std::filesystem::path const& name){
        //potentially enforce it exists in the file system, and enforce create is called for construction
        return Get(CrossPlatformPathHash(name));
    }

#ifdef EWE_IMGUI
    void Manager<RasterPackage>::Imgui(){
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

    bool Manager<RasterPackage>::WriteToFile(RasterPackage const& rt){
        //if it has the old record its invalid
        //if it doesnt have an instruction package, its special execution, aka hand coded, cant be written

        std::ofstream outFile{rt.name.c_str(), std::ios::binary};

        std::size_t temp_buffer = current_file_version;
        outFile.write(reinterpret_cast<char*>(&temp_buffer), sizeof(temp_buffer));
        

        temp_buffer = rt.objectPackages.size();
        outFile.write(reinterpret_cast<char*>(&temp_buffer), sizeof(temp_buffer));

        for(auto* pkg : rt.objectPackages){
            AssetHash hash_buffer = EWE::Global::assetManager->objPkg.GetHash(*pkg);
            WriteToInstPkgFile(*pkg, pkg->name);
            outFile.write(reinterpret_cast<char*>(&hash_buffer), sizeof(AssetHash));
        }
        return true;
    }

    RasterPackage& Manager<RasterPackage>::ReadFile(std::filesystem::path const& name){
        std::ifstream inFile{name, std::ios::binary};

        std::size_t temp_buffer;
        inFile.read(reinterpret_cast<char*>(&temp_buffer), sizeof(temp_buffer));
        EWE_ASSERT(temp_buffer == current_file_version, "needs support otherwise");
        
        TaskRasterConfig tempConfig;
        auto& ret = data_arena.AddElement(name.string(), *Global::logicalDevice, Global::stcManager->renderQueue, tempConfig, nullptr);
        association_container.push_back(GetHash(ret), &ret);

        inFile.read(reinterpret_cast<char*>(&temp_buffer), sizeof(temp_buffer));

        ret.objectPackages.reserve(temp_buffer);
        for(std::size_t i = 0; i < temp_buffer; i++){
            AssetHash hash_buffer;
            inFile.read(reinterpret_cast<char*>(&hash_buffer), sizeof(AssetHash));
            //i need to make sure this is also written to file, or it will become invalid
            ret.objectPackages.push_back(&Global::assetManager->objPkg.Get(hash_buffer));
        }

        return ret;
    }

} //namepsace Asset
} //namespace EWE