#include "EWEngine/Imgui/Params.h"

namespace EWE{
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
						auto& buf = Global::assetManager->buffer.Get(Global::assetManager->buffer.ConvertBDAToHash(push->GetDeviceAddress(iter)));
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
						const AssetHash temp_hash = Global::assetManager->dii.ConvertTextureIndexToHash(push->GetTextureIndex(iter));
#if EWE_DEBUG_BOOL
						if(temp_hash == Asset::INVALID_HASH) {
							Logger::Print<Logger::Warning>("caught an invalid push texture index\n");
							push->GetTextureIndex(iter) = null_texture;
						}
						else{
#endif
							auto* dii = Global::assetManager->dii.Get(temp_hash);
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
}