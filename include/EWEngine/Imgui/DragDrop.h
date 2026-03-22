#pragma once

#include "EWEngine/Preprocessor.h"
#include "EightWinds/Reflect/Reflect.h"

#if EWE_IMGUI
#include "imgui.h"

namespace EWE{
	struct DragDropPtr{
		template<typename T>
        static bool Source(T& obj){
            if(ImGui::BeginDragDropSource()){
                //Logger::Print<Logger::Debug>("sourced : %s\n", Reflect::GetName<^^T>().data());
                std::size_t reint_ptr = reinterpret_cast<std::size_t>(&obj);
                ImGui::SetDragDropPayload(Reflect::GetName<^^T>().data(), &reint_ptr, sizeof(std::size_t));
                ImGui::Text("dragging %s", Reflect::GetName<^^T>().data());
                ImGui::EndDragDropSource();
                return true;
            }
            return false;
        }

		template<typename T>
        static bool Target(T*& obj){
            if(ImGui::BeginDragDropTarget()){
                //Logger::Print<Logger::Debug>("targeted : %s\n", Reflect::GetName<^^T>().data());
                if (const ImGuiPayload* dragdrop_payload = ImGui::AcceptDragDropPayload(Reflect::GetName<^^T>().data())) {
                    obj = reinterpret_cast<T*>(*reinterpret_cast<std::size_t*>(dragdrop_payload->Data));
                    ImGui::EndDragDropTarget();
                    return true;
                }
                
                ImGui::EndDragDropTarget();
            }
            return false;
        }
	};

} //namespace EWE
#endif