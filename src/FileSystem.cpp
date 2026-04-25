#include "EWEngine/Assets/FileSystem.h"

#include "EWEngine/Assets/Base.h"


#if EWE_IMGUI
#include "imgui.h"
#endif

namespace EWE{
namespace Asset{ 
    FileSystem::FileSystem(std::filesystem::path const& _root_directory, std::span<const std::string_view> _acceptable_extensions)
    : root_directory{_root_directory},
        acceptable_extensions{}
    {
        for(auto& str : _acceptable_extensions){
            acceptable_extensions.emplace_back(str);
        }
        RefreshFiles();
    }

#if EWE_IMGUI
    void FileSystem::Imgui(){
        ImGui::Text("root : %s", root_directory.string().c_str());

        for(auto& kvp : hashed_path){
            ImGui::Text("%s", kvp.value.string().c_str());
        }

    }
#endif

    void FileSystem::RefreshFiles(){

        hashed_path.clear();

        if (!std::filesystem::exists(root_directory) || !std::filesystem::is_directory(root_directory)) {
            return; 
        }

        for (const auto& entry : std::filesystem::recursive_directory_iterator(root_directory)) {
            if (entry.is_regular_file()) {
                std::filesystem::path const& current_path = entry.path();
                std::string const& ext = current_path.extension().string();

                const bool match = std::any_of(acceptable_extensions.begin(), acceptable_extensions.end(),
                    [&ext](std::string const& acceptable) {
                        return ext == acceptable;
                    }
                );

                if (match) {
                    auto const& temp_path = std::filesystem::relative(entry.path(), root_directory);
                    auto hash = CrossPlatformPathHash(temp_path);
                    hashed_path.push_back(hash, temp_path);
                }
            }
        }
#if EWE_DEBUG
        //assert the hash and path are each unique, if it's an issue
#endif
    }
}//namespace Asset
} //namespace EWE