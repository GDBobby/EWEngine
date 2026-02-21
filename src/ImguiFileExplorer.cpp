#include "EWEngine/Tools/ImguiFileExplorer.h"

#include <array>


#ifdef EWE_IMGUI
namespace EWE{
	ExplorerContext::ExplorerContext(std::string_view path)
		: current_path{path}
	{
		memset(path_buffer, 0, path_length);
	}
	ExplorerContext::ExplorerContext(std::filesystem::path path)
		: current_path{path}
	{
		memset(path_buffer, 0, path_length);
	}
	
	
	void ExplorerContext::Imgui(){
		//ImGui::Text(current_path.string().c_str()); probably use htis to create whatever window is displaying this
		ImGui::InputText("dest path", path_buffer, path_length);
		ImGui::SameLine();
		if(ImGui::Button("Go")){
			if (std::filesystem::exists(path_buffer) && std::filesystem::is_directory(path_buffer)) {
				current_path = path_buffer;
			}
		}
		
		if(ImGui::Button("up folder")){
			current_path = current_path.parent_path();
		}
		if (ImGui::BeginTable("file explorer", 4, table_flags)){
			ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_NoHide);
			ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, TEXT_BASE_WIDTH * 60.0f);
			ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, TEXT_BASE_WIDTH * 70.0f);
			ImGui::TableSetupColumn("Date", ImGuiTableColumnFlags_WidthFixed, TEXT_BASE_WIDTH * 100.0f);
			ImGui::TableHeadersRow();
			
			for (const auto& entry : std::filesystem::directory_iterator(current_path)) {
				const auto& entry_path = entry.path();
				bool is_dir = entry.is_directory();
				const auto label = entry_path.filename().string();
				bool isSelected = false;
				
				ImGui::TableNextRow();
				
				ImGui::TableNextColumn();
				ImGui::PushID(label.c_str());
				if (ImGui::Selectable(label.c_str(), isSelected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowDoubleClick)) {
					if (ImGui::IsMouseDoubleClicked(0)) {
						//current_path = entry_path;
						//strncpy(path_buffer, current_path.string().c_str(), path_length);
						selected_file = entry_path;
						enabled = false;
						//ImGui::TextUnformatted(label.c_str());
					}
				}

				ImGui::TableNextColumn();
				if (!is_dir) {
					auto file_size = std::filesystem::file_size(entry_path);
					
					static constexpr auto size_names = std::to_array<std::string_view>({
						std::string_view{"bytes"},
						std::string_view{"KBs"},
						std::string_view{"MBs"},
						std::string_view{"GBs"}
					});
					std::size_t size_index = 0;
					while(file_size > (1024 * 10)){
						file_size /= 1024;
						size_index++;
					}
					const std::string format_string = std::string("%zu ") + size_names[size_index].data();
					ImGui::Text(format_string.c_str(), file_size);
				} else {
					ImGui::TextUnformatted("---");
				}

				ImGui::TableNextColumn();
				if (is_dir) {
					ImGui::TextUnformatted("Folder");
				}
				else {
					ImGui::TextUnformatted(entry_path.extension().string().c_str());
				}

				ImGui::TableNextColumn();
				auto ftime = std::filesystem::last_write_time(entry_path);
				ImGui::Text("Modified"); 
				ImGui::PopID();
			}
			ImGui::EndTable();
		}
	}
} //namespace EWE
#endif