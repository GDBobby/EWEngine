#pragma once

#include "LAB/Vector.h"
#include "EWEngine/InputData.h"

#include <string>
#include <fstream>


namespace EWE{
    namespace Node{
        using NodeID = uint32_t;
        using PinID = uint32_t;
        using PinOffset = uint16_t; //this is the pin id within the node
        using LinkID = uint32_t;

        struct NodeBuffer {
            lab::vec3 titleColor;
            float titleScale; //what percentage of the foreground it takes up, vertical only
            lab::vec3 foregroundColor;
            float foregroundScale; //what percentage of the width it takes up, width and height separate

            lab::vec3 backgroundColor;

            enum ObjectType : int {
                OT_Node = 0,
                OT_Pin = 1,
            };

            int objectType;

            //i could make it a mat3x3 and precompute that on the GPU. its a bit cleaner, but uses more space (9 floats instead of 4)
            lab::vec4 position;
            lab::vec2 scale;

            void Init();
            void InitPin();

            void Serialize(std::ofstream& outFile);
            void Deserialize(std::ifstream& inFile);
        };

        struct Pin{
            std::string name;
            NodeBuffer* buffer; //sucks, but it's a decent temp measure

            NodeID parentNode; //potentially void* if not a vector for Node
            PinOffset offset_within_parent; //which pin in the parent is it?
            PinID globalPinID;

            [[nodiscard]] explicit Pin(std::string_view name, NodeBuffer* buffer, uint32_t index);
            [[nodiscard]] explicit Pin(NodeBuffer* buffer, uint32_t index);

            enum Type : uint8_t{
                InOut = 0,
                In = 1,
                Out = 2,
            };
            Type property;

            bool Update(Input::Mouse const& mouseData);
#ifdef EWE_IMGUI
            void Imgui();
#endif
        };
    }//namespace Node
}
