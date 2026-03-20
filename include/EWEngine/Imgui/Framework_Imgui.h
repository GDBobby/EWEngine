#pragma once

#include "EWEngine/Preprocessor.h"
#include "EightWinds/VulkanHeader.h"

#include "imgui.h"
#include "EightWinds/RenderGraph/Command/Instruction.h"

#ifdef EWE_IMGUI

namespace EWE{

	struct ImguiExtension{
		template<typename T>
        static void Imgui(T& obj);
	};
	struct Image;
	struct ImageView;
	struct Buffer;
	
	
	//forward declared specializations
	#define FWD_DEC_IMGUI(Type) struct Type; template<> void ImguiExtension::Imgui(Type& obj);
	
	FWD_DEC_IMGUI(Image);
	FWD_DEC_IMGUI(ImageView);
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

    //putting these in the macro leads to weird compiler behavior (as of gcc 16.0.1 20260217 (experimental))
	template<> void ImguiExtension::Imgui(VkAttachmentLoadOp&);
	template<> void ImguiExtension::Imgui(VkAttachmentStoreOp&);
	template<> void ImguiExtension::Imgui(VkSamplerCreateInfo&);
    

	namespace Command {
		struct Record;
		struct Executor;
	}

	template<> void ImguiExtension::Imgui(Command::Executor&);
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




	template<typename T>
	void ImguiDefaultParamReflect(T* mem_addr){
		static constexpr std::size_t member_count = std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::current()).size();
		static constexpr auto members = std::define_static_array(std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::current()));

		template for(constexpr auto mem : members){
			static constexpr auto name = std::meta::identifier_of(mem);
			auto* mem_ptr = &mem_addr->[:mem:];
			ImGui::DragFloat(name.data(), reinterpret_cast<float*>(mem_ptr), -1000.f, 1000.f);
		}
	}

	template<typename T>
	void ImguiReflectParamStruct(T* mem_addr){
		if constexpr(std::is_same_v<VkRenderingInfo, T>){

		}
		else{
			ImguiDefaultParamReflect(mem_addr);
		}
	}

	void ImguiExpandInstruction(void* mem_addr, Instruction::Type itype);


		
} //namespace EWE
#endif