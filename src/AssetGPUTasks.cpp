#include "EWEngine/Assets/GPUTasks.h"
#include "EWEngine/Global.h"
#include "EightWinds/Command/ParamPointerChain.h"

#include <fstream>

namespace EWE{

namespace Asset{
    static constexpr std::size_t file_version = 0;
#define cast_write(data) outFile.write(reinterpret_cast<const char*>(&data), sizeof(data))
#define cast_read(data) inFile.read(reinterpret_cast<char*>(&data), sizeof(data))
    
    void WriteParamPointerChain(ParamPointerChain const& src, std::ofstream& outFile){
        const AssetHash base_hash = GetHash(*src.base);
        cast_write(base_hash);

        cast_write(src.package_iter);

        std::size_t size_buffer = src.pointer_into.size();
        cast_write(size_buffer);
        
        outFile.write(reinterpret_cast<const char*>(src.pointer_into.data()), sizeof(std::size_t) * size_buffer);
    }
    void ReadParamPointerChain(ParamPointerChain& dst, std::ifstream& inFile){
        AssetHash base_hash;
        cast_read(base_hash);

        cast_read(dst.package_iter);

        std::size_t size_buffer;
        cast_read(size_buffer);
        dst.pointer_into.resize(size_buffer);

        inFile.read(reinterpret_cast<char*>(dst.pointer_into.data()), sizeof(std::size_t) * size_buffer);
    }


    template<>
    bool WriteAssetToFile(GPUTask const& task, std::filesystem::path const& root_directory, std::filesystem::path const& path){        
        auto const combined_path = root_directory / path;
        std::ofstream outFile{combined_path, std::ios::binary};
        if(!outFile.is_open()){
            outFile.open(combined_path, std::ios::binary);
            if(!outFile.is_open()){
                Logger::Print("failed to write to file - %s / %s\n", root_directory.string().c_str(), path.string().c_str());
                return false;
            }
        }


        cast_write(file_version);


        AssetHash const record_hash = GetHash(*task.pkgRecord);
        cast_write(record_hash);

        std::size_t size_buffer = task.meta.resource_pointers.size();
        cast_write(size_buffer);

        for(auto const& ppc : task.meta.resource_pointers){
            if(ppc.base != nullptr){
                WriteParamPointerChain(ppc, outFile);
            }
            else{
                //is this possible?
                Logger::Print<Logger::Warning>("invalid config\n");
                return false;
            }
        }

        outFile.close();
        return true;
    }



    template<>
    bool LoadAssetFromFile(GPUTask* ptr_to_raw_mem, std::filesystem::path const& root_directory, std::filesystem::path const& path){
        const auto combined_path = root_directory / path;
        std::ifstream inFile{combined_path, std::ios::binary};
        if(!inFile.is_open()){
            if(!std::filesystem::exists(combined_path)){
                Logger::Print<Logger::Warning>("attempting to open file that doesn't eixst\n");
                return false;
            }
            else{
                inFile.open(combined_path, std::ios::binary);
                if(!inFile.is_open()){
                    Logger::Print<Logger::Warning>("failed to open file : %s / %s\n", root_directory.string().c_str(), path.string().c_str());
                    return false;
                }
            }
        }

        std::size_t version_buffer;
        cast_read(version_buffer);
        if(version_buffer != file_version){
            Logger::Print<Logger::Warning>("idk how to deal with htis yet\n");
            return false;
        }

        AssetHash base_pkg_hash;
        cast_read(base_pkg_hash);
        if(base_pkg_hash == INVALID_HASH){
            return false;
        }
        Command::PackageRecord* base_pkg = Global::assetManager->pkgRecord.Get(base_pkg_hash);
        
        auto& task = *std::construct_at(ptr_to_raw_mem, base_pkg->name, *Global::logicalDevice, *base_pkg, false);
        
        std::size_t size_buffer;
        cast_read(size_buffer);

        task.meta.resource_pointers.resize(size_buffer);
        for(auto& ppc : task.meta.resource_pointers){
            ReadParamPointerChain(ppc, inFile);
        }

        inFile.close();
        return true;
    }


#undef cast_read
#undef cast_write
} //namespace Asset


