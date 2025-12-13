#pragma once

#include "EWEngine/NodeGraph/Pin.h"

#include "LAB/Vector.h"

#include <cstdint>

namespace EWE{
    namespace Node{
    struct Link{
        Pin* first;
        Pin* second;

        lab::vec3 color;

        //bezier
    };
    }//namespace Node
} //namespace EWE