#pragma once

#include "EWEngine/NodeGraph/Pin.h"
#include "EWEngine/NodeGraph/Link.h"
#include "EWEngine/InputData.h"

#include "LAB/Vector.h"
#include "LAB/Transform.h"

#include <vector>
#include <string>

namespace EWE{
    namespace Node{

        struct Node{
            std::string name;
            NodeBuffer* buffer;
            uint32_t index; //into the contiguous buffer that owns this memory

            std::vector<Pin*> pins{};
            //i dont know how I want to store links yet
            std::vector<Link*> links{};

            static std::function<Pin&(std::string_view name, NodeBuffer* buffer, uint32_t index)> pin_adder;

            Pin& AddPin() {
                return *pins.emplace_back();
            }

            [[nodiscard]] explicit Node(std::string_view name, NodeBuffer* buffer, uint32_t index)
                : name{name},
                buffer{buffer},
                index{index}
            {
                buffer->Init();
            }
            [[nodiscard]] explicit Node(NodeBuffer* buffer, uint32_t index)
                : name{},
                buffer{ buffer },
                index{ index }
            {
                buffer->Init();
            }
            
            Node(Node const& copySrc) = delete;
            Node& operator=(Node const& copySrc) = delete;
            [[nodiscard]] Node(Node&& moveSrc) noexcept
                : pins{std::move(moveSrc.pins)},
                links{std::move(moveSrc.links)},
                name{std::move(moveSrc.name)}
            {
                memcpy(buffer, moveSrc.buffer, sizeof(NodeBuffer));
            }
            Node& operator=(Node&& moveSrc) noexcept {
                memcpy(buffer, moveSrc.buffer, sizeof(NodeBuffer));
                pins = std::move(moveSrc.pins);
                links = std::move(moveSrc.links);
                name = moveSrc.name;
            }

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