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
			ImGui::SetNextItemWidth(200.0f);
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
	void ImguiReflectParamStruct(ParamPack<Inst::Type::Push>* temp_addr);

	void ImguiExpandInstruction(void* mem_addr, Inst::Type itype);
} //namespace EWE
#endif