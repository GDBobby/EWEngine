#pragma once

#include "EightWinds/Data/Address.h"

#include <cstdint>

namespace EWE {
    template<typename T>
    struct ConstructionDelayer{
        using underlying_type = T;
        alignas(T) std::byte data[sizeof(T)];

        bool constructed = false;

        T* GetPtr(){
            return reinterpret_cast<T*>(data);
        }
        T& GetRef(){
            return *GetPtr();
        }

        Address GetAddress(){
            return Address{data};
        }
    };
}