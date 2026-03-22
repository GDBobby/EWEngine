#include "EWEngine/Tools/ImguiFileExplorer.h"
#include "imgui.h"

#include <array>
#include <LAB/Vector.h>
#include <filesystem>


#ifdef EWE_IMGUI
namespace EWE{
	
	auto AlphanumericFilter = [](ImGuiInputTextCallbackData* data) -> int {
		if (isalnum((unsigned char)data->EventChar)) {
			return 0;
		}
		return 1;
	};
	auto FileFilter = [](ImGuiInputTextCallbackData* data) -> int {
		if (isalnum((unsigned char)data->EventChar)) {
			return 0;
		}
		switch(data->EventChar){
			case '.': return 0;
			case '_': return 0;
			default: return 1;
		}
		return 1;
	};



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

	std::filesystem::path ExplorerContext::CreateFullFilePath(std::string_view extension){

		std::filesystem::path ret{path_buffer};
		if(selected_file.has_value()){
			ret /= *selected_file;
		}
		else{
			ret /= "untitled";
		}
		return ret / extension;
	}

	bool ExplorerContext::ProcessDirectoryEntry(std::filesystem::path const& entry_path, bool is_selected, bool is_dir){

		const auto label = entry_path.filename().string();
		
		ImGui::TableNextRow();
		
		ImGui::TableNextColumn();
		ImGui::PushID(label.c_str());

		const bool was_selected = ImGui::Selectable(label.c_str(), is_selected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowDoubleClick);

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

		if (was_selected) {
			if (ImGui::IsMouseDoubleClicked(0)) {
				//current_path = entry_path;
				//strncpy(path_buffer, current_path.string().c_str(), path_length);
				if(is_dir){
					current_path = entry_path;
					selection_index = -1;
				}
				else{
					selected_file = entry_path;
					enabled = false;
				}
				//ImGui::TextUnformatted(label.c_str());
			}
		}

		return was_selected;
	}

	bool ExplorerContext::AcceptableEntry(std::filesystem::path const& entry, bool is_dir) const noexcept{
		if(is_dir){
			return true;
		}
		if(acceptable_extensions.size() == 0){
			return true;
		}
		if(acceptable_extensions.size() == 1 && 
			(
				(acceptable_extensions[0] == ".*")
				|| (acceptable_extensions[0] == "*")
			)
		){
			return true;
		}
		auto const& ext = entry.extension();
		for(auto const& acc : acceptable_extensions){
			if(ext == acc){
				return true;
			}
		}
		return false;
	}
	
	void ExplorerContext::Imgui(){
		std::string ext_enumerated = std::string("acceptable extensions [") + std::to_string(acceptable_extensions.size()) + "] : "; 
		for(uint32_t i = 0; i < acceptable_extensions.size(); i++){
			ext_enumerated += acceptable_extensions[i];
			if(i != (acceptable_extensions.size() - 1)){
				ext_enumerated += ", ";
			}
		}
		ImGui::Text(ext_enumerated.c_str());
		//ImGui::Text(current_path.string().c_str()); probably use htis to create whatever window is displaying this
		ImGui::InputText("dest path", path_buffer, path_length);
		ImGui::SameLine();
		if(ImGui::Button("Go")){
			if (std::filesystem::exists(path_buffer) && std::filesystem::is_directory(path_buffer)) {
				current_path = path_buffer;
			}
		}

		if(state != State::Load){
			ImGui::PushID(0);
			if (ImGui::Button("create folder")){

				ImGui::OpenPopup("file_create_popup");
			}
			if(ImGui::BeginPopup("file_create_popup")){

				ImGui::InputText("folder name", file_create_buf, 64, ImGuiInputTextFlags_CallbackCharFilter, AlphanumericFilter);
				ImGui::PushID(696969);
				if(ImGui::Button("Create")){
					if(file_create_buf[0] != '\0'){
						std::filesystem::create_directory(file_create_buf);
						ImGui::CloseCurrentPopup();
					}
				}
				ImGui::PopID();
				ImGui::EndPopup();
			}

			ImGui::PopID();

			ImGui::SameLine();
		}
		
		if(ImGui::Button("up folder")){
			current_path = current_path.parent_path();
		}
		if(selected_file.has_value()){
			ImGui::Text("selected_file : %s", selected_file.value().string().c_str());
		}

		ImGui::Separator();

		std::optional<std::filesystem::path> currently_selected_folder{};

		if(std::filesystem::exists(current_path)){
			static lab::ivec4 temp_widths{100};
			//ImGui::DragInt4("widths", &temp_widths.x, 1.f, 0.f, 200.f);

			if (ImGui::BeginTable("file explorer", 4, table_flags)){
				ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_NoHide | ImGuiTableColumnFlags_WidthFixed, TEXT_BASE_WIDTH * temp_widths.x);
				ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, TEXT_BASE_WIDTH * temp_widths.y);
				ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, TEXT_BASE_WIDTH * temp_widths.z);
				ImGui::TableSetupColumn("Date", ImGuiTableColumnFlags_WidthFixed, TEXT_BASE_WIDTH * temp_widths.w);
				ImGui::TableHeadersRow();

				
				int current_index = 0;
				for (const auto& entry : std::filesystem::directory_iterator(current_path)) {
					bool isSelected = current_index == selection_index;
					bool is_dir = entry.is_directory();
					auto const& entry_path = entry.path();

					if(AcceptableEntry(entry_path, is_dir)){
						const bool was_selected = ProcessDirectoryEntry(entry_path, isSelected, is_dir);
						if(was_selected){
							selection_index = current_index;
						}
						if(selection_index == current_index && is_dir){

							currently_selected_folder = entry;
						}
						current_index++;
					}
				}
				ImGui::EndTable();
			}

			if(state == State::Save){

				if(ImGui::Button("Save in Current Folder")){
					save_path = current_path;
				}
				if(currently_selected_folder.has_value()){
					ImGui::SameLine();
					if(ImGui::Button("Save in Selected Folder")){
						save_path = currently_selected_folder;
					}
				}

				if(!popup_opened && save_path.has_value()){
					ImGui::OpenPopup("save file popup");
					popup_opened = true;
					//selected_file = save_path;
				}
				if(ImGui::BeginPopup("save file popup")){
					static char file_save_buf[64] = "";
					ImGui::PushID(696970);
					ImGui::InputText("file name", file_save_buf, 64, ImGuiInputTextFlags_CallbackCharFilter, FileFilter);
					if(file_save_buf[0] != '\0'){
						if(ImGui::Button("Create")){
							//std::filesystem::create_directory(file_save_buf);
							selected_file = save_path.value() / file_save_buf;
							save_path.reset();
							ImGui::CloseCurrentPopup();
							popup_opened = false;
						}
					}
					ImGui::PopID();
					ImGui::EndPopup();
				}
			}
		}
	}
} //namespace EWE
#endif