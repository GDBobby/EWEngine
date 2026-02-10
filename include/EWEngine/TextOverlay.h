#pragma once

#include "EightWinds/VulkanHeader.h"

#include "EightWinds/LogicalDevice.h"
#include "EightWinds/Buffer.h"
#include "EightWinds/Shader.h"
#include "EightWinds/RenderGraph/Command/Record.h"
#include "EightWinds/RenderGraph/RasterTask.h"
#include "EightWinds/Sampler.h"

#include "EWEngine/Global.h"

#include <LAB/Vector.h>

#include <iostream>
#include <vector>
#include <unordered_map>

#define ENABLE_VALIDATION false 

#define DEFAULT_WIDTH 1920.f
#define DEFAULT_HEIGHT 1080.f

namespace EWE {

	enum TextAlign : uint8_t { TA_left, TA_center, TA_right };
	struct TextStruct {
		std::string string;
		float x{ 0.f };
		float y{ 0.f };
		uint8_t align{ TA_left };
		float scale{ 1.f };
		TextStruct() {}
		TextStruct(std::string string, float x, float y, uint8_t align, float scale)
			: string{ string }, x{ x }, y{ y }, align{ align }, scale{ scale }
		{}
		TextStruct(std::string string, float x, float y, TextAlign align, float scale) 
			: string{ string }, x{ x }, y{ y }, align{ static_cast<uint8_t>(align) }, scale{ scale }
		{}
		uint16_t GetSelectionIndex(double xpos);
		float GetWidth();
	};

	struct Font {
		~Font();
		struct CharacterData {
			struct Vert {
				float x;
				float y;
				float u;
				float v;
			};
			//top left and bottom right, can be mixed for top right and bottom left
			Vert vertices[2];
		};
		Font(std::unordered_map<wchar_t, EWE::Font::CharacterData>& vertData, std::unordered_map<wchar_t, float>& advanceData, uint16_t width, uint16_t height, void* imgdata);
		Font& operator=(Font& other) = delete;
		Font& operator=(Font&& other) = delete;
		Font(Font&& other) noexcept;
		Font(Font& other) = delete;

		float GetCharWidth(wchar_t c, const float charW) const;
		CharacterData::Vert const* GetVertData(const wchar_t c) const;
		float GetStringWidth(std::string const& str, const float charW) const;

		const uint16_t width;
		const uint16_t height;

		const std::string name;

		std::size_t drawnLetterCount = 0;
		std::unordered_map<wchar_t, CharacterData> vertData{};
		std::unordered_map<wchar_t, float> advanceData;

		Sampler sampler;

		Image image;
		ImageView image_view;

		PerFlight<Buffer> buffers;

		Font::CharacterData::Vert* mapped = nullptr;

		VkImageMemoryBarrier2 GenerateBarrier(VkImageLayout dstLayout);
	};


	class TextOverlay {
	private:
		static constexpr uint32_t TEXTOVERLAY_MAX_CHAR_COUNT = 65536 / sizeof(lab::vec4);

		Shader vertShader;
		Shader fragShader;
		PipeLayout pipeLayout;

		uint32_t numLetters;

		std::vector<Font*> fonts{};
		int16_t currentFont;

		friend struct TextStruct;
		friend struct Font;
		void LoadConsolas24();

	public:

		bool visible = true;
		float scale;
		
		float framebuffer_width;
		float framebuffer_height;

		ObjectRasterData objectConfig;

		[[nodiscard]] explicit TextOverlay(float framebufferwidth, float framebufferheight);
		~TextOverlay();
		
		static float GetWidth(std::string const& text, float textScale = 1.f);
		//float addText(std::string text, float x, float y, TextAlign align, float textScale = 1.f);
		static void AddText(TextStruct const& textStruct, const float scaleX = 1.f);

		static void StaticAddText(TextStruct textStruct);

		static void Draw();
		static void AddDefaultText(double time, double peakTime, double averageTime, double highTime);
		static void BeginTextUpdate();
		static void EndTextUpdate();

		static bool SetCurrentFont(uint16_t);
		static int16_t GetCurrentFont();
		static uint16_t GetFontCount();
		static std::string const& GetCurrentFontName();

		static int AddFont(Font* font);
		static bool RemoveFont(uint16_t fontIndex);


		void WindowResize() {
			framebuffer_width = Global::window->screenDimensions.width;
			framebuffer_height = Global::window->screenDimensions.height;
			scale = framebuffer_width / DEFAULT_WIDTH;
		}
		
		VertexIndirectCountDrawData indirect_vert_raw;


		void Record(RasterTask& task){
			objectConfig.config.SetDefaults();

			task.AddDraw(objectConfig, indirect_vert_raw);
		}
	};
}
