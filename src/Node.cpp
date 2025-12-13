#include "EWEngine/NodeGraph/Node.h"

#include "imgui.h"

namespace EWE{
    namespace Node {
        bool Node::Update(Input::Mouse const& mouseData) {
            return true;
        }

#ifdef EWE_IMGUI
        void Node::Imgui() {
            ImGui::ColorEdit3("title color", &buffer->titleColor.x);
        }
#endif
    }
}