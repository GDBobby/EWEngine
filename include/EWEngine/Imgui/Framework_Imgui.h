#pragma once

#include "EWEngine/Preprocessor.h"
#include "EightWinds/VulkanHeader.h"

#ifdef EWE_IMGUI

namespace EWE{

	//this is gonna be shitty until i get c++26 reflection

	struct ImguiExtension{
		template<typename T>
        static void Imgui(T& obj);
	};
	struct Image;
	struct Buffer;
	
	
	//forward declared specializations
	#define FWD_DEC_IMGUI(Type) struct Type; template<> void ImguiExtension::Imgui(Type& obj);
	
	FWD_DEC_IMGUI(RenderGraph);
	FWD_DEC_IMGUI(RasterTask);
	FWD_DEC_IMGUI(TaskRasterConfig);
	FWD_DEC_IMGUI(ObjectRasterConfig);
	FWD_DEC_IMGUI(ObjectRasterData);
	FWD_DEC_IMGUI(GlobalPushConstant_Abstract);
	FWD_DEC_IMGUI(GPUTask);
	FWD_DEC_IMGUI(TaskResourceUsage);
	//FWD_DEC_IMGUI(TaskAffix); //no valuable info i think, rebuilt every frame
	FWD_DEC_IMGUI(AttachmentSetInfo);
	template<> void ImguiExtension::Imgui(VkPipelineDepthStencilStateCreateInfo&);
	template<> void ImguiExtension::Imgui(VkPipelineRenderingCreateInfo&);
	FWD_DEC_IMGUI(DepthBias);
	template<> void ImguiExtension::Imgui(VkPipelineColorBlendAttachmentState&);
	template<> void ImguiExtension::Imgui(VkStencilOpState&);
	FWD_DEC_IMGUI(PipeLayout);
	FWD_DEC_IMGUI(Shader);
	FWD_DEC_IMGUI(AttachmentInfo);
	template<> void ImguiExtension::Imgui(VkAttachmentLoadOp&);
	template<> void ImguiExtension::Imgui(VkAttachmentStoreOp&);

	namespace Command {
		struct Record;
	}
	template<> void ImguiExtension::Imgui(Command::Record&);
	
	//need each param pack
	
	#undef FWD_DEC_IMGUI

	template<typename T>
	struct Resource;
	template<typename T>
	struct UsageData;

	template<> void ImguiExtension::Imgui<Resource<Image>>(Resource<Image>& obj);
	template<> void ImguiExtension::Imgui(Resource<Buffer>& obj);
	template<> void ImguiExtension::Imgui(UsageData<Image>& obj);
	template<> void ImguiExtension::Imgui(UsageData<Buffer>& obj);
		
} //namespace EWE
#endif