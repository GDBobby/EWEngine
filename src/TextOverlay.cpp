
#include "../include/EWEngine/Graphics/TextOverlay.h"

#include "EWEngine/EWEngine.h"

//#define STB_TRUETYPE_IMPLEMENTATION  // force following include to generate implementation
//#include "EWEngine/Fonts/stb_font_consolas_24_latin1.inl"
#include "EightWinds/Backend/StagingBuffer.h"
#include "EightWinds/RenderGraph/Resources.h"


#include <hb.h>
#include <hb-ft.h>
#include <vulkan/vulkan_core.h>


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
		draw_buffer{
			engine->logicalDevice,
			sizeof(FontDraw), font_limit,
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
		},
		indirect_buffer{
			engine->logicalDevice,
			sizeof(VkDrawIndirectCommand), font_limit,
			VmaAllocationCreateInfo{
				.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
				.usage = VMA_MEMORY_USAGE_AUTO,
				.requiredFlags = 0,
				.preferredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				.memoryTypeBits = 0,
				.pool = VK_NULL_HANDLE,
				.pUserData = nullptr,
				.priority = 1.f
			},
			VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT
		},
		objPkg{"textoverlay"}
	{
		//objPkg.paramPool.PushBack(Inst::Breakpoint);
		//objPkg.paramPool.PushBack(Inst::DebugFunction);
		objPkg.paramPool.PushBack(Inst::Push);
		objPkg.paramPool.PushBack(Inst::DrawIndirect);

		pushPack = *objPkg.paramPool.param_data[0].CastTo<ParamPack<Inst::Push>>();
		indirectPack = *objPkg.paramPool.param_data[1].CastTo<ParamPack<Inst::DrawIndirect>>();

		//auto temp_debug_pack = *objPkg.paramPool.param_data[0].CastTo<ParamPack<Inst::DebugFunction>>();

		for_each_frame{
			//temp_debug_pack.GetRef(frame).callback = [&](EWE::Command::Exec::ExecContext& ctx){
				//draw_buffer[frame
			//	Log::Debug("manually inserted breakpoint\n");
			//};

			std::string buffer_name = "textoverlay_draw[" + std::to_string(frame) + ']';
			draw_buffer[frame].SetName(buffer_name);
			buffer_name = "textoverlay_indirect[" + std::to_string(frame) + ']';
			indirect_buffer[frame].SetName(buffer_name);
			
			pushPack.GetRef(frame).buffer_count = 1;
			pushPack.GetRef(frame).texture_count = 0;
			pushPack.GetRef(frame).size = pushPack.GetRef(frame).Size();
			pushPack.GetRef(frame).GetDeviceAddress(0) = draw_buffer[frame].deviceAddress;

			indirectPack.GetRef(frame).buffer = indirect_buffer[frame].buffer_info.buffer;
			indirectPack.GetRef(frame).offset = 0;
			indirectPack.GetRef(frame).drawCount = 0;
			indirectPack.GetRef(frame).stride = sizeof(VkDrawIndirectCommand);
		}

        objPkg.payload.shaders[ShaderStage::Vertex] = Global::assetManager->shader.Get("textoverlay.vert.spv");
        objPkg.payload.shaders[ShaderStage::Fragment] = Global::assetManager->shader.Get("textoverlay.frag.spv");
		objPkg.payload.config.SetDefaults();
        objPkg.payload.config.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
		/*
		static constexpr std::size_t size_debugger = sizeof(FontDraw);
		static constexpr std::size_t align_debugger = sizeof(VkDrawIndirectCommand);

		FontDraw debug_array[2];
		const std::size_t offset_debugger = reinterpret_cast<std::size_t>(&debug_array[1]) - reinterpret_cast<std::size_t>(&debug_array[0]);
        */
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
		font_draw_data.clear();
		indirect_cmd_data.clear();
		font_draw_data.resize(fonts.Size());
		indirect_cmd_data.resize(fonts.Size());

		uint32_t font_index = 0;
		uint32_t cmd_index = 0;

		uint32_t& drawCount = indirectPack.GetRef(engine->frameIndex).drawCount;
		drawCount = 0;

		for (auto& font : fonts) {
			auto& font_buffer = font.buffer[engine->frameIndex];
			if(font.char_instance_count > 0){
				font_draw_data[font_index].buffer = font_buffer.deviceAddress;
				font_draw_data[font_index].index = font.dii.index;

				indirect_cmd_data[cmd_index].vertexCount = 4;
				indirect_cmd_data[cmd_index].instanceCount = font.char_instance_count;
				indirect_cmd_data[cmd_index].firstVertex = 0;
				indirect_cmd_data[cmd_index].firstInstance = 0;
				
				/*
				Log::Debug("font[%s] draw buffer[%u] : %s : %s : %u\n", 
					font.name.string().c_str(), drawCount, 
					engine->logicalDevice.RevertDA(font_draw_data[font_index].buffer).name.string().c_str(), 
					engine->logicalDevice.RevertTI(font_draw_data[font_index].index).view.image.name.string().c_str(),
					indirect_cmd_data[cmd_index].instanceCount
				);
				*/

				cmd_index++;
				font_index++;
				drawCount++;

				//font.ifPack.GetRef(engine->frameIndex).enabled = font.char_instance_count > 0;
				font.EndRenderUpdate();
			}
		}
		if(drawCount > 1){
			//drawCount = 1;
		}

		memcpy(draw_buffer[engine->frameIndex].Map(), font_draw_data.data(), font_draw_data.size() * sizeof(FontDraw));
		memcpy(indirect_buffer[engine->frameIndex].Map(), indirect_cmd_data.data(), indirect_cmd_data.size() * sizeof(VkDrawIndirectCommand));
		draw_buffer[engine->frameIndex].Flush();
		indirect_buffer[engine->frameIndex].Flush();
		draw_buffer[engine->frameIndex].Unmap();
		indirect_buffer[engine->frameIndex].Unmap();
	}

	bool TextOverlay::RemoveFont(Font* font) {
		fonts.DestroyElement(font);
		return true;
	}
}
