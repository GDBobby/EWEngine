#include "EWEngine/Assets/Shaders.h"


namespace EWE{
namespace Asset{



    Manager<Shader>::Manager(LogicalDevice& logicalDevice, std::filesystem::path root_directory)
    : logicalDevice{logicalDevice},
        files{root_directory, {".spv"}}
    {

    }

    Shader* Manager<Shader>::Get(std::string_view file_name){

        auto foundShader = shaders.find(file_name);
        
        if(foundShader != shaders.end()){
            return foundShader->second;
        }
        else{
            auto iter = std::find_if(files.hashed_path.begin(), files.hashed_path.end(), [&](const auto& p) {
                    return p.value == file_name;
                }
            );

            std::filesystem::path full_file_name = files.root_directory / file_name.data();

            if(iter != files.hashed_path.end()){
                auto emp_back = shaders.try_emplace(file_name, new Shader(logicalDevice, full_file_name.string().c_str()));
                EWE_ASSERT(emp_back.second);
                return emp_back.first->second;
            }
        }

        Logger::Print<Logger::Error>("returning nullptr from GetShader : %s\n", file_name.data());
        return nullptr;
    }

#ifdef EWE_IMGUI
    void Manager<Shader>::Imgui(){
        ImGui::Text("root : %s", files.root_directory.string().c_str());

        for(auto& file : files.hashed_path){
            auto foundShader = shaders.find(file.value);
            if(foundShader != shaders.end()) {

                bool tree_open = ImGui::TreeNode(file.value.string().c_str());
                if (ImGui::BeginDragDropSource()) {

                    ImGui::SetDragDropPayload("SHADER", file.value.string().c_str(), file.value.string().size() + 1);
                    ImGui::PushID(6942068);
                    ImGui::Text("%s",file.value.string().c_str());
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