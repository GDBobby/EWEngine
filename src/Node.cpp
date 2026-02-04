#include "EWEngine/NodeGraph/Node.h"

#ifdef EWE_IMGUI
#include "imgui.h"
#endif

namespace EWE{
    namespace Node {
        Node::Node(std::string_view name, NodeBuffer* buffer, uint32_t index, ContiguousContainer<Pin>& upper_pins)
            : name{ name },
            buffer{ buffer },
            index{ index },
            upper_pins{ upper_pins }
        {
            buffer->Init();
        }
        Node::Node(NodeBuffer* buffer, uint32_t index, ContiguousContainer<Pin>& upper_pins)
            : name{},
            buffer{ buffer },
            index{ index },
            upper_pins{ upper_pins }
        {
            buffer->Init();
        }
        Node::Node(Node&& moveSrc) noexcept
            : name{ std::move(moveSrc.name) },
            buffer{ moveSrc.buffer },
            index{ moveSrc.index },
            pins{ std::move(moveSrc.pins) },
            links{ std::move(moveSrc.links) },
            upper_pins{ moveSrc.upper_pins }

        {
            //moveSrc.buffer = nullptr;
            //doesnt matter ^ buffer is just an index into an array of trivially constructed memory
        }
        Node& Node::operator=(Node&& moveSrc) noexcept {
            memcpy(buffer, moveSrc.buffer, sizeof(NodeBuffer));
            //destroy old pins?
            pins = std::move(moveSrc.pins);
            links = std::move(moveSrc.links);
            name = moveSrc.name;
            return *this;
        }


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
            lab::vec4 oldPos = buffer->position;
            if (ImGui::SliderFloat4(name.c_str(), &buffer->position.x, -1.f, 1.f)) {
                auto pos_diff = oldPos - buffer->position;
                for (auto& pin_index : pins) {
                    upper_pins[pin_index].buffer->position -= pos_diff;
                }
            }
            name = "scale" + extension;
            lab::vec2 oldScale = buffer->scale;
            if (ImGui::SliderFloat2(name.c_str(), &buffer->scale.x, 0.f, 1.f)) {
                auto scale_diff = oldScale - buffer->scale;
                for (auto& pin_index : pins) {
                    upper_pins[pin_index].buffer->scale -= scale_diff;
                }
            }
        }
#endif
    }
}