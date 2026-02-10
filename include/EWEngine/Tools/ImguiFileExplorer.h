#pragma once

#include "EWEngine/Preprocessor.h"

#ifdef EWE_IMGUI
#include <cstdint>
#include "imgui.h"

#include <filesystem>
#include <optional>
#include <vector>
#include <string>
#include <string_view>

namespace EWE{
	struct ExplorerContext{
		std::filesystem::path current_path{};

		//only for internal closing
		bool enabled = true;

		std::optional<std::filesystem::path> selected_file = std::nullopt;

		[[nodiscard]] explicit ExplorerContext(std::string_view path);
		[[nodiscard]] explicit ExplorerContext(std::filesystem::path path);
		
		static constexpr std::size_t path_length = 256;
		char path_buffer[path_length];
		
		ImGuiTableFlags table_flags = ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg | ImGuiTableFlags_NoBordersInBody;
		ImGuiTreeNodeFlags tree_node_flags_base = ImGuiTreeNodeFlags_SpanAllColumns | ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_DrawLinesFull;
		const float TEXT_BASE_WIDTH = 24.f;
		
		std::vector<std::string> acceptable_extensions{};
		
		void Imgui();
	};
}
#endif