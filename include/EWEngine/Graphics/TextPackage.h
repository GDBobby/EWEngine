#pragma once

#include <cstdint>
#include <string>

namespace EWE {
    enum TextAlign : uint8_t { TA_left, TA_center, TA_right };
    struct TextStruct {
        std::string string{""};
        float x{ 0.f };
        float y{ 0.f };
        float z{1.f}; //depth buffer depth
        TextAlign align{ TA_left };
        float scale{ 1.f };
        uint16_t GetSelectionIndex(double xpos) const;
        float GetWidth() const;
    };
}//namespace EWE