#pragma once

#include "EWEngine/Preprocessor.h"
#include "EightWinds/VulkanHeader.h"

#include "imgui.h"
#include "EightWinds/RenderGraph/Command/Instruction.h"

#if EWE_IMGUI

namespace EWE{
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