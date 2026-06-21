#pragma once

#include "EWEngine/Graphics/TextPackage.h"
#include "EWEngine/Data/Vertex.h"

#include "LAB/Vector.h"


#include <hb.h>
#include <hb-ft.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <vector>
#include <unordered_map>
#include <string>


namespace EWE{
	static constexpr VertexProperty FontVertexProperty{
		.position = 3,
		.uv = true
	};

	struct Glyph {
	    lab::vec2 uv0;
	    lab::vec2 uv1;
	    float width;
	    float height;
	    float bearingX;
	    float bearingY;
	    float advance;
	};

	struct FontObject {

		using Vert = Vertex<FontVertexProperty>;

	    FT_Library ft;
	    FT_Face face;
	    hb_font_t* hbFont;

	    int atlasW = 1024; //remains in pixel space
	    int atlasH = 1024; //remains in pixel space

	    std::vector<std::byte> pixels;
	    std::unordered_map<uint32_t, Glyph> glyphs;

	    int penX = 0;
	    int penY = 0;
	    int cellHeight = 0;
		int baselineOffset = 0;

		int GetHeight() const {
			return lab::Min(penY + cellHeight, atlasH);
		}
		VkDeviceSize GetDeviceSize() const {
			return lab::Min(static_cast<std::size_t>(GetHeight() * atlasW), pixels.size());
		}

		[[nodiscard]] FontObject(std::filesystem::path const& path, int pxSize);
		FontObject(FontObject const& copySrc) = delete;
		FontObject(FontObject&& moveSrc) = delete;

		FontObject& operator=(const FontObject& copySrc) = delete;
		FontObject& operator=(FontObject&& moveSrc) = delete;

	    void AddGlyph(uint32_t cp);
	    void PrebakeASCII();
	    uint16_t ShapeText(TextStruct const& textStruct, Vert* out);
		float GetStringWidth(TextStruct const& ts) const;
	};
}