
#include "../include/EWEngine/Graphics/TextOverlay.h"

#include "EWEngine/EWEngine.h"

//#define STB_TRUETYPE_IMPLEMENTATION  // force following include to generate implementation
//#include "EWEngine/Fonts/stb_font_consolas_24_latin1.inl"
#include "EightWinds/Backend/StagingBuffer.h"
#include "EightWinds/RenderGraph/Resources.h"


#include <hb.h>
#include <hb-ft.h>


namespace EWE {

	TextOverlay::TextOverlay()
	: scale{ engine->window.screenDimensions.width / DEFAULT_WIDTH<float> },
		sampler{
			Global::assetManager->sampler.Get(
				VkSamplerCreateInfo{
					.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
					.pNext = nullptr,
					.flags = 0,
					.magFilter = VK_FILTER_NEAREST,
					.minFilter = VK_FILTER_NEAREST,
					.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
					.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
					.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
					.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
					.mipLodBias = 0.f,
					.anisotropyEnable = VK_FALSE,
					.maxAnisotropy = 1.f,
					.compareEnable = VK_FALSE,
					.compareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
					.minLod = 0.f,
					.maxLod = 1.f,
					.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
					.unnormalizedCoordinates = VK_FALSE
				}
			)
		},
		draw_buffer{Global::assetManager->buffer.ConstructInto(
			engine->logicalDevice,
			sizeof(FontDescriptor), font_limit,
            VmaAllocationCreateInfo{
                .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
                .usage = VMA_MEMORY_USAGE_AUTO,
                .requiredFlags = 0,
                .preferredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                .memoryTypeBits = 0,
                .pool = VK_NULL_HANDLE,
                .pUserData = nullptr,
                .priority = 1.f
            }
		)},
		objPackage{"textoverlay"}
	{

		/*
		const FontKey fontKey{
			.name{"Consolas.ttf"},
			.size{16}
		};
		GetFont(fontKey);
		*/
		objPackage.paramPool.PushBack(Inst::Push);
		objPackage.paramPool.PushBcck(Inst::DrawIndirect);

		pushPack = *objPackage.paramPool.param_data[0].CastTo<ParamPack<Inst::Push>>();
		indirectPack = *objPackage.paramPool.param_data[1].CastTo<ParamPack<Inst::Indirect>>();
	}


	TextOverlay::~TextOverlay() {}

	Font& TextOverlay::GetFont(FontKey const& fontKey){
		auto found = font_map.find(fontKey);
		if(found != font_map.end()){
			return *found->value;
		}
		else{
			mut.lock();
			Font& ret = fonts.AddElement(fontKey.name, fontKey.size, sampler);
			font_map.push_back(fontKey, &ret);
			mut.unlock();
			return ret;
		}
	}
	Font& TextOverlay::GetFont(std::filesystem::path const& name, uint8_t pxSize) {
		return GetFont(
			FontKey{
				name,
				pxSize
			}
		);
	}

	void TextOverlay::AddDefaultText(LoopTimer const& timer) {
		/*
		AddText(
			TextStruct{
				engine->logicalDevice.physicalDevice.name, 
				lab::vec3{0.f, framebuffer_height - (20.f * scale), 0.1f}, 
				TextAlign::left, 1.f,
				GetFont(FontKey{"Consolas.ttf", 16})
			}
		);
		//const float lastFPS = LoopTimer::ToMilliseconds(timer.delta);
		const float averageFPS = LoopTimer::ToMilliseconds(timer.last_average);
		std::string buffer_string = std::format("frame time: {:.2f} ms)", LoopTimer::ToMilliseconds(timer.delta);
		AddText(TextStruct{ buffer_string, 0.f, framebuffer_height - (40.f * scale), TextAlign::left, 1.f });
		buffer_string = std::format("average FPS: {}", averageFPS);
		AddText(TextStruct{ buffer_string, 0.f, framebuffer_height - (60.f * scale), TextAlign::left, 1.f });
		buffer_string = std::format("peak: {:.2f} ms ~ average: {:.2f} ms ~ high: {:.2f} ms", peakTime * 1000, averageTime * 1000, highTime * 1000);
		AddText(TextStruct{ buffer_string, 0.f, framebuffer_height - (80.f * scale), TextAlign::left, 1.f });
		SetCurrentFont(previousFont);
		*/
	}

	float TextOverlay::GetWidth(TextStruct const& ts) {
		return ts.font.GetStringWidth(ts);
	}

	void TextOverlay::AddText(TextStruct const& textStruct, const float scaleX) {
		textStruct.font.AddText(textStruct);
	}

	void TextOverlay::BeginTextUpdate() {
		for (auto& font : fonts) {
			font.char_instance_count = 0;
			font.string_debugger[engine->frameIndex].clear();
		}
		numLetters = 0;
	}

	void TextOverlay::EndTextUpdate() {
		for (auto& font : fonts) {
			auto& font_buffer = font.buffer[engine->frameIndex];
			font.ifPack.GetRef(engine->frameIndex).enabled = font.char_instance_count > 0;
			if (font_buffer.GetMapped() != nullptr) {
				font_buffer.Flush();
				font_buffer.Unmap();
			}
			font.EndRenderUpdate();
		}
	}

	bool TextOverlay::RemoveFont(Font* font) {
		fonts.DestroyElement(font);
		return true;
	}

	void TextOverlay::WindowResize() {
		framebuffer_width = engine->window.screenDimensions.width;
		framebuffer_height = engine->window.screenDimensions.height;
		scale = framebuffer_width / DEFAULT_WIDTH<float>;
	}
}
