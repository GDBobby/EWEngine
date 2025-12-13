#pragma once

#include "LAB/Vector.h"

#include <string>


namespace EWE{
    namespace Node{
        struct Pin{
            std::string name;

            lab::vec3 color;

            uint32_t parentIndex; //potentially void* if not a vector for Node
            uint16_t parentOffset; //which pin in the parent is it?

            //relative to parent, -1 being top left and 1 being bottom right
            lab::vec2 position; 

            enum Type : uint8_t{
                InOut = 0,
                In = 1,
                Out = 2,
            };
            Type property;
        };
    }//namespace Node
}