    bool PointerChainParentEqual(ParamPointerChain const& lhs, ParamPointerChain const& rhs){
        if(lhs.base != rhs.base){
            return false;
        }
        if(lhs.package_iter != rhs.package_iter){
            return false;
        }
        if(lhs.pointer_into.size() != (rhs.pointer_into.size() - 1)){
            return false;
        }
        for(std::size_t iter = 0; lhs.pointer_into.size(); iter++){
            if(lhs.pointer_into[iter] != rhs.pointer_into[iter]){
                return false;
            }
        }
        return true;
    }

    GPUTaskMeta_Helper::PushHelper::PushHelper(ParamPointerChain const& pointer_chain_to_push, std::span<Shader*> shaders)
    : pointer_chain{pointer_chain_to_push},
        push{MergePushRanges(shaders)}
    {
        buffer_active.resize(push.buffers.size(), false);
        texture_active.resize(push.textures.size(), false);
    }

    GPUTaskMeta_Helper::GPUTaskMeta_Helper(GPUTask& _task)
    : task{_task}
    {
        auto& pkg_record = *task.pkgRecord;

        for(std::size_t pkg_iter = 0; pkg_iter < pkg_record.packages.size(); pkg_iter++){
            auto& pkg = pkg_record.packages[pkg_iter];
            if(pkg->type == Command::InstructionPackage::Raster){
                auto& raster_pkg = *reinterpret_cast<RasterPackage*>(pkg);
                for(std::size_t obj_pkg_iter = 0; obj_pkg_iter < raster_pkg.objectPackages.size(); obj_pkg_iter++){
                    auto& obj_pkg = *raster_pkg.objectPackages[obj_pkg_iter];
                    std::size_t instruction_offset = 0;
                    for(auto& inst : obj_pkg.paramPool.instructions){
                        if(inst == Inst::Push){
                            ParamPointerChain pointer_to_push;
                            pointer_to_push.base = task.pkgRecord;
                            pointer_to_push.package_iter = pkg_iter;
                            pointer_to_push.pointer_into.push_back(obj_pkg_iter);
                            pointer_to_push.pointer_into.push_back(instruction_offset);

                            pushes.push_back(PushHelper{pointer_to_push, obj_pkg.payload.shaders});
                            auto& back_push = pushes.back();

                            //hash the pointer_to_push?
                            for(auto& existing : task.meta.resource_pointers){
                                if(PointerChainParentEqual(pointer_to_push, existing)){
                                    std::size_t back_iter = existing.pointer_into.back();
                                    if(back_iter < back_push.buffer_active.size()){
                                        back_push.buffer_active[back_iter] = true;
                                    }
                                    else{
                                        back_push.texture_active[back_iter - back_push.buffer_active.size()] = true;
                                    }
                                    break;
                                }
                            }

                        }
                        instruction_offset += Inst::GetParamSize(inst);
                    }
                }
            }
        }
    }

    //copying a bool is cheap, and prevents a lookup to the value
    void GPUTaskMeta_Helper::ToggleResource(bool value, ParamPointerChain const& chain_into, uint8_t res_index){
        ParamPointerChain emplaced_chain = chain_into;
        emplaced_chain.pointer_into.push_back(res_index);
        if(value){
            //adding
            task.meta.resource_pointers.push_back(emplaced_chain);
        }
        else{
            for(auto iter = task.meta.resource_pointers.begin(); iter != task.meta.resource_pointers.end(); iter++){
                if(*iter == emplaced_chain){
                    task.meta.resource_pointers.erase(iter);
                    return;
                }
            }
            EWE_UNREACHABLE;
        }
    }



} //namespace EWE


namespace std{
    std::size_t hash<EWE::ParamPointerChain>::operator()(EWE::ParamPointerChain const& chain) const noexcept {
        std::size_t seed = 0;
        
        EWE::hash_combine(seed, EWE::Asset::GetHash(*chain.base));
        EWE::hash_combine(seed, std::hash<std::size_t>{}(chain.package_iter));
        
        for (const auto& val : chain.pointer_into) {
            EWE::hash_combine(seed, std::hash<std::size_t>{}(val));
        }

        return seed;
    }
} //namespace std