#include "EWEngine/Graphics/TextPackage.h"

#include "EWEngine/Graphics/TextOverlay.h"

namespace EWE {
    uint16_t TextStruct::GetSelectionIndex(double xpos) const {
        const float charW = 1.5f * scale / engine->window.screenDimensions.width;
        const float width = GetWidth();
        float currentPos = x;

        const Font* font = Global::textOverlay->currentFont;
#if EWE_DEBUG
        Log::Debug("xpos get selection index - %.1f \n", xpos);
#endif
        switch (align) {
            case TA_left:break;
            case TA_center: currentPos -= width / 2.f; break;
            case TA_right: currentPos -= width; break;
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
        auto* font = Global::textOverlay->currentFont;
        const float textWidth = font->GetStringWidth(*this);
        if (textWidth < 0.0f) {

            Log::Error("width less than 0, what  was the string? : %s:%u \n", string.c_str(), engine->window.screenDimensions.width);
        }
        return textWidth;
#else
        return Global::textOverlay->currentFont->GetStringWidth(string, charW);
#endif
    }
}//namespace EWE