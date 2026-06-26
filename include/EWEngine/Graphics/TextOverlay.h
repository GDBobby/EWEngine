#pragma once

#include "EWEngine/Graphics/TextPackage.h"
#include "EWEngine/Graphics/Font.h"

#include "EightWinds/VulkanHeader.h"

#include "EightWinds/LogicalDevice.h"
#include "EightWinds/Window.h"
#include "EightWinds/Buffer.h"
#include "EightWinds/Shader.h"
#include "EightWinds/Sampler.h"
#include "EightWinds/ImageView.h"

#include "EightWinds/Command/ObjectInstructionPackage.h"
#include "EightWinds/RenderGraph/RasterPackage.h"

#include "EightWinds/Data/KeyValueContainer.h"


#include <LAB/Vector.h>

#include <vector>

namespace EWE {
	//Font uses harfbuzz and freetype, i want to hide it from the rest of the program
	struct FontObject; //

	struct FontKey{
		std::string name; //no folders
		uint8_t size;

		bool operator==(FontKey const& other) const{
			return name == other.name && size == other.size;
		}
	};

	class TextOverlay {
	private:
		static constexpr uint32_t TEXTOVERLAY_MAX_CHAR_COUNT = 65536 / sizeof(Font::Vert);
	public:
		float scale; //not in use currently

		uint32_t numLetters;
		
		std::mutex mut;
		Hive<Font> fonts;
		KeyValueContainer<FontKey, Font*> font_map;

		Sampler& sampler;
//#pragma pack(push, 1)
		struct FontDraw{
			DeviceAddress buffer;
			TextureIndex index;
		};
//#pragma pack(pop)
		static constexpr uint16_t font_limit = 256;

		std::vector<FontDraw> font_draw_data{};
		std::vector<VkDrawIndirectCommand> indirect_cmd_data{};

		PerFlight<Buffer> draw_buffer;
		PerFlight<Buffer> indirect_buffer;
		//swap this to a mesh shader
		Command::ObjectPackage objPkg;
		InstructionPointer<ParamPack<Inst::Push>> pushPack;
		InstructionPointer<ParamPack<Inst::DrawIndirect>> indirectPack;

		Font& GetFont(FontKey const& font);
		Font& GetFont(std::filesystem::path const& name, uint8_t size);


		friend struct TextStruct;
		friend struct FontObject;

	public:
		bool visible = true;

		[[nodiscard]] explicit TextOverlay();
		~TextOverlay();
		
		float GetWidth(TextStruct const& ts);
		//float addText(std::string text, float x, float y, TextAlign align, float textScale = 1.f);
		void AddText(TextStruct const& textStruct, const float scaleX = 1.f);

		void AddDefaultText(LoopTimer const& timer);
		void BeginTextUpdate();
		void EndTextUpdate();

		bool RemoveFont(Font* font);

		void Record();
	};
}
