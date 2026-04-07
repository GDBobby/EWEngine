#pragma once

#include "EWEngine/Imgui/DragDrop.h"
#include "EWEngine/Preprocessor.h"
#include "EightWinds/DescriptorImageInfo.h"
#include "EightWinds/GlobalPushConstant.h"
#include "EightWinds/VulkanHeader.h"

#include "imgui.h"
#include "EightWinds/Command/Instruction.h"

#if EWE_IMGUI

namespace EWE{
	template<Inst::Type IType>
	void ImguiDefaultParamReflect(ParamPack<IType>* mem_addr){
		static constexpr std::size_t member_count = std::meta::nonstatic_data_members_of(^^ParamPack<IType>, std::meta::access_context::current()).size();
		static constexpr auto members = std::define_static_array(std::meta::nonstatic_data_members_of(^^ParamPack<IType>, std::meta::access_context::current()));

		template for(constexpr auto mem : members){
			static constexpr auto name = std::meta::identifier_of(mem);
			auto* mem_ptr = &mem_addr->[:mem:];
			ImGui::DragFloat(name.data(), reinterpret_cast<float*>(mem_ptr), -1000.f, 1000.f);
		}
	}

	template<Inst::Type IType>
	inline void ImguiReflectParamStruct(ParamPack<IType>* mem_addr){
		ImguiDefaultParamReflect(mem_addr);
	}

	template<>
	inline void ImguiReflectParamStruct(ParamPack<Inst::Type::BeginRender>* temp_addr){
		VkRenderingInfo* real_addr = reinterpret_cast<VkRenderingInfo*>(temp_addr);
		//mem_addr->
	}
	template<>
	inline void ImguiReflectParamStruct(ParamPack<Inst::Type::Push>* temp_addr){
		GlobalPushConstant_Raw* mem_addr = reinterpret_cast<GlobalPushConstant_Raw*>(temp_addr);
		if(ImGui::BeginTable("push", 2)){
			ImGui::TableSetupColumn("buffer addresses");
			ImGui::TableSetupColumn("texture indices");

			uint8_t iter = 0;
			while(iter < GlobalPushConstant_Raw::buffer_count || iter < GlobalPushConstant_Raw::texture_count){
				//this is buffer device addresses
				ImGui::TableNextColumn();
				if(iter < GlobalPushConstant_Raw::buffer_count){
					if(mem_addr->buffer_addr[iter] != null_buffer){
						auto* buf = Global::buffers->Get(Global::buffers->ConvertBDAToHash(mem_addr->buffer_addr[iter]));
						ImGui::Text("buffer : %s", buf->name.c_str());
					}
					else{
						ImGui::Text("buffer : nullptr");
					}
					Buffer* temp_drop_ptr = nullptr;
					if(DragDropPtr::Target<Buffer>(temp_drop_ptr)){
						mem_addr->buffer_addr[iter] = temp_drop_ptr->deviceAddress;
					}
				}
				ImGui::TableNextColumn();
				if(iter < GlobalPushConstant_Raw::texture_count){
					if(mem_addr->texture_indices[iter] != null_texture){
						auto& dii = Global::diis->Get(Global::diis->ConvertTextureIndexToHash(mem_addr->texture_indices[iter]));
						ImGui::Text("texture : %s", dii.name.c_str());
					}
					else{
						ImGui::Text("texture : nullptr");
					}
					DescriptorImageInfo* temp_drop_ptr = nullptr;
					if(DragDropPtr::Target<DescriptorImageInfo>(temp_drop_ptr)){
						mem_addr->texture_indices[iter] = temp_drop_ptr->index;
					}
				}
				iter++;
			}
		}
	}

	void ImguiExpandInstruction(void* mem_addr, Inst::Type itype);
} //namespace EWE
#endif