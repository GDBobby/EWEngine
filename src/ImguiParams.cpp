#include "EWEngine/Imgui/Params.h"

namespace EWE{
    template<>
	void ImguiReflectParamStruct<Inst::Type::Push>(ParamPack<Inst::Type::Push>* temp_addr){
		GlobalPushConstant_Raw* mem_addr = reinterpret_cast<GlobalPushConstant_Raw*>(temp_addr);
		if(ImGui::BeginTable("push", 2, ImGuiTableFlags_Borders| ImGuiTableFlags_SizingFixedFit, ImVec2(300.0f, 0.0f))) {
			ImGui::TableSetupColumn("buffer addresses", ImGuiTableColumnFlags_WidthFixed, 150.0f);
			ImGui::TableSetupColumn("texture indices", ImGuiTableColumnFlags_WidthFixed, 150.0f);
            ImGui::TableHeadersRow();
			uint8_t iter = 0;
			while(iter < GlobalPushConstant_Raw::buffer_count || iter < GlobalPushConstant_Raw::texture_count){
				//this is buffer device addresses
				ImGui::TableNextColumn();
				if(iter < GlobalPushConstant_Raw::buffer_count){
					if(mem_addr->buffer_addr[iter] != null_buffer){
						auto& buf = Global::assetManager->buffer.Get(Global::assetManager->buffer.ConvertBDAToHash(mem_addr->buffer_addr[iter]));
						ImGui::Text("%u:%s", iter, buf.name.c_str());
						ImGui::SetItemTooltip(buf.name.c_str());
					}
					else{
						ImGui::Text("%u:null", iter);
					}
					Buffer* temp_drop_ptr = nullptr;
					if(DragDropPtr::Target(temp_drop_ptr)){
						mem_addr->buffer_addr[iter] = temp_drop_ptr->deviceAddress;
					}
				}
				ImGui::TableNextColumn();
				if(iter < GlobalPushConstant_Raw::texture_count){
					if(mem_addr->texture_indices[iter] != null_texture){
						const AssetHash temp_hash = Global::assetManager->dii.ConvertTextureIndexToHash(mem_addr->texture_indices[iter]);
#if EWE_DEBUG_BOOL
						if(temp_hash == Asset::INVALID_HASH) {
							Logger::Print<Logger::Warning>("caught an invalid push texture index\n");
							mem_addr->texture_indices[iter] = null_texture;
						}
						else{
#endif			
							auto& dii = Global::assetManager->dii.Get(temp_hash);
							ImGui::Text("%u:%s", iter, dii.name.c_str());
							ImGui::SetItemTooltip(dii.name.c_str());
#if EWE_DEBUG_BOOL
						}
#endif
					}
					else{
						ImGui::Text("%u:null", iter);
					}
					DescriptorImageInfo* temp_drop_ptr = nullptr;
					if(DragDropPtr::Target(temp_drop_ptr)){
						mem_addr->texture_indices[iter] = temp_drop_ptr->index;
					}
				}
				iter++;
			}
			ImGui::EndTable();
		}
	}
}