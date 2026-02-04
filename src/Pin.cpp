#include "EWEngine/NodeGraph/Pin.h"

#ifdef EWE_IMGUI
#include "imgui.h"
#endif

namespace EWE {
    namespace Node {
        Pin::Pin(std::string_view name, NodeBuffer* buffer, uint32_t index)
            :name{ name },
            buffer{ buffer },
            globalPinID{ index }
        {

        }
        Pin::Pin(NodeBuffer* buffer, uint32_t index)
            :name{},
            buffer{ buffer },
            globalPinID{ index } {

        }

        bool Pin::Update(Input::Mouse const& mouseData) {
            return true;
        }
#ifdef EWE_IMGUI
        void Pin::Imgui() {
            const std::string pin_name = "pin[" + std::to_string(globalPinID) + ':' + std::to_string(offset_within_parent) + ']';
            ImGui::Text(pin_name.c_str());
            const std::string extension = std::string("##") + std::to_string(reinterpret_cast<uint64_t>(this));
            std::string name = std::string("color") + extension;
            ImGui::ColorEdit3(name.c_str(), &buffer->foregroundColor.x);
            name = "Position" + extension;
            ImGui::SliderFloat4(name.c_str(), &buffer->position.x, -1.f, 1.f);
            name = "scale" + extension;
            ImGui::SliderFloat2(name.c_str(), &buffer->scale.x, 0.f, 1.f);
        }
#endif
    }
}