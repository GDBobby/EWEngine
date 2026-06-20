
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
		framebuffer_width {engine->window.screenDimensions.width},
		framebuffer_height{ engine->window.screenDimensions.height },
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
		}
	{
		Log::Warning("load obj and raster pkg here eventually\n");
		LoadConsolas24();
	}


	TextOverlay::~TextOverlay() {}

	void TextOverlay::LoadConsolas24() {
		//FT_Face face;
		//hb_font_t* font = hb_ft_font_create(face, NULL);
		//unsigned char** font24pixels = new unsigned char* [STB_FONT_consolas_24_latin1_BITMAP_WIDTH];
		//for (std::size_t i = 0; i < STB_FONT_consolas_24_latin1_BITMAP_WIDTH; ++i) {
		//	font24pixels[i] = new unsigned char[STB_FONT_consolas_24_latin1_BITMAP_WIDTH];
		//}

		//std::vector<stb_fontchar> stbData{};
		//stb_font_consolas_24_latin1(stbData.data(), font24pixels, STB_FONT_consolas_24_latin1_BITMAP_WIDTH);
		auto& consolas = fonts.AddElement("Consolas.ttf", 24, sampler);
		currentFont = &consolas;
	}

	/*
	void TextOverlay::PreparePipeline(VkPipelineRenderingCreateInfo renderingInfo) {
		pipeLayout = Construct<EWE::PipeLayout>(std::initializer_list<std::string_view>{"Shader/textoverlay.vert.spv", "Shader/textoverlay.frag.spv"});

		VkPipelineColorBlendAttachmentState blendAttachmentState{};
		blendAttachmentState.blendEnable = VK_TRUE;
		blendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		blendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		blendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		blendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
		blendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		blendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		blendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;

		VkPipelineInputAssemblyStateCreateInfo inputAssemblyState{};
		inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
		inputAssemblyState.flags = 0;
		inputAssemblyState.primitiveRestartEnable = VK_FALSE;
		VkPipelineRasterizationStateCreateInfo rasterizationState{};
		rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizationState.cullMode = VK_CULL_MODE_NONE;
		rasterizationState.frontFace = VK_FRONT_FACE_CLOCKWISE;
		rasterizationState.flags = 0;
		rasterizationState.depthClampEnable = VK_FALSE;
		rasterizationState.lineWidth = 1.0f;
		VkPipelineColorBlendStateCreateInfo colorBlendState{};
		colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlendState.attachmentCount = 1;
		colorBlendState.pAttachments = &blendAttachmentState;
		VkPipelineDepthStencilStateCreateInfo depthStencilState{};
		depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencilState.depthTestEnable = VK_TRUE;
		depthStencilState.depthWriteEnable = VK_TRUE;
		depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		depthStencilState.back.compareOp = VK_COMPARE_OP_ALWAYS;
		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.scissorCount = 1;
		viewportState.flags = 0;
		VkPipelineMultisampleStateCreateInfo multisampleState{};
		multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampleState.flags = 0;
		VkDynamicState dynamicStateEnables[2] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		VkPipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.pDynamicStates = dynamicStateEnables;
		dynamicState.dynamicStateCount = 2;
		dynamicState.flags = 0;
		/*
		*
		* if using indices to character data instead of uploading the character data, use this pipeline vertex data
		*
		VkVertexInputBindingDescription vertexInputBindings;
		vertexInputBindings.binding = 0;
		vertexInputBindings.stride = sizeof(lab::vec4);
		vertexInputBindings.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		VkVertexInputAttributeDescription vertexInputAttributes;
		vertexInputAttributes.location = 0;
		vertexInputAttributes.binding = 0;
		vertexInputAttributes.format = VK_FORMAT_R16_UINT;
		vertexInputAttributes.offset = 0;
		*

		VkPipelineVertexInputStateCreateInfo vertexInputState{};
		vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

		vertexInputState.vertexBindingDescriptionCount = 0;
		vertexInputState.pVertexBindingDescriptions = nullptr;// &vertexInputBindings;
		vertexInputState.vertexAttributeDescriptionCount = 0;
		vertexInputState.pVertexAttributeDescriptions = nullptr;// &vertexInputAttributes;

		vertexInputState.vertexBindingDescriptionCount = 0;
		vertexInputState.pVertexBindingDescriptions = nullptr;
		vertexInputState.vertexAttributeDescriptionCount = 0;
		vertexInputState.pVertexAttributeDescriptions = nullptr;
	}
	*/

	void TextOverlay::AddDefaultText(LoopTimer const& timer) {
		SetCurrentFont(0);
		Font* font = Global::textOverlay->currentFont;
		AddText(TextStruct{ engine->logicalDevice.physicalDevice.name, 0, Global::textOverlay->framebuffer_height - (20.f * Global::textOverlay->scale), 0.1f, TA_left, 1.f });
		//const float lastFPS = LoopTimer::ToMilliseconds(timer.delta);
		/*
		const float averageFPS = LoopTimer::ToMilliseconds(timer.last_average);
		std::string buffer_string = std::format("frame time: {:.2f} ms)", LoopTimer::ToMilliseconds(timer.delta);
		AddText(TextStruct{ buffer_string, 0.f, Global::textOverlay->framebuffer_height - (40.f * Global::textOverlay->scale), TA_left, 1.f });
		buffer_string = std::format("average FPS: {}", averageFPS);
		AddText(TextStruct{ buffer_string, 0.f, Global::textOverlay->framebuffer_height - (60.f * Global::textOverlay->scale), TA_left, 1.f });
		buffer_string = std::format("peak: {:.2f} ms ~ average: {:.2f} ms ~ high: {:.2f} ms", peakTime * 1000, averageTime * 1000, highTime * 1000);
		AddText(TextStruct{ buffer_string, 0.f, Global::textOverlay->framebuffer_height - (80.f * Global::textOverlay->scale), TA_left, 1.f });
		SetCurrentFont(previousFont);
		*/
	}

	float TextOverlay::GetWidth(TextStruct const& ts) {
		return Global::textOverlay->currentFont->GetStringWidth(ts);
	}
	void TextOverlay::StaticAddText(TextStruct textStruct) {
		Global::textOverlay->AddText(textStruct);
	}

	void TextOverlay::AddText(TextStruct const& textStruct, const float scaleX) {
		Global::textOverlay->currentFont->AddText(textStruct);
	}

	void TextOverlay::BeginTextUpdate() {
		for (auto& font : Global::textOverlay->fonts) {
			font.char_instance_count = 0;
			font.string_debugger[engine->frameIndex].clear();
		}
		Global::textOverlay->numLetters = 0;
	}

	void TextOverlay::EndTextUpdate() {
		for (auto& font : Global::textOverlay->fonts) {
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
		Global::textOverlay->fonts.DestroyElement(font);
		return true;
	}


	Font* TextOverlay::AddFont(std::filesystem::path const& name, int size) {
		auto& font = Global::textOverlay->fonts.AddElement(name, size, Global::textOverlay->sampler);
		return &font;
	}

	bool TextOverlay::SetCurrentFont(Font* font) {
		Global::textOverlay->currentFont = font;
		return true;
	}

	void TextOverlay::WindowResize() {
		framebuffer_width = engine->window.screenDimensions.width;
		framebuffer_height = engine->window.screenDimensions.height;
		scale = framebuffer_width / DEFAULT_WIDTH<float>;
	}

	Font* TextOverlay::GetCurrentFont() {
		return Global::textOverlay->currentFont;
	}
	uint16_t TextOverlay::GetFontCount() { return Global::textOverlay->fonts.Size(); };

	std::filesystem::path const& TextOverlay::GetCurrentFontName() {
		return Global::textOverlay->currentFont->name;
	}
}
