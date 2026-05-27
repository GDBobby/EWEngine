#pragma once

#ifdef EWE_IMGUI
#include "imgui.h"

#include <filesystem>
#include <optional>
#include <vector>
#include <string>
#include <string_view>

namespace EWE{

	struct ImguiInputFilters{
		static int Alphanumeric(ImGuiInputTextCallbackData* data) {
			if (isalnum((unsigned char)data->EventChar)) {
				return 0;
			}
			return 1;
		};
		static int File(ImGuiInputTextCallbackData* data) {
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
	};

	struct ExplorerContext{

		enum class State{
			Load,
			Save,
			Exploring
		};
		State state = State::Exploring;

		std::filesystem::path current_path{};

		//only for internal closing
		bool enabled = true;

		std::optional<std::filesystem::path> selected_file = std::nullopt;

		[[nodiscard]] explicit ExplorerContext(std::string_view path);
		[[nodiscard]] explicit ExplorerContext(std::filesystem::path path);
		
		static constexpr std::size_t path_length = 128;
		char path_buffer[path_length];
		
		ImGuiTableFlags table_flags = ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg | ImGuiTableFlags_NoBordersInBody;
		ImGuiTreeNodeFlags tree_node_flags_base = ImGuiTreeNodeFlags_SpanAllColumns | ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_DrawLinesFull;
		const float TEXT_BASE_WIDTH = 24.f;
		
		std::vector<std::string> acceptable_extensions{};

		std::filesystem::path CreateFullFilePath(std::string_view extension);
		
		void Imgui();

		bool AcceptableEntry(std::filesystem::path const& entry, bool is_dir) const noexcept;
		bool ProcessDirectoryEntry(std::filesystem::path const& entry, bool isSelected, bool is_dir);

		char file_save_buf[path_length] = "";
		char file_create_buf[path_length] = "";
	private:
		std::optional<std::filesystem::path> save_path{};

		bool want_save_open = false;
		int selection_index = -1;
		bool popup_opened = false;
	};

	/*
	struct SaveExplorer{
		ExplorerContext context;

		[[nodiscard]] explicit SaveExplorer(std::string_view path) : context{path} {}
		[[nodiscard]] explicit SaveExplorer(std::filesystem::path path) : context{path} {}

		std::string extension;

		void SaveFile(std::filesystem::path path);
		void SaveFile(std::string_view path){
			std::filesystem::path fwd_path{path};
			SaveFile(fwd_path);
		}

		void Imgui(){
			context.Imgui();
		}
	};
	*/
}
#endif