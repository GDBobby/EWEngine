#pragma once

#include <cstdint>
#include <string>

namespace EWE {

    struct FontKey;
    struct Font;

    enum TextAlign : uint8_t { left, center, right };
    struct TextStruct {
        std::string string{""};
        lab::vec3 pos{0.f, 0.f, 1.f};
        TextAlign align{ TextAlign::left };
        float scale{ 1.f };

        Font& font; //use the key or use a Font* ptr?
        //void AddText(); //could move this here, but not rn at least

        [[nodiscard]] TextStruct(std::string_view string, lab::vec3 pos, TextAlign align, float scale, FontKey fontKey);
        [[nodiscard]] TextStruct(std::string_view string, lab::vec3 pos, TextAlign align, float scale, Font& font);
        [[nodiscard]] TextStruct(std::string_view string, lab::vec3 pos, TextAlign align, float scale, std::string_view font_name, uint8_t font_size);

        [[nodiscard]] TextStruct(TextStruct const& copySrc);
        [[nodiscard]] TextStruct(TextStruct&& moveSrc);
        TextStruct& operator=(TextStruct const& copySrc);
        TextStruct& operator=(TextStruct&& moveSrc);

        uint16_t GetSelectionIndex(double xpos) const;
        float GetWidth() const;
    };
}//namespace EWE