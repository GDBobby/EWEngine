#include "EWEngine/NodeGraph/Pin.h"

#ifdef EWE_IMGUI
#include "imgui.h"
#endif

namespace EWE {
    namespace Node {

        void NodeBuffer::Init() {
            titleColor = lab::vec3(1.f, 0.f, 0.f);
            titleScale = 0.9f;
            foregroundColor = lab::vec3(0.f, 0.f, 1.f);
            foregroundScale = 0.975f;
            backgroundColor = lab::vec3(0.f, 1.f, 0.f);

            objectType = OT_Node;

            position = lab::vec4(0.f);
            scale = lab::vec2(0.15f);
            /*
            constexpr std::size_t stride = sizeof(NodeBuffer);
            constexpr auto offset = std::array<std::size_t, 8>{
                offsetof(NodeBuffer, titleColor),
                offsetof(NodeBuffer, titleScale),
                offsetof(NodeBuffer, foregroundColor),
                offsetof(NodeBuffer, foregroundScale),
                offsetof(NodeBuffer, backgroundColor),
                offsetof(NodeBuffer, objectType),
                offsetof(NodeBuffer, position),
                offsetof(NodeBuffer, scale)

            };
            */
        }
        void NodeBuffer::InitPin() {
            foregroundColor = lab::vec3(0.3f, 0.3f, 0.5f);

            objectType = OT_Pin;

            position = lab::vec4(0.f);
            scale = lab::vec2(0.03f);
        }


        Pin::Pin(std::string_view name, NodeBuffer* buffer, PinID index, NodeID parentNode)
            :name{ name },
            buffer{ buffer },
            parentNode{parentNode},
            globalPinID{ index }

        {}

        Pin::Pin(NodeBuffer* buffer, PinID index, NodeID parentNode)
            :name{},
            buffer{ buffer },
            parentNode{ parentNode },
            globalPinID{ index } 
        {}

        bool Pin::Update(Input::Mouse const& mouseData) {
            return true;
        }
#ifdef EWE_IMGUI
        void Pin::Imgui() {
            const std::string pin_name = "pin[" + std::to_string(globalPinID) + ':' + std::to_string(offset_within_parent) + ']';
            ImGui::PushID(globalPinID);
            ImGui::Text(pin_name.c_str());
            ImGui::ColorEdit3("color", &buffer->foregroundColor.x);
            ImGui::SliderFloat4("Position", &buffer->position.x, -1.f, 1.f);
            ImGui::SliderFloat2("scale", &buffer->scale.x, 0.f, 1.f);
            ImGui::PopID();
        }
#endif
    }
}