#include "EWEngine/Assets/Hash.h"
#include "EWEngine/Assets/Shaders.h"

#include "EWEngine/Imgui/Objects.h"
#include "EWEngine/Imgui/DragDrop.h"

namespace EWE{
namespace Asset{



    Manager<Shader>::Manager(std::filesystem::path root_directory)
    : files{root_directory, {".spv"}}
    {

    }
    bool Manager<Shader>::GetMeta(ShaderMeta& meta, std::filesystem::path const& file_path){
        auto const meta_path = files.root_directory / "meta" / file_path;
        if(!std::filesystem::exists(meta_path)){
            return false;
        }
        return meta.ReadFromFile(meta_path);
    }
    bool Manager<Shader>::GetMeta(ShaderMeta& meta, AssetHash hash){
        for(auto& kvp : files.hashed_path){
            if(kvp.key == hash){
                return GetMeta(meta, kvp.value);
            }
        }
        return false;
    }

    void Manager<Shader>::Destroy(AssetHash hash){
        for(auto iter = association_container.begin(); iter != association_container.end(); iter++){
            if(iter->key == hash){
                data_arena.DestroyElement(iter->value);
                association_container.erase(iter);
                return;
            }
        }
        EWE_UNREACHABLE;
    }
    void Manager<Shader>::Destroy(Shader& shader){
        for(auto iter = association_container.begin(); iter != association_container.end(); iter++){
            if(iter->value == &shader){
                data_arena.DestroyElement(iter->value);
                association_container.erase(iter);
                return;
            }
        }
        EWE_UNREACHABLE;
    }

    Shader& Manager<Shader>::Get(AssetHash hash){
        auto foundShader = association_container.find(hash);
        
        if(foundShader != association_container.end()){
            return *foundShader->value;
        }
        else{
            auto iter = std::find_if(files.hashed_path.begin(), files.hashed_path.end(), [&](const auto& p) {
                    return p.key == hash;
                }
            );
            if(iter != files.hashed_path.end()){
                const auto shader_asset_path = files.root_directory / iter->value;
                Shader& ret = data_arena.AddElement(*Global::logicalDevice, shader_asset_path);
                ret.filepath = iter->value;

                if(GetMeta(ret.meta, iter->value)){
                    if(ret.meta.buffer_written_to.Size() != ret.pushRange.buffers.size()){
                        Logger::Print<Logger::Warning>("incorrect meta buffer - %zu:%zu\n", ret.meta.buffer_written_to.Size(), ret.pushRange.buffers.size());
                        const auto lower_size = std::min(ret.meta.buffer_written_to.Size(), ret.pushRange.buffers.size());
                        ret.meta.buffer_written_to.ClearAndResize(lower_size);

                        //if the shader's buffer count is larger, the rest are assumed read only
                        for(std::size_t i = 0; i < lower_size; i++){ 
                            ret.meta.buffer_written_to[i] = ret.meta.buffer_written_to[i];
                        }

                    }
                    if(ret.meta.texture_written_to.Size() != ret.pushRange.textures.size()){
                        Logger::Print<Logger::Warning>("incorrect meta buffer - %zu:%zu\n", ret.meta.texture_written_to.Size(), ret.pushRange.textures.size());
                        const auto lower_size = std::min(ret.meta.texture_written_to.Size(), ret.pushRange.textures.size());
                        ret.meta.texture_written_to.ClearAndResize(lower_size);

                        //if the shader's buffer count is larger, the rest are assumed read only
                        for(std::size_t i = 0; i < lower_size; i++){ 
                            ret.meta.texture_written_to[i] = ret.meta.texture_written_to[i];
                        }
                    }
                }
                else{
                    ret.meta.buffer_written_to.ClearAndResize(ret.pushRange.buffers.size());
                    ret.meta.texture_written_to.ClearAndResize(ret.pushRange.textures.size());
                    for(auto& buf : ret.meta.buffer_written_to){
                        buf = false;
                    }
                    for(auto& img : ret.meta.texture_written_to){
                        img = false;
                    }
                }
                association_container.push_back(GetHash(ret), &ret);

                return ret;
            }
            
        }

        Logger::Print<Logger::Error>("returning nullptr from GetShader : %s - %zu\n", files.root_directory.string().c_str(), hash);
        EWE_UNREACHABLE;
    }
    Shader& Manager<Shader>::Get(std::filesystem::path const& file_name){
        return Get(CrossPlatformPathHash(file_name));
    }

#ifdef EWE_IMGUI
    void Manager<Shader>::Imgui(){
        ImGui::Text("root : %s", files.root_directory.string().c_str());
        if(ImGui::Button("refresh files")){
            files.RefreshFiles();
        }

        for(auto& file : files.hashed_path){
            auto foundShader = association_container.find(file.key);
            if(foundShader != association_container.end()) {

                bool tree_open = ImGui::TreeNode(file.value.string().c_str());
                DragDropPtr::Source(*foundShader->value);

                if(tree_open) {
                    auto& shader = *foundShader->value;
                    ImguiExtension::Imgui(shader);
                    ImGui::TreePop();
                }
            }
            else {
                ImGui::PushID(69420);
                if(ImGui::Button(file.value.string().c_str())){
                    Get(file.value.string().c_str());
                }
                ImGui::PopID();
                //ImGui::Text("%s", file.string().c_str());
            }
        }
    }
#endif
} //namespace Asset
} //namespace EWE