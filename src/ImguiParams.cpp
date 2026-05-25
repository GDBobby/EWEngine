#include "EWEngine/Imgui/Params.h"

#include "EWEngine/EWEngine.h"

namespace EWE{

    void ImguiExpandInstruction(void* mem_addr, Inst::Type itype){
            static constexpr auto type_mems = std::define_static_array(std::meta::enumerators_of(^^Inst::Type));

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
            template for(constexpr auto type_mem : type_mems){
                if ([:type_mem:] == itype){
                    if constexpr(std::meta::is_complete_type(^^ParamPack<([:type_mem:])>)){
                        ImguiReflectParamStruct(reinterpret_cast<ParamPack<([:type_mem:])>*>(mem_addr));
                    }
                }
            }
#pragma GCC diagnostic pop
        
    }

    template<>
	void ImguiReflectParamStruct<Inst::Type::Push>(ParamPack<Inst::Type::Push>* push){
		if(ImGui::BeginTable("push", 2, ImGuiTableFlags_Borders| ImGuiTableFlags_SizingFixedFit, ImVec2(300.0f, 0.0f))) {
			ImGui::TableSetupColumn("buffer addresses", ImGuiTableColumnFlags_WidthFixed, 150.0f);
			ImGui::TableSetupColumn("texture indices", ImGuiTableColumnFlags_WidthFixed, 150.0f);
            ImGui::TableHeadersRow();
			uint8_t iter = 0;

			while(iter < push->buffer_count || iter < push->texture_count){
				//this is buffer device addresses
				ImGui::TableNextColumn();

				if(iter < push->buffer_count){
					if(push->GetDeviceAddress(iter) != null_buffer){
						auto& buf = *engine->assetManager.buffer.Get(engine->assetManager.buffer.ConvertBDAToHash(push->GetDeviceAddress(iter)));
						ImGui::Text("%u:%s", iter, buf.name.c_str());
						ImGui::SetItemTooltip(buf.name.c_str());
					}
					else{
						ImGui::Text("%u:null", iter);
					}
					Buffer* temp_drop_ptr = nullptr;
					if(DragDropPtr::Target(temp_drop_ptr)){
						push->GetDeviceAddress(iter) = temp_drop_ptr->deviceAddress;
					}
				}
				ImGui::TableNextColumn();
				if(iter < push->texture_count){
					if(push->GetTextureIndex(iter) != null_texture){
						const AssetHash temp_hash = engine->assetManager.dii.ConvertTextureIndexToHash(push->GetTextureIndex(iter));
#if EWE_DEBUG_BOOL
						if(temp_hash == Asset::INVALID_HASH) {
							Log::Warning("caught an invalid push texture index\n");
							push->GetTextureIndex(iter) = null_texture;
						}
						else{
#endif
							auto* dii = engine->assetManager.dii.Get(temp_hash);
							ImGui::Text("%u:%s", iter, dii->name.string().c_str());
							ImGui::SetItemTooltip(dii->name.string().c_str());
#if EWE_DEBUG_BOOL
						}
#endif
					}
					else{
						ImGui::Text("%u:null", iter);
					}
					DescriptorImageInfo* temp_drop_ptr = nullptr;
					if(DragDropPtr::Target(temp_drop_ptr)){
						push->GetTextureIndex(iter) = temp_drop_ptr->index;
					}
				}
				iter++;
			}
			ImGui::EndTable();
		}
	}

	void ExpandPush(PushConstant const& constant, ParamPack<Inst::Type::Push>* push) {
		if(ImGui::BeginTable("push", 4, ImGuiTableFlags_Borders| ImGuiTableFlags_SizingFixedFit, ImVec2(300.0f, 0.0f))) {
			ImGui::TableSetupColumn("buffer name", ImGuiTableColumnFlags_WidthFixed, 100.0f);
			ImGui::TableSetupColumn("buffer address", ImGuiTableColumnFlags_WidthFixed, 50.0f);
			ImGui::TableSetupColumn("texture index", ImGuiTableColumnFlags_WidthFixed, 50.0f);
			ImGui::TableSetupColumn("texture name", ImGuiTableColumnFlags_WidthFixed, 100.0f);
            ImGui::TableHeadersRow();
			uint8_t iter = 0;

			while(iter < push->buffer_count || iter < push->texture_count){
				//this is buffer device addresses
				ImGui::TableNextColumn();
				if(iter < constant.buffers.size()){
					ImGui::Text("%s[%u]", constant.buffers[iter].name.c_str(), iter);
				}
				ImGui::TableNextColumn();

				if(iter < push->buffer_count){
					
					if(push->GetDeviceAddress(iter) != null_buffer){
						auto& buf = *engine->assetManager.buffer.Get(engine->assetManager.buffer.ConvertBDAToHash(push->GetDeviceAddress(iter)));
						ImGui::Text(buf.name.c_str());
						ImGui::SetItemTooltip(buf.name.c_str());
					}
					else{
						ImGui::Text("null");
					}
					Buffer* temp_drop_ptr = nullptr;
					if(DragDropPtr::Target(temp_drop_ptr)){
						push->GetDeviceAddress(iter) = temp_drop_ptr->deviceAddress;
					}
				}


				ImGui::TableNextColumn();
				if(iter < constant.textures.size()){
					ImGui::Text("%s[%u]", constant.textures[iter].name.c_str(), iter);
				}
				ImGui::TableNextColumn();

				if(iter < push->texture_count){
					if(push->GetTextureIndex(iter) != null_texture){
						const AssetHash temp_hash = engine->assetManager.dii.ConvertTextureIndexToHash(push->GetTextureIndex(iter));
#if EWE_DEBUG_BOOL
						if(temp_hash == Asset::INVALID_HASH) {
							Log::Warning("caught an invalid push texture index\n");
							push->GetTextureIndex(iter) = null_texture;
						}
						else{
#endif
							auto* dii = engine->assetManager.dii.Get(temp_hash);
							ImGui::Text(dii->name.string().c_str());
							ImGui::SetItemTooltip(dii->name.string().c_str());
#if EWE_DEBUG_BOOL
						}
#endif
					}
					else{
						ImGui::Text("null");
					}
					DescriptorImageInfo* temp_drop_ptr = nullptr;
					if(DragDropPtr::Target(temp_drop_ptr)){
						push->GetTextureIndex(iter) = temp_drop_ptr->index;
					}
				}
				iter++;
			}
			ImGui::EndTable();
		}
	}


	template<>
	inline void ImguiReflectParamStruct(ParamPack<Inst::Draw>* mem_addr){
		auto const addr_id = static_cast<int>(reinterpret_cast<std::size_t>(mem_addr));
		ImGui::PushID(addr_id);
		ImGui::DragInt("vertex count", reinterpret_cast<int*>(&mem_addr->vertexCount), 1.f, 0, INT32_MAX);
		ImGui::DragInt("instance count", reinterpret_cast<int*>(&mem_addr->instanceCount), 1.f, 0, INT32_MAX);
		ImGui::DragInt("first vertex", reinterpret_cast<int*>(&mem_addr->firstVertex), 1.f, 0, mem_addr->vertexCount);
		ImGui::DragInt("first instance", reinterpret_cast<int*>(&mem_addr->firstInstance), 1.f, 0, mem_addr->instanceCount);
		ImGui::PopID();
	}
}