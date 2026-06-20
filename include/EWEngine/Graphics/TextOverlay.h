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


#include <LAB/Vector.h>

#include <vector>

namespace EWE {
	//Font uses harfbuzz and freetype, i want to hide it from the rest of the program
	struct FontObject; //

	class TextOverlay {
	private:
		static constexpr uint32_t TEXTOVERLAY_MAX_CHAR_COUNT = 65536 / sizeof(Font::Vert);
	public:
		float scale;
		
		float framebuffer_width;
		float framebuffer_height;

		uint32_t numLetters;
		Hive<Font> fonts;
		Font* currentFont;
		Sampler& sampler;

		friend struct TextStruct;
		friend struct FontObject;
		void LoadConsolas24();

	public:
		bool visible = true;

		[[nodiscard]] explicit TextOverlay();
		~TextOverlay();

		void WindowResize();
		
		static float GetWidth(TextStruct const& ts);
		//float addText(std::string text, float x, float y, TextAlign align, float textScale = 1.f);
		static void AddText(TextStruct const& textStruct, const float scaleX = 1.f);

		static void StaticAddText(TextStruct textStruct);

		static void AddDefaultText(LoopTimer const& timer);
		static void BeginTextUpdate();
		static void EndTextUpdate();

		static Font* AddFont(std::filesystem::path const& name, int size);
		static bool SetCurrentFont(Font*);
		static Font* GetCurrentFont();
		static uint16_t GetFontCount();
		static std::filesystem::path const& GetCurrentFontName();

		static bool RemoveFont(Font* font);

		static void Record();
	};
}
