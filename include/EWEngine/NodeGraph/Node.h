#pragma once

#include "EWEngine/NodeGraph/Pin.h"
#include "EWEngine/NodeGraph/Link.h"
#include "EWEngine/InputData.h"

#include "EightWinds/Data/RuntimeArray.h"
#include "EWEngine/NodeGraph/ContiguousContainer.h"

#include "LAB/Vector.h"
#include "LAB/Transform.h"

#include <vector>
#include <string>

namespace EWE{
    namespace Node{

        struct Node{
            std::string name;
            NodeID index; //into the contiguous buffer that owns this memory

            NodeBuffer* buffer;

            std::vector<uint32_t> pins{};
            //i dont know how I want to store links yet
            std::vector<uint32_t> links{};
            ContiguousContainer<Pin>& upper_pins;

            [[nodiscard]] explicit Node(std::string_view name, NodeBuffer* buffer, uint32_t index, ContiguousContainer<Pin>& upper_pins);
            [[nodiscard]] explicit Node(NodeBuffer* buffer, uint32_t index, ContiguousContainer<Pin>& upper_pins);
            
            Node(Node const& copySrc) = delete;
            Node& operator=(Node const& copySrc) = delete;
            [[nodiscard]] Node(Node&& moveSrc) noexcept;
            Node& operator=(Node&& moveSrc) noexcept;

            bool Update(Input::Mouse const& mouseData);

            bool Complete() const {
                return pins.size() == links.size();
            }

#ifdef EWE_IMGUI
            void Imgui();
#endif
        };
    }
}