#include "EWEngine/NodeGraph/Node.h"

#include "imgui.h"

namespace EWE{
    namespace Node {
        bool Node::Update(Input::Mouse const& mouseData) {
            return true;
        }

#ifdef EWE_IMGUI
        void Node::Imgui() {
            const std::string extension = std::string("##") + std::to_string(reinterpret_cast<uint64_t>(this));
            std::string name = std::string("title color") + extension;
            ImGui::ColorEdit3(name.c_str(), &buffer->titleColor.x);
            name = "title scale" + extension;
            ImGui::SliderFloat(name.c_str(), &buffer->titleScale, 0.f, 1.f);
            name = "foreground color" + extension;
            ImGui::ColorEdit3(name.c_str(), &buffer->foregroundColor.x);
            name = "foreground scale" + extension;
            ImGui::SliderFloat(name.c_str(), &buffer->foregroundScale, 0.f, 1.f);
            name = "background color" + extension;
            ImGui::ColorEdit3(name.c_str(), &buffer->backgroundColor.x);
            name = "Position" + extension;
            ImGui::SliderFloat2(name.c_str(), &buffer->position.x, -1.f, 1.f);
            name = "scale" + extension;
            ImGui::SliderFloat2(name.c_str(), &buffer->scale.x, 0.f, 1.f);
        }
#endif
    }
}