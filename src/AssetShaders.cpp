#include "EWEngine/Assets/Base.h"
#include "EWEngine/Assets/Hash.h"
#include "EWEngine/Assets/Shaders.h"

#include "EWEngine/Global.h"
#include "EWEngine/Imgui/Objects.h"
#include "EWEngine/Imgui/DragDrop.h"

namespace EWE{
namespace Asset{


    template<>
    bool ReadMetaFile(Shader& shader, std::filesystem::path const& root_directory, std::filesystem::path const& file_path){
        auto const meta_path = root_directory / "meta" / file_path;
        if(!std::filesystem::exists(meta_path)){
            return false;
        }
        return shader.meta.ReadFromFile(meta_path);
    }

    template<>
    bool LoadAssetFromFile(Shader* ptr_to_raw_mem, std::filesystem::path const& root_directory, const std::filesystem::path &path){
        Shader& ret = *std::construct_at(ptr_to_raw_mem, *Global::logicalDevice, root_directory / path);
        ret.name = path;

        if(ReadMetaFile(ret, root_directory, path)){
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

        return true;
    }
} //namespace Asset
} //namespace EWE