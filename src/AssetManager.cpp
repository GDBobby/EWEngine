#include "EWEngine/Assets/Manager.h"

#include "EWEngine/Global.h"

#include "EightWinds/Reflect/Reflect.h"
#include "imgui.h"

namespace EWE{
namespace Asset{
    /*
        Manager<Buffer> buffer;
        Manager<GPUTask> gpuTask;
        Manager<Image> image;
        Manager<ImageView> imageView;
        Manager<Command::InstructionPackage> instPkg;
        Manager<Command::PackageRecord> pkgRecord;
        Manager<RasterPackage> rasterTask;
        Manager<Sampler> sampler;
        Manager<DescriptorImageInfo> dii;
        Manager<SubmissionTask> subTask;
    */
    Manager<AssetHash>::Manager(std::filesystem::path const& asset_directory)
    : root_directory{asset_directory},
        buffer{asset_directory},
        //gpuTask{asset_directory},
        image{asset_directory},
        imageView{asset_directory, image},
        instPkg{asset_directory},
        objPkg{asset_directory},
        pkgRecord{asset_directory},
        rasterTask{asset_directory},
        sampler{},
        dii{asset_directory, sampler, imageView},
        subTask{asset_directory},
        shader{asset_directory / "shaders/"}

    {
    }

#ifdef EWE_IMGUI
    void Manager<AssetHash>::Imgui(){
        if(ImGui::TreeNode("shaders")){
            shader.Imgui();
            ImGui::TreePop();
        }
        if(ImGui::TreeNode("dii")){
            dii.Imgui();
            ImGui::TreePop();
        }
        if(ImGui::TreeNode("images")){
            image.Imgui();
            ImGui::TreePop();
        }
        if(ImGui::TreeNode("samplers")){
            sampler.Imgui();
            ImGui::TreePop();
        }
        if(ImGui::TreeNode("buffers")){
            buffer.Imgui();
            ImGui::TreePop();
        }
        if(ImGui::TreeNode("inst pkgs")){
            instPkg.Imgui();
            ImGui::TreePop();
        }
        if(ImGui::TreeNode("pkg records")){
            pkgRecord.Imgui();
            ImGui::TreePop();
        }
        if(ImGui::TreeNode("raster tasks")){
            rasterTask.Imgui();
            ImGui::TreePop();
        }
        if(ImGui::TreeNode("submission tasks")){
            subTask.Imgui();
            ImGui::TreePop();
        }
    }
#endif

    template<typename T>
    std::string_view GetTypeName_Please(){
        return Reflect::GetName<^^T>();
    }

    template<typename T>
    bool CheckExtension(T& container, std::filesystem::path const& dropped_ext, std::filesystem::path const& path){
        if constexpr(requires{container.files;}){
            auto const& extensions = container.files.acceptable_extensions;
                for(auto const& ext : extensions){
                    if(dropped_ext == ext){
                        auto& temp = container.Get(path);
                        /*
                            if constexpr(std::meta::has_template_arguments(^^T)){
                                static constexpr auto template_args = Reflect::GetMetaSpan<^^T, std::meta::template_arguments_of, false, std::meta::has_template_arguments(^^T)>();

                                Logger::Print("template arg : %s", Reflect::GetName(template_args.at(0)).data());
                            }
                        */

                        if(ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceExtern)){
                            std::size_t reint_ptr = reinterpret_cast<std::size_t>(&temp);


                            auto temp_name = GetTypeName_Please<std::decay_t<decltype(temp)>>();

                            ImGui::SetDragDropPayload(temp_name.data(), &reint_ptr, sizeof(std::size_t));
                        
                            ImGui::EndDragDropSource();
                        }
                        
                        return true;
                    }
                }
        }
        return false;
    }

    void Manager<AssetHash>::ApplyGLFWDrops(){

        static constexpr auto members = std::define_static_array(std::meta::nonstatic_data_members_of(^^Manager<AssetHash>, std::meta::access_context::current()));

        for(auto const& proximate : glfw_drops){
            auto dropped_ext = proximate.extension();


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
            template for(constexpr auto mem : members){
                if(CheckExtension(this->[:mem:], dropped_ext, proximate)){
                    glfw_drops.clear();
                    return;
                }
            }
#pragma GCC diagnostic pop
        
        }
        glfw_drops.clear();
    }

    bool CheckWithinRootDir(std::filesystem::path const& proximate){
        return (!proximate.empty()) && (proximate.native()[0] != '.') && (proximate.string().find("..") == std::string::npos);
    }

    void Manager<AssetHash>::DropCallback(std::filesystem::path const& path){
        Logger::Print("droppe a file : %s\n", path.string().c_str());

        const auto proximate = std::filesystem::proximate(path, root_directory);
        if(!CheckWithinRootDir(proximate)){
            Logger::Print<Logger::Warning>("attempted to drop a file from outside the asset directory\n");
            return;
        }

        if(path.has_extension()){
            glfw_drops.push_back(proximate);
        }

        Logger::Print("failed to find a match\n");
    }

}//namespace Asset


    void GLFW_Drop_Callback(GLFWwindow* window, int count, const char** paths){

        for(int i = 0; i < count; i++){
            std::filesystem::path drop_path{paths[i]};
            Global::assetManager->DropCallback(drop_path);
        }
    }


} //namespace EWE