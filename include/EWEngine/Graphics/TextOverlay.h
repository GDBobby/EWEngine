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
		float scale;
		
		float framebuffer_width;
		float framebuffer_height;

		uint32_t numLetters;
		
		std::mutex mut;
		Hive<Font> fonts;
		KeyValueContainer<FontKey, Font*> font_map;
 
		struct FontDescriptor{
			DeviceAddress buffer;
			TextureIndex index;
		};
		static constexpr uint16_t font_limit = 256;
		PerFlight<Buffer> draw_buffer;
		//swap this to a mesh shader
		Command::ObjectPackage objPackage;
		InstructionPointer<ParamPack<Inst::Push>> pushPack;
		InstructionPointer<ParamPack<Inst::DrawIndirect>> indirectPack;

		Font& GetFont(FontKey const& font);
		Font& GetFont(std::filesystem::path const& name, uint8_t size);

		Sampler& sampler;

		friend struct TextStruct;
		friend struct FontObject;

	public:
		bool visible = true;

		[[nodiscard]] explicit TextOverlay();
		~TextOverlay();

		void WindowResize();
		
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
