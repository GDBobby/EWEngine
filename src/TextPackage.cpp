#include "EWEngine/Graphics/TextPackage.h"

#include "EWEngine/Graphics/TextOverlay.h"

namespace EWE {

    inline float NormalizeSize(float& scale, uint8_t& font_size){
        static constexpr uint8_t kFontSizes[] = { 12, 16, 24, 32, 48, 64, 96, 128 };
        static constexpr size_t kCount = sizeof(kFontSizes) / sizeof(kFontSizes[0]);

        const float desired_size = static_cast<float>(font_size) * scale;

        if (desired_size <= kFontSizes[0]) {
            font_size = kFontSizes[0];
            return desired_size / static_cast<float>(font_size);
        }
        if (desired_size >= kFontSizes[kCount - 1]) {
            font_size = kFontSizes[kCount - 1];
            return desired_size / static_cast<float>(font_size);
        }

        for (std::size_t i = 0; i + 1 < kCount; ++i) {
            uint8_t lo = kFontSizes[i];
            uint8_t hi = kFontSizes[i + 1];
            if (desired_size >= lo && desired_size <= hi) {
                float scaleLo = desired_size / static_cast<float>(lo);
                float scaleHi = desired_size / static_cast<float>(hi);

                //prefer slightly downscaling over large upscale
                if (scaleHi >= 1.0f / 1.25f) { 
                    font_size = hi;
                    return scaleHi;
                }
                font_size = lo;
                return scaleLo;
            }
        }

        // unreachable
        EWE_UNREACHABLE;
        return scale;
    }

    TextStruct::TextStruct(std::string_view _string, lab::vec3 _pos, TextAlign _align, float _scale, Font& _font)
    : string{_string},
        pos{_pos},
        align{_align},
        scale{_scale},
        font{_font}
    {}
    TextStruct::TextStruct(std::string_view _string, lab::vec3 _pos, TextAlign _align, float _scale, FontKey fontKey)
    : string{_string},
        pos{_pos},
        align{_align},
        scale{NormalizeSize(_scale, fontKey.size)},
        font{Global::textOverlay->GetFont(fontKey)}
    {}
    TextStruct::TextStruct(std::string_view _string, lab::vec3 _pos, TextAlign _align, float _scale, std::string_view font_name, uint8_t font_size)
        : string{_string},
        pos{_pos},
        align{_align},
        scale{NormalizeSize(_scale, font_size)},
        font{Global::textOverlay->GetFont(font_name, font_size)}
    {}


    TextStruct::TextStruct(TextStruct const& copySrc) 
    : string{copySrc.string},
        pos{copySrc.pos},
        align{copySrc.align},
        scale{copySrc.scale},
        font{copySrc.font}
    {}
    TextStruct::TextStruct(TextStruct&& moveSrc)
    : string{moveSrc.string},
        pos{moveSrc.pos},
        align{moveSrc.align},
        scale{moveSrc.scale},
        font{moveSrc.font}
    {}
    TextStruct& TextStruct::operator=(TextStruct const& copySrc){
        string = copySrc.string;
        pos = copySrc.pos;
        align = copySrc.align;
        scale = copySrc.scale;
        //assert the font? or let it be?
        return *this;
    }
    TextStruct& TextStruct::operator=(TextStruct&& moveSrc){
        string = moveSrc.string;
        pos = moveSrc.pos;
        align = moveSrc.align;
        scale = moveSrc.scale;
        //assert the font? or let it be?
        return *this;
    }

    uint16_t TextStruct::GetSelectionIndex(double xpos) const {
        const float charW = 1.5f * scale / engine->window.screenDimensions.width;
        const float width = GetWidth();
        float currentPos = pos.x;

#if EWE_DEBUG
        Log::Debug("xpos get selection index - %.1f \n", xpos);
#endif
        switch (align) {
            case TextAlign::left:break;
            case TextAlign::center: currentPos -= width / 2.f; break;
            case TextAlign::right: currentPos -= width; break;
        }

        //float lastPos = currentPos;
        Log::Warning("replace this\n");
        /*
        for (uint16_t i = 0; i < string.length(); i++) {
            currentPos += font->GetStringWidth(string[i], charW) * engine->window.screenDimensions.width / 8.f;
#if EWE_DEBUG
            Log::Debug("currentPos : %.2f \n", currentPos);
#endif
            if (xpos <= currentPos) { return i; }
            currentPos += font->GetStringWidth(string[i], charW) * engine->window.screenDimensions.width * 3.f / 8.f;
        }
        */
        return static_cast<uint16_t>(string.length());
    }

    float TextStruct::GetWidth() const {
        const float charW = 1.5f * scale / engine->window.screenDimensions.width;
#if EWE_DEBUG
        const float textWidth = font.GetStringWidth(*this);
        if (textWidth < 0.0f) {

            Log::Error("width less than 0, what  was the string? : %s:%u \n", string.c_str(), engine->window.screenDimensions.width);
        }
        return textWidth;
#else
        return font.GetStringWidth(string, charW);
#endif
    }
}//namespace EWE