#include "EWEngine/NodeGraph/Pin.h"

#include "EightWinds/Data/StreamHelper.h"

#ifdef EWE_IMGUI
#include "imgui.h"
#endif

namespace EWE {
    namespace Node {


        void NodeBuffer::Serialize(std::ofstream& outFile) {
            if (objectType == OT_Node) {
                Stream::Helper(outFile, titleColor);
                Stream::Helper(outFile, titleScale);
                Stream::Helper(outFile, foregroundColor);
                Stream::Helper(outFile, foregroundScale);
                Stream::Helper(outFile, backgroundColor);
                Stream::Helper(outFile, position);
                Stream::Helper(outFile, scale);
            }
            else if (objectType == OT_Pin) {
                Stream::Helper(outFile, foregroundColor);
                Stream::Helper(outFile, position);
                Stream::Helper(outFile, scale);
            }
        }
        void NodeBuffer::Deserialize(std::ifstream& inFile) {
            if (objectType == OT_Node) {
                Stream::Helper(inFile, titleColor);
                Stream::Helper(inFile, titleScale);
                Stream::Helper(inFile, foregroundColor);
                Stream::Helper(inFile, foregroundScale);
                Stream::Helper(inFile, backgroundColor);
                Stream::Helper(inFile, position);
                Stream::Helper(inFile, scale);
            }
            else if (objectType == OT_Pin) {
                Stream::Helper(inFile, foregroundColor);
                Stream::Helper(inFile, position);
                Stream::Helper(inFile, scale);
            }
        }

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