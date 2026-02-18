
#include "EWEngine/TextOverlay.h"


//#define STB_TRUETYPE_IMPLEMENTATION  // force following include to generate implementation
//#include "EWEngine/Fonts/stb_font_consolas_24_latin1.inl"
#include "EightWinds/Backend/StagingBuffer.h"
#include "EightWinds/RenderGraph/Resources.h"
#include "EWEngine/STC_Manager.h"

#include <stdexcept>
#include <sstream>
#include <iomanip>

#include <hb.h>
#include <hb-ft.h>


namespace EWE {

	TextOverlay* textOverlayPtr{ nullptr };


	TextOverlay::TextOverlay(LogicalDevice& logicalDevice, float screenWidth, float screenHeight)
		: logicalDevice{ logicalDevice }, 
		framebuffer_width {screenWidth},
		framebuffer_height{ screenHeight },
		scale{ framebuffer_width / DEFAULT_WIDTH },
		vertShader{ logicalDevice, "common/shaders/textoverlay.vert.spv" },
		fragShader{ logicalDevice, "common/shaders/textoverlay.frag.spv" },
		pipeLayout{ logicalDevice, {&vertShader, &fragShader} }
	{
		EWE_ASSERT(textOverlayPtr == nullptr, "trying to recreate textoverlay??");
		textOverlayPtr = this;

		LoadConsolas24();
	}

	Font::~Font() {
		vkDestroySampler(*Global::logicalDevice, sampler, nullptr);
	}

	Font::Font(std::unordered_map<wchar_t, CharacterData>& vertData, std::unordered_map<wchar_t, float>& advanceData, uint16_t width, uint16_t height, void* imgdata)
		: vertData{ std::move(vertData) },
		advanceData{ std::move(advanceData) },
		width{ width },
		height{ height },
		image{ *Global::logicalDevice },
		image_view{ image, false },
		buffers{
			*Global::logicalDevice,
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
			*Global::logicalDevice,
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
		image.extent = VkExtent3D{
			.width = width,
			.height = height,
			.depth = 1
		};
		image.arrayLayers = 1;
		image.mipLevels = 1;
		image.format = VK_FORMAT_B8G8R8A8_UNORM;
		image.layout = VK_IMAGE_LAYOUT_UNDEFINED;
		image.createFlags = 0;
		image.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		image.type = VK_IMAGE_TYPE_2D;
		image.samples = VK_SAMPLE_COUNT_1_BIT;
		image.tiling = VK_IMAGE_TILING_OPTIMAL;

		VmaAllocationCreateInfo vmaAllocCreateInfo{
			.flags = static_cast<VmaAllocationCreateFlags>(VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT) |
					static_cast<VmaAllocationCreateFlags>(VMA_ALLOCATOR_CREATE_EXTERNALLY_SYNCHRONIZED_BIT),
			.usage = VMA_MEMORY_USAGE_AUTO
		};
		image.Create(vmaAllocCreateInfo);
		image_view.Create();


		//i havent done staging buffer yet
		StagingBuffer* stagingBuffer = new StagingBuffer(*Global::logicalDevice, width * height * 4, imgdata);
		// Copy to image

		//i need to create a new STC manager
		AsyncTransferContext_Image transferContext{
			.resource{
				image,
				UsageData<Image>{
					.stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
					.accessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
					.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
				}
			}
		};
		Global::stcManager->AsyncTransfer(transferContext, Queue::Graphics);

		VkDescriptorImageInfo descImgInfo;
		descImgInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		//descImgInfo.imageView = view;
		descImgInfo.sampler = sampler;




	}


	TextOverlay::~TextOverlay() {
		// Free up all Vulkan resources requested by the text overlay
#if DECONSTRUCTION_DEBUG
		printf("deconstrructing textoverlay \n");
#endif
		for (auto& font : fonts) {
			delete font;
		}

#if DECONSTRUCTION_DEBUG
		printf("end deconstruction textoverlay \n");
#endif

	}


	uint16_t TextStruct::GetSelectionIndex(double xpos) {
		const float charW = 1.5f * scale / Global::window->screenDimensions.width;
		const float width = GetWidth();
		float currentPos = x;

		const Font* font = textOverlayPtr->fonts[textOverlayPtr->currentFont];
#if EWE_DEBUG
		printf("xpos get selection index - %.1f \n", xpos);
#endif
		switch (align) {
			case TA_left:break;
			case TA_center: currentPos -= width / 2.f; break;
			case TA_right: currentPos -= width; break;
		}

		//float lastPos = currentPos;
		for (uint16_t i = 0; i < string.length(); i++) {
			currentPos += font->GetCharWidth(string[i], charW) * Global::window->screenDimensions.width / 8.f;
#if EWE_DEBUG
			printf("currentPos : %.2f \n", currentPos);
#endif
			if (xpos <= currentPos) { return i; }
			currentPos += font->GetCharWidth(string[i], charW) * Global::window->screenDimensions.width * 3.f / 8.f;
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
		//std::cout << "yo? : " << frameBufferWidth << std::endl;
		const float charW = 1.5f * scale / Global::window->screenDimensions.width;
		//printf("text struct get width : %.5f \n", textWidth);
#if EWE_DEBUG
		const float textWidth = textOverlayPtr->fonts[textOverlayPtr->currentFont]->GetStringWidth(string, charW);
		if (textWidth < 0.0f) {

			printf("width less than 0, what  was the string? : %s:%u \n", string.c_str(), Global::window->screenDimensions.width);
			assert(false);
		}
		return textWidth;
#else
		return textOverlayPtr->fonts[textOverlayPtr->currentFont]->GetStringWidth(string, charW);
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
		const int16_t previousFont = textOverlayPtr->currentFont;
		SetCurrentFont(0);
		if (textOverlayPtr->fonts[textOverlayPtr->currentFont]->mapped == nullptr) {
			textOverlayPtr->fonts[textOverlayPtr->currentFont]->mapped = reinterpret_cast<Font::CharacterData::Vert*>(textOverlayPtr->fonts[textOverlayPtr->currentFont]->buffers[Global::frameIndex].mapped);
		}
		AddText(TextStruct{ Global::logicalDevice->physicalDevice.name, 0, textOverlayPtr->framebuffer_height - (20.f * textOverlayPtr->scale), TA_left, 1.f });
		int lastFPS = static_cast<int>(1 / time);
		int averageFPS = static_cast<int>(1 / averageTime);
		std::string buffer_string = std::format("frame time: {:.2f} ms ({} fps)", time * 1000.0f, lastFPS);
		AddText(TextStruct{ buffer_string, 0.f, textOverlayPtr->framebuffer_height - (40.f * textOverlayPtr->scale), TA_left, 1.f });
		buffer_string = std::format("average FPS: {}", averageFPS);
		AddText(TextStruct{ buffer_string, 0.f, textOverlayPtr->framebuffer_height - (60.f * textOverlayPtr->scale), TA_left, 1.f });
		buffer_string = std::format("peak: {:.2f} ms ~ average: {:.2f} ms ~ high: {:.2f} ms", peakTime * 1000, averageTime * 1000, highTime * 1000);
		AddText(TextStruct{ buffer_string, 0.f, textOverlayPtr->framebuffer_height - (80.f * textOverlayPtr->scale), TA_left, 1.f });
		SetCurrentFont(previousFont);
	}

	float TextOverlay::GetWidth(std::string const& text, float textScale) {
		//const float charW = 1.5f * textOverlayPtr->scale * textScale / textOverlayPtr->frameBufferWidth;
		float textWidth = 0.f;
		for (auto const& letter : text) {
			textWidth += textOverlayPtr->fonts[textOverlayPtr->currentFont]->advanceData.at(letter);
		}
		return textWidth;
	}
	void TextOverlay::StaticAddText(TextStruct textStruct) {
		textOverlayPtr->AddText(textStruct);
	}

	void TextOverlay::AddText(TextStruct const& textStruct, const float scaleX) {
		Font::CharacterData::Vert* mapped = textOverlayPtr->fonts[textOverlayPtr->currentFont]->mapped;
		if (mapped == nullptr) {
			mapped = reinterpret_cast<Font::CharacterData::Vert*>(textOverlayPtr->fonts[textOverlayPtr->currentFont]->buffers[Global::frameIndex].Map());
			textOverlayPtr->fonts[textOverlayPtr->currentFont]->mapped = mapped;
			assert(mapped != nullptr);
		}
		mapped = mapped + (4 * textOverlayPtr->fonts[textOverlayPtr->currentFont]->drawnLetterCount);
		const float charW = 1.5f * textOverlayPtr->scale * scaleX * textStruct.scale / textOverlayPtr->framebuffer_width;
		const float charH = 1.5f * textOverlayPtr->scale * textStruct.scale / textOverlayPtr->framebuffer_height;

		float xPos = (textStruct.x / textOverlayPtr->framebuffer_width * 2.0f) - 1.0f;
		const float yPos = (textStruct.y / textOverlayPtr->framebuffer_height * 2.0f) - 1.0f;

		switch (textStruct.align) {
			case TA_right:
				for (auto const& letter : textStruct.string) {
					xPos -= textOverlayPtr->fonts[textOverlayPtr->currentFont]->GetCharWidth(letter, charW);
				}
				break;
			case TA_center:
				for (auto const& letter : textStruct.string) {
					xPos -= textOverlayPtr->fonts[textOverlayPtr->currentFont]->GetCharWidth(letter, charW) / 2.f;
				}
				break;
			case TA_left:
				break;
		}

		const std::size_t startingLetterCount = textOverlayPtr->fonts[textOverlayPtr->currentFont]->drawnLetterCount;
		for (auto const& letter : textStruct.string) {
			if (textOverlayPtr->fonts[textOverlayPtr->currentFont]->drawnLetterCount >= TEXTOVERLAY_MAX_CHAR_COUNT) {
				printf("trying to add more letters than allowed in textoverlay. consider expanding the TEXTOVERLAY_MAX_CHAR_COUNT constant - (drawn/max) (%zu/%u) \n", textOverlayPtr->fonts[textOverlayPtr->currentFont]->drawnLetterCount, TEXTOVERLAY_MAX_CHAR_COUNT);
				textOverlayPtr->fonts[textOverlayPtr->currentFont]->drawnLetterCount++;
				break;
			}
			Font::CharacterData::Vert const* verts = textOverlayPtr->fonts[textOverlayPtr->currentFont]->GetVertData(letter);

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

			mapped = reinterpret_cast<Font::CharacterData::Vert*>(reinterpret_cast<std::size_t>(mapped) + textOverlayPtr->fonts[textOverlayPtr->currentFont]->buffers[0].alignmentSize);

			xPos += textOverlayPtr->fonts[textOverlayPtr->currentFont]->GetCharWidth(letter, charW);

			textOverlayPtr->fonts[textOverlayPtr->currentFont]->drawnLetterCount++;

			textOverlayPtr->numLetters++;
		}


		/* we dont do this here anymore
		EWE_VK(vkCmdBindPipeline, EWE::VK::Object->GetFrameBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, textOverlayPtr->pipeline);

		EWE::VK::Object->BindVPScissor();


		EWE_VK(vkCmdBindDescriptorSets, VK::Object->GetFrameBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, textOverlayPtr->pipeLayout->vkLayout, 0, 1, &textOverlayPtr->fonts[textOverlayPtr->currentFont]->descriptorSets[VK::Object->frameIndex], 0, nullptr);
		EWE_VK(vkCmdDraw, VK::Object->GetFrameBuffer(), 4, textStruct.string.size(), 0, startingLetterCount);
		*/
	}



	void TextOverlay::Draw() {
		for (auto& font : textOverlayPtr->fonts) {
			if (font->drawnLetterCount > 0) {
				font->buffers[Global::frameIndex].Flush();
				font->buffers[Global::frameIndex].Unmap();
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
		if (fontIndex < textOverlayPtr->fonts.size()) {
			textOverlayPtr->fonts.erase(textOverlayPtr->fonts.begin() + fontIndex);
			return true;
		}
		return false;
	}


	int TextOverlay::AddFont(Font* font) {
		textOverlayPtr->fonts.push_back(font);
		return textOverlayPtr->fonts.size() - 1;
	}

	bool TextOverlay::SetCurrentFont(uint16_t whichFont) {
		if (whichFont < textOverlayPtr->fonts.size()) {
			textOverlayPtr->currentFont = whichFont;
			return true;
		}
		else {
#if EWE_DEBUG
			//printf("trying to set current font to %d, but only %d fonts available \n", whichFont, textOverlayPtr->fonts.size());
			EWE_ASSERT(whichFont < textOverlayPtr->fonts.size(), "trying to set current font to %d, but only %d fonts available");
#endif
			return false;
		}
	}

	int16_t TextOverlay::GetCurrentFont() {
		return textOverlayPtr->currentFont;
	}
	uint16_t TextOverlay::GetFontCount() { return textOverlayPtr->fonts.size(); };

	std::string_view TextOverlay::GetCurrentFontName() {
		if (textOverlayPtr->currentFont >= 0 && textOverlayPtr->currentFont < textOverlayPtr->fonts.size()) { return textOverlayPtr->fonts[textOverlayPtr->currentFont]->name; }
		else { 
			return "no font currently selected"; 
		}
	}
}
