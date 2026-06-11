
#include "EWEngine/TextOverlay.h"

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
		: scale{ engine->window.screenDimensions.width / DEFAULT_WIDTH },
		framebuffer_width {engine->window.screenDimensions.width},
		framebuffer_height{ engine->window.screenDimensions.height },
		pipeLayout{
			engine->logicalDevice, 
			{
				Global::assetManager->shader.Get("textoverlay.vert.spv"), 
				Global::assetManager->shader.Get("textoverlay.frag.spv")
			}
		}
	{

		LoadConsolas24();
	}

	Font::~Font() {
		vkDestroySampler(engine->logicalDevice, sampler, nullptr);
	}

	Font::Font(std::unordered_map<wchar_t, CharacterData>& _vertData, std::unordered_map<wchar_t, float>& _advanceData, uint16_t _width, uint16_t _height, void* imgdata)
		:  width{ _width },
		height{ _height },
		vertData{ std::move(_vertData) },
		advanceData{ std::move(_advanceData) },
		image{ engine->logicalDevice },
		image_view{ image, false },
		buffers{
			engine->logicalDevice,
			sizeof(Font::CharacterData::Vert) * 4, TextOverlay::TEXTOVERLAY_MAX_CHAR_COUNT,
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
		sampler{
			engine->logicalDevice,
			VkSamplerCreateInfo{
				.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
				.pNext = nullptr,
				.magFilter = VK_FILTER_LINEAR,
				.minFilter = VK_FILTER_LINEAR,
				.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
				.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
				.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
				.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
				.mipLodBias = 0.0f,
				.maxAnisotropy = 1.0f,
				.compareOp = VK_COMPARE_OP_NEVER,
				.minLod = 0.0f,
				.maxLod = 1.0f,
				.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
			},
		}
	{
		image.name = "";
		image.data.extent = VkExtent3D{
			.width = width,
			.height = height,
			.depth = 1
		};
		image.data.arrayLayers = 1;
		image.data.mipLevels = 1;
		image.data.format = VK_FORMAT_B8G8R8A8_UNORM;
		image.data.layout = VK_IMAGE_LAYOUT_UNDEFINED;
		image.data.createFlags = 0;
		image.data.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		image.data.type = VK_IMAGE_TYPE_2D;
		image.data.samples = VK_SAMPLE_COUNT_1_BIT;
		image.data.tiling = VK_IMAGE_TILING_OPTIMAL;

		VmaAllocationCreateInfo vmaAllocCreateInfo{
			.flags = static_cast<VmaAllocationCreateFlags>(VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT) |
					static_cast<VmaAllocationCreateFlags>(VMA_ALLOCATOR_CREATE_EXTERNALLY_SYNCHRONIZED_BIT),
			.usage = VMA_MEMORY_USAGE_AUTO
		};
		image.Create(vmaAllocCreateInfo);
		image_view.Create();


		//i havent done staging buffer yet
		// Copy to image

		//i need to create a new STC manager
		TransferContext<Image> transferContext{
			.images{1},
			.regions{1},
			.stagingBuffer{engine->logicalDevice, width * height * 4, imgdata},
			.final_usage{
				.stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
				.accessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
				.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			},
			.generatingMipMaps = false
			
		};
		transferContext.images[0] = &image;
		Log::Error("fix this\n");
		transferContext.regions[0] = VkBufferImageCopy{
			
		};

		engine->stcManager.AsyncTransfer(transferContext, Queue::Graphics);

		VkDescriptorImageInfo descImgInfo;
		descImgInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		//descImgInfo.imageView = view;
		descImgInfo.sampler = sampler;




	}


	TextOverlay::~TextOverlay() {
		for (auto& font : fonts) {
			delete font;
		}
	}


	uint16_t TextStruct::GetSelectionIndex(double xpos) {
		const float charW = 1.5f * scale / engine->window.screenDimensions.width;
		const float width = GetWidth();
		float currentPos = x;

		const Font* font = Global::textOverlay->fonts[Global::textOverlay->currentFont];
#if EWE_DEBUG
		Log::Debug("xpos get selection index - %.1f \n", xpos);
#endif
		switch (align) {
			case TA_left:break;
			case TA_center: currentPos -= width / 2.f; break;
			case TA_right: currentPos -= width; break;
		}

		//float lastPos = currentPos;
		for (uint16_t i = 0; i < string.length(); i++) {
			currentPos += font->GetCharWidth(string[i], charW) * engine->window.screenDimensions.width / 8.f;
#if EWE_DEBUG
			Log::Debug("currentPos : %.2f \n", currentPos);
#endif
			if (xpos <= currentPos) { return i; }
			currentPos += font->GetCharWidth(string[i], charW) * engine->window.screenDimensions.width * 3.f / 8.f;
		}
		return static_cast<uint16_t>(string.length());
	}

	float Font::GetCharWidth(wchar_t c, const float charW) const {
		return advanceData.at(c) * charW;
	}
	float Font::GetStringWidth(std::string const& str, const float charW) const {
		float ret = 0.f;
		for (auto const& letter : str) {
			ret += advanceData.at(letter) * charW;
		}
		return ret;
	}
	Font::CharacterData::Vert const* Font::GetVertData(wchar_t c) const {
		return vertData.at(c).vertices;
	}

	float TextStruct::GetWidth() {
		const float charW = 1.5f * scale / engine->window.screenDimensions.width;
		Log::Warning("fix this\n");
		return 1.f;
		//Log::Debug("text struct get width : %.5f \n", textWidth);
#if EWE_DEBUG
		auto* font = Global::textOverlay->fonts[Global::textOverlay->currentFont];
		const float textWidth = font->GetStringWidth(string, charW);
		if (textWidth < 0.0f) {

			Log::Debug("width less than 0, what  was the string? : %s:%u \n", string.c_str(), engine->window.screenDimensions.width);
			EWE_ASSERT(false);
		}
		return textWidth;
#else
		return Global::textOverlay->fonts[Global::textOverlay->currentFont]->GetStringWidth(string, charW);
#endif
	}

	void TextOverlay::LoadConsolas24() {
		//FT_Face face;
		//hb_font_t* font = hb_ft_font_create(face, NULL);
		//unsigned char** font24pixels = new unsigned char* [STB_FONT_consolas_24_latin1_BITMAP_WIDTH];
		//for (std::size_t i = 0; i < STB_FONT_consolas_24_latin1_BITMAP_WIDTH; ++i) {
		//	font24pixels[i] = new unsigned char[STB_FONT_consolas_24_latin1_BITMAP_WIDTH];
		//}

		//std::vector<stb_fontchar> stbData{};
		//stb_font_consolas_24_latin1(stbData.data(), font24pixels, STB_FONT_consolas_24_latin1_BITMAP_WIDTH);
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


		VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
		pipelineCreateInfo.pNext = &renderingInfo;
		pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineCreateInfo.layout = pipeLayout->vkLayout;
		pipelineCreateInfo.flags = 0;
		pipelineCreateInfo.basePipelineIndex = -1;
		pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
		pipelineCreateInfo.pVertexInputState = &vertexInputState;
		pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
		pipelineCreateInfo.pRasterizationState = &rasterizationState;
		pipelineCreateInfo.pColorBlendState = &colorBlendState;
		pipelineCreateInfo.pMultisampleState = &multisampleState;
		pipelineCreateInfo.pViewportState = &viewportState;
		pipelineCreateInfo.pDepthStencilState = &depthStencilState;
		pipelineCreateInfo.pDynamicState = &dynamicState;

		auto stageData = pipeLayout->GetStageData();
		pipelineCreateInfo.stageCount = static_cast<uint32_t>(stageData.size());
		pipelineCreateInfo.pStages = stageData.data();

		EWE_VK(vkCreateGraphicsPipelines, VK::Object->vkDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipeline);
#if DEBUG_NAMING
		DebugNaming::SetObjectName(pipeline, VK_OBJECT_TYPE_PIPELINE, "textoverlay pipeline");
#endif
	}
	*/

	void TextOverlay::AddDefaultText(double time, double peakTime, double averageTime, double highTime) {
		const int16_t previousFont = Global::textOverlay->currentFont;
		SetCurrentFont(0);
		if (Global::textOverlay->fonts[Global::textOverlay->currentFont]->mapped == nullptr) {
			Global::textOverlay->fonts[Global::textOverlay->currentFont]->mapped = reinterpret_cast<Font::CharacterData::Vert*>(Global::textOverlay->fonts[Global::textOverlay->currentFont]->buffers[engine->frameIndex].mapped);
		}
		AddText(TextStruct{ engine->logicalDevice.physicalDevice.name, 0, Global::textOverlay->framebuffer_height - (20.f * Global::textOverlay->scale), TA_left, 1.f });
		int lastFPS = static_cast<int>(1 / time);
		int averageFPS = static_cast<int>(1 / averageTime);
		std::string buffer_string = std::format("frame time: {:.2f} ms ({} fps)", time * 1000.0f, lastFPS);
		AddText(TextStruct{ buffer_string, 0.f, Global::textOverlay->framebuffer_height - (40.f * Global::textOverlay->scale), TA_left, 1.f });
		buffer_string = std::format("average FPS: {}", averageFPS);
		AddText(TextStruct{ buffer_string, 0.f, Global::textOverlay->framebuffer_height - (60.f * Global::textOverlay->scale), TA_left, 1.f });
		buffer_string = std::format("peak: {:.2f} ms ~ average: {:.2f} ms ~ high: {:.2f} ms", peakTime * 1000, averageTime * 1000, highTime * 1000);
		AddText(TextStruct{ buffer_string, 0.f, Global::textOverlay->framebuffer_height - (80.f * Global::textOverlay->scale), TA_left, 1.f });
		SetCurrentFont(previousFont);
	}

	float TextOverlay::GetWidth(std::string const& text, float textScale) {
		//const float charW = 1.5f * Global::textOverlay->scale * textScale / Global::textOverlay->frameBufferWidth;
		float textWidth = 0.f;
		for (auto const& letter : text) {
			textWidth += Global::textOverlay->fonts[Global::textOverlay->currentFont]->advanceData.at(letter);
		}
		return textWidth;
	}
	void TextOverlay::StaticAddText(TextStruct textStruct) {
		Global::textOverlay->AddText(textStruct);
	}

	void TextOverlay::AddText(TextStruct const& textStruct, const float scaleX) {
		Font::CharacterData::Vert* mapped = Global::textOverlay->fonts[Global::textOverlay->currentFont]->mapped;
		if (mapped == nullptr) {
			mapped = reinterpret_cast<Font::CharacterData::Vert*>(Global::textOverlay->fonts[Global::textOverlay->currentFont]->buffers[engine->frameIndex].Map());
			Global::textOverlay->fonts[Global::textOverlay->currentFont]->mapped = mapped;
			EWE_ASSERT(mapped != nullptr);
		}
		mapped = mapped + (4 * Global::textOverlay->fonts[Global::textOverlay->currentFont]->drawnLetterCount);
		const float charW = 1.5f * Global::textOverlay->scale * scaleX * textStruct.scale / Global::textOverlay->framebuffer_width;
		const float charH = 1.5f * Global::textOverlay->scale * textStruct.scale / Global::textOverlay->framebuffer_height;

		float xPos = (textStruct.x / Global::textOverlay->framebuffer_width * 2.0f) - 1.0f;
		const float yPos = (textStruct.y / Global::textOverlay->framebuffer_height * 2.0f) - 1.0f;

		switch (textStruct.align) {
			case TA_right:
				for (auto const& letter : textStruct.string) {
					xPos -= Global::textOverlay->fonts[Global::textOverlay->currentFont]->GetCharWidth(letter, charW);
				}
				break;
			case TA_center:
				for (auto const& letter : textStruct.string) {
					xPos -= Global::textOverlay->fonts[Global::textOverlay->currentFont]->GetCharWidth(letter, charW) / 2.f;
				}
				break;
			case TA_left:
				break;
		}

		const std::size_t startingLetterCount = Global::textOverlay->fonts[Global::textOverlay->currentFont]->drawnLetterCount;
		for (auto const& letter : textStruct.string) {
			if (Global::textOverlay->fonts[Global::textOverlay->currentFont]->drawnLetterCount >= TEXTOVERLAY_MAX_CHAR_COUNT) {
				Log::Debug("trying to add more letters than allowed in textoverlay. consider expanding the TEXTOVERLAY_MAX_CHAR_COUNT constant - (drawn/max) (%zu/%u) \n", Global::textOverlay->fonts[Global::textOverlay->currentFont]->drawnLetterCount, TEXTOVERLAY_MAX_CHAR_COUNT);
				Global::textOverlay->fonts[Global::textOverlay->currentFont]->drawnLetterCount++;
				break;
			}
			Font::CharacterData::Vert const* verts = Global::textOverlay->fonts[Global::textOverlay->currentFont]->GetVertData(letter);

			mapped[0].x = (xPos + verts[0].x * charW);
			mapped[0].y = -(yPos + verts[0].y * charH);
			mapped[0].u = verts[0].u;
			mapped[0].v = verts[0].v;

			mapped[1].x = (xPos + verts[1].x * charW);
			mapped[1].y = -(yPos + verts[0].y * charH);
			mapped[1].u = verts[1].u;
			mapped[1].v = verts[0].v;

			mapped[2].x = (xPos + verts[0].x * charW);
			mapped[2].y = -(yPos + verts[1].y * charH);
			mapped[2].u = verts[0].u;
			mapped[2].v = verts[1].v;

			mapped[3].x = (xPos + verts[1].x * charW);
			mapped[3].y = -(yPos + verts[1].y * charH);
			mapped[3].u = verts[1].u;
			mapped[3].v = verts[1].v;

			mapped = reinterpret_cast<Font::CharacterData::Vert*>(reinterpret_cast<std::size_t>(mapped) + Global::textOverlay->fonts[Global::textOverlay->currentFont]->buffers[0].alignmentSize);

			xPos += Global::textOverlay->fonts[Global::textOverlay->currentFont]->GetCharWidth(letter, charW);

			Global::textOverlay->fonts[Global::textOverlay->currentFont]->drawnLetterCount++;

			Global::textOverlay->numLetters++;
		}


		/* we dont do this here anymore
		EWE_VK(vkCmdBindPipeline, EWE::VK::Object->GetFrameBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, Global::textOverlay->pipeline);

		EWE::VK::Object->BindVPScissor();


		EWE_VK(vkCmdBindDescriptorSets, VK::Object->GetFrameBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, Global::textOverlay->pipeLayout->vkLayout, 0, 1, &Global::textOverlay->fonts[Global::textOverlay->currentFont]->descriptorSets[engine->frameIndex], 0, nullptr);
		EWE_VK(vkCmdDraw, VK::Object->GetFrameBuffer(), 4, textStruct.string.size(), 0, startingLetterCount);
		*/
	}



	void TextOverlay::Draw() {
		for (auto& font : Global::textOverlay->fonts) {
			if (font->drawnLetterCount > 0) {
				font->buffers[engine->frameIndex].Flush();
				font->buffers[engine->frameIndex].Unmap();
				font->mapped = nullptr;
				font->drawnLetterCount = 0; // reset drawn letter count after binding
			}

		}
	}

	void TextOverlay::BeginTextUpdate() {
	}

	void TextOverlay::EndTextUpdate() {
		Draw();
	}

	bool TextOverlay::RemoveFont(uint16_t fontIndex) {
		if (fontIndex < Global::textOverlay->fonts.size()) {
			Global::textOverlay->fonts.erase(Global::textOverlay->fonts.begin() + fontIndex);
			return true;
		}
		return false;
	}


	int TextOverlay::AddFont(Font* font) {
		Global::textOverlay->fonts.push_back(font);
		return Global::textOverlay->fonts.size() - 1;
	}

	bool TextOverlay::SetCurrentFont(uint16_t whichFont) {
		if (whichFont < Global::textOverlay->fonts.size()) {
			Global::textOverlay->currentFont = whichFont;
			return true;
		}
		else {
#if EWE_DEBUG
			//Log::Debug("trying to set current font to %d, but only %d fonts available \n", whichFont, Global::textOverlay->fonts.size());
			EWE_ASSERT(whichFont < Global::textOverlay->fonts.size(), "trying to set current font to %d, but only %d fonts available");
#endif
			return false;
		}
	}

	void TextOverlay::WindowResize() {
		framebuffer_width = engine->window.screenDimensions.width;
		framebuffer_height = engine->window.screenDimensions.height;
		scale = framebuffer_width / DEFAULT_WIDTH;
	}

	int16_t TextOverlay::GetCurrentFont() {
		return Global::textOverlay->currentFont;
	}
	uint16_t TextOverlay::GetFontCount() { return Global::textOverlay->fonts.size(); };

	std::string_view TextOverlay::GetCurrentFontName() {
		if (Global::textOverlay->currentFont >= 0 && Global::textOverlay->currentFont < Global::textOverlay->fonts.size()) { return Global::textOverlay->fonts[Global::textOverlay->currentFont]->name; }
		else { 
			return "no font currently selected"; 
		}
	}
}
