#pragma once

#include "LAB/Vector.h"


#include <hb.h>
#include <hb-ft.h>

#include <ft2build.h>
#include FT_FREETYPE_H


#include <vector>
#include <unordered_map>
#include <string>
#include <cstring>
#include <cstdint>

struct Glyph {
    lab::vec2 uv0;
    lab::vec2 uv1;
    int width;
    int height;
    int bearingX;
    int bearingY;
    int advance;
};

class FontAtlas {
public:

	struct Vert {
		float x;
		float y;
		float u;
		float v;
	};

    FT_Library ft;
    FT_Face face;
    hb_font_t* hbFont;

    int atlasW = 1024;
    int atlasH = 1024;

    std::vector<uint8_t> pixels;
    std::unordered_map<uint32_t, Glyph> glyphs;

    int penX = 0;
    int penY = 0;
    int rowH = 0;
	
	FontAtlas(const char* path, int pxSize) {
        FT_Init_FreeType(&ft);
        FT_New_Face(ft, path, 0, &face);
        FT_Set_Pixel_Sizes(face, 0, pxSize);

        hbFont = hb_ft_font_create(face, NULL);

        pixels.resize(atlasW * atlasH);
        memset(pixels.data(), 0, pixels.size());
    }

    void AddGlyph(uint32_t cp) {
        FT_UInt idx = FT_Get_Char_Index(face, cp);
        if (!idx) {
			return;
		}
			
        FT_Load_Glyph(face, idx, FT_LOAD_DEFAULT);
        FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);

        FT_Bitmap* bmp = &face->glyph->bitmap;

        if (penX + bmp->width >= atlasW) {
            penX = 0;
            penY += rowH;
            rowH = 0;
        }

        for (int y = 0; y < bmp->rows; y++) {
            memcpy(
                &pixels[(penY + y) * atlasW + penX],
                &bmp->buffer[y * bmp->pitch],
                bmp->width
			);
        }

        Glyph g{
			.uv0{penX / static_cast<float>(atlasW), penY / (float)atlasH},
			.uv1{(penX + bmp->width) / static_cast<float>(atlasW), (penY + bmp->rows) / static_cast<float>(atlasH)},
			.width = bmp->width,
			.height = bmp->rows,
			.bearingX = face->glyph->bitmap_left,
			.bearingY = face->glyph->bitmap_top,
			.advance = face->glyph->advance.x >> 6
		};

        glyphs[idx] = g;

        penX += bmp->width + 1;
        rowH = std::max(rowH, (int)bmp->rows);
    }

    void PrebakeASCII() {
        for (uint32_t cp = 32; cp < 127; ++cp) {
            AddGlyph(cp);
		}
    }

    void ShapeText(const std::string& text, std::vector<Vert>& out, float x, float y) {
        hb_buffer_t* buf = hb_buffer_create();
        hb_buffer_add_utf8(buf, text.c_str(), -1, 0, -1);
        hb_buffer_guess_segment_properties(buf);

        hb_shape(hbFont, buf, NULL, 0);

        unsigned int count;
        auto info = hb_buffer_get_glyph_infos(buf, &count);
        auto pos  = hb_buffer_get_glyph_positions(buf, &count);

        float penXf = x;
        float penYf = y;

        for (unsigned int i = 0; i < count; i++) {
            uint32_t gid = info[i].codepoint;
            if (!glyphs.count(gid)) {
				continue;
			}

            Glyph& g = glyphs[gid];

            float gx = penXf + (pos[i].x_offset / 64.0f) + g.bearingX;
            float gy = penYf - (pos[i].y_offset / 64.0f) - g.bearingY;

            float w = g.width;
            float h = g.height;

            out.push_back({gx,     gy,     g.uv0.x, g.uv0.y});
            out.push_back({gx+w,   gy,     g.uv1.x, g.uv0.y});
            out.push_back({gx+w,   gy+h,   g.uv1.x, g.uv1.y});
            out.push_back({gx,     gy+h,   g.uv0.x, g.uv1.y});

            penXf += pos[i].x_advance / 64.0f;
            penYf += pos[i].y_advance / 64.0f;
        }

        hb_buffer_destroy(buf);
    }
};