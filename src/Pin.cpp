#include "EWEngine/NodeGraph/Pin.h"

namespace EWE{
        bool Pin::Update(Input::Mouse const& mouseData) {
            return true;
        }
#ifdef EWE_IMGUI
        void Pin::Imgui() {
            const std::string extension = std::string("##") + std::to_string(reinterpret_cast<uint64_t>(this));
            std::string name = std::string("color") + extension;
            ImGui::ColorEdit3(name.c_str(), &buffer->foregroundColor.x);
            name = "Position" + extension;
            ImGui::SliderFloat2(name.c_str(), &buffer->position.x, -1.f, 1.f);
            name = "scale" + extension;
            ImGui::SliderFloat2(name.c_str(), &buffer->scale.x, 0.f, 1.f);
        }
#endif
}