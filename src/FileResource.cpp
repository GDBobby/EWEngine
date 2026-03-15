#include "EWEngine/Tools/FileResource.h"

#include "EightWinds/Shader.h"


#if EWE_IMGUI
#include "imgui.h"
#endif

namespace EWE{
    
    FileSystem::FileSystem(std::filesystem::path root_directory, std::vector<std::string_view> const& acceptable_extensions)
    : root_directory{root_directory}
    {
        if (!std::filesystem::exists(root_directory) || !std::filesystem::is_directory(root_directory)) {
            return; 
        }

        for (const auto& entry : std::filesystem::recursive_directory_iterator(root_directory)) {
            if (entry.is_regular_file()) {
                std::filesystem::path const& current_path = entry.path();
                std::string ext = current_path.extension().string();

                const bool match = std::any_of(acceptable_extensions.begin(), acceptable_extensions.end(),
                    [&ext](std::string_view acceptable) {
                        return ext == acceptable;
                    }
                );

                if (match) {
                    files.push_back(std::filesystem::relative(entry.path(), root_directory));
                }
            }
        }
    }

    ShaderFileSystem::ShaderFileSystem(LogicalDevice& logicalDevice, std::filesystem::path root_directory)
    : logicalDevice{logicalDevice},
        files{root_directory, {".spv"}}
    {

    }

    Shader* ShaderFileSystem::GetShader(std::string_view file_name){

        auto foundShader = shaders.find(file_name);
        
        if(foundShader != shaders.end()){
            return foundShader->second;
        }
        else{
            auto iter = std::find_if(files.files.begin(), files.files.end(), [&](const std::filesystem::path& p) {
                    return p == file_name;
                }
            );

            std::filesystem::path full_file_name = files.root_directory / file_name.data();

            if(iter != files.files.end()){
                auto emp_back = shaders.try_emplace(file_name, new Shader(logicalDevice, full_file_name.string().c_str()));
                EWE_ASSERT(emp_back.second);
                return emp_back.first->second;
            }
        }

        printf("returning nullptr from GetShader : %s\n", file_name.data());
        return nullptr;
    }

#if EWE_IMGUI
    void FileSystem::Imgui(){
        ImGui::Text("root : %s", root_directory.string().c_str());

        for(auto& file : files){
            ImGui::Text("%s", file.string().c_str());
        }

    }

    void ShaderFileSystem::Imgui(){
        ImGui::Text("root : %s", files.root_directory.string().c_str());

        for(auto& file : files.files){
            auto foundShader = shaders.find(file);
            if(foundShader != shaders.end()) {

                bool tree_open = ImGui::TreeNode(file.string().c_str());
                if (ImGui::BeginDragDropSource()) {

                    ImGui::SetDragDropPayload("SHADER", file.string().c_str(), file.string().size() + 1);
                    ImGui::PushID(6942068);
                    ImGui::Text("Drop handle");
                    ImGui::PopID();
                    ImGui::EndDragDropSource();
                }

                if(tree_open) {
                    auto& shader = *foundShader->second;
                    ImGui::Text("address : %zu", reinterpret_cast<std::size_t>(&shader));
                    for(auto& str : shader.BDA_data){
                        if(ImGui::BeginTable("this name doesnt matter", 4)){
                            ImGui::TableSetupColumn(str.name.c_str());
                            ImGui::TableSetupColumn("member name");
                            ImGui::TableSetupColumn("size");
                            ImGui::TableSetupColumn("offset");

                            ImGui::TableHeadersRow();
                            for(auto& data : str.members){
                                ImGui::TableNextColumn();
                                ImGui::TableNextColumn();
                                ImGui::Text("%s", data.name.c_str());
                                ImGui::TableNextColumn();
                                ImGui::Text("%u", data.size);
                                ImGui::TableNextColumn();
                                ImGui::Text("%u", data.offset);
                                ImGui::TableNextRow();
                            }

                            ImGui::EndTable();
                        }
                    }
                    ImGui::TreePop();
                }
            }
            else {
                ImGui::PushID(69420);
                if(ImGui::Button(file.string().c_str())){
                    GetShader(file.string().c_str());
                }
                ImGui::PopID();
                //ImGui::Text("%s", file.string().c_str());
            }
        }
    }
#endif
}