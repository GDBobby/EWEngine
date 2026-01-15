#pragma once

#include "EightWinds/CommandBuffer.h"

#include "EWEngine/NodeGraph/Pin.h"
#include "EWEngine/NodeGraph/Link.h"
#include "EWEngine/InputData.h"

#include "LAB/Vector.h"
#include "LAB/Transform.h"

#include <vector>
#include <string>

namespace EWE{
    namespace Node{
        struct NodeBuffer{
            lab::vec3 titleColor;
            float titleScale; //what percentage of the foreground it takes up, vertical only
            lab::vec3 foregroundColor;
            float foregroundScale; //what percentage of the width it takes up, width and height separate
            
            lab::vec3 backgroundColor;

            //i could make it a mat3x3 and precompute that on the GPU. its a bit cleaner, but uses more space (9 floats instead of 4)
            lab::vec2 position;
            lab::vec2 scale;

            void Init() {
                titleColor = lab::vec3(1.f, 0.f, 0.f);
                titleScale = 0.1f;
                foregroundColor = lab::vec3(0.f, 0.f, 1.f);
                foregroundScale = 0.9f;
                backgroundColor = lab::vec3(0.f, 1.f, 0.f);

                position = lab::vec2(0.f);
                scale = lab::vec2(1.f);
            }
        };

        struct Node{
            std::vector<Pin> pins{};
            //i dont know how I want to store links yet
            std::vector<Link*> links{};

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
            Node(Node&& moveSrc) noexcept
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

            std::string name;

            NodeBuffer* buffer;
            uint32_t index; //into the contiguous buffer that owns this memory

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