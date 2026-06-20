#include "EWEngine/Graphics/FontObject.h"

#include "EWEngine/Graphics/TextOverlay.h"

#define USING_VULKAN_CONVERSION
#include "ImageProcessor.h"

namespace EWE {

	hb_font_t* Create_hbFont(std::filesystem::path const& path, int pxSize, FT_Library& ft, FT_Face& face) {
		const std::filesystem::path full_path = Global::assetManager->root_directory / "fonts" / path;
		EWE_ASSERT(std::filesystem::exists(full_path));

		FT_Init_FreeType(&ft);
		FT_New_Face(ft, full_path.string().data(), 0, &face);
		FT_Set_Pixel_Sizes(face, 0, pxSize);

		return hb_ft_font_create(face, NULL);
	}

	FontObject::FontObject(std::filesystem::path const& path, int pxSize)
	: hbFont(Create_hbFont(path, pxSize, ft, face))
	{
		pixels.resize(1024 * 1024, std::byte{0});

		PrebakeASCII();
	}

	void FontObject::AddGlyph(uint32_t cp) {

		FT_UInt idx = FT_Get_Char_Index(face, cp);
		if (!idx) {
			return;
		}
		FT_Load_Glyph(face, idx, FT_LOAD_DEFAULT);
		FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
		FT_Bitmap* bmp = &face->glyph->bitmap;

		if (penX + static_cast<int>(bmp->width) >= atlasW) {
			penX = 0;
			penY += cellHeight;
		}

		if (penY + cellHeight >= atlasH) {
			Log::Debug("attempted to fill up font texture more than allowed\n");
			return;
		}

		// where this glyph's baseline sits within the shared cell
		const int glyphTop = face->glyph->bitmap_top;
		const int dstRowStart = baselineOffset - glyphTop;

		for (uint32_t y = 0; y < bmp->rows; y++) {
			const int dstY = penY + dstRowStart + y;
			if (dstY < 0 || dstY >= atlasH) continue;
			std::memcpy(
				&pixels[dstY * atlasW + penX],
				&bmp->buffer[y * bmp->pitch],
				bmp->width
			);
		}

		glyphs[idx] = Glyph{
			.uv0{penX / static_cast<float>(atlasW), penY},
			.uv1{(penX + bmp->width) / static_cast<float>(atlasW), penY + cellHeight},
			.width = bmp->width,
			.height = bmp->rows,
			.bearingX = face->glyph->bitmap_left,
			.bearingY = face->glyph->bitmap_top,
			.advance = face->glyph->advance.x >> 6
		};

		penX += bmp->width + 1;
	}

	void FontObject::PrebakeASCII() {
		cellHeight = static_cast<int>((face->size->metrics.ascender - face->size->metrics.descender) >> 6);
		baselineOffset = static_cast<int>(face->size->metrics.ascender >> 6);

		for (uint32_t cp = 32; cp < 127; ++cp) {
			AddGlyph(cp);
		}
		const float height = static_cast<float>(GetHeight());
		for (auto& glyph : glyphs) {
			glyph.second.uv0.y /= height;
			glyph.second.uv1.y /= height;
			std::swap(glyph.second.uv0.y, glyph.second.uv1.y);
		}
	}

	uint16_t FontObject::ShapeText(TextStruct const& ts, Vert* out) {
		hb_buffer_t* buf = hb_buffer_create();
	    hb_buffer_add_utf8(buf, ts.string.c_str(), -1, 0, -1);
	    hb_buffer_guess_segment_properties(buf);
	    hb_shape(hbFont, buf, NULL, 0);

	    uint32_t count;
	    const auto info = hb_buffer_get_glyph_infos(buf, &count);
	    const auto pos  = hb_buffer_get_glyph_positions(buf, &count);

	    float totalAdvance = 0.0f;
	    for (uint32_t i = 0; i < count; i++) {
	        totalAdvance += (pos[i].x_advance / 64.0f) * ts.scale;
	    }

	    float startX = ts.x;
	    switch (ts.align) {
	        case TA_center:
	            startX = ts.x - (totalAdvance * 0.5f);
	            break;
	        case TA_right:
	            startX = ts.x - totalAdvance;
	            break;
	        case TA_left:
	        default:
	            break;
	    }

	    float penXf = startX;
	    float penYf = ts.y;
	    uint16_t vertIndex = 0;
		uint16_t instanceCount = 0;

	    for (uint32_t i = 0; i < count; i++) {
	        uint32_t gid = info[i].codepoint;
	        auto it = glyphs.find(gid);
	        if (it == glyphs.end()) {
	            penXf += pos[i].x_advance / 64.0f * ts.scale;
	            penYf += pos[i].y_advance / 64.0f * ts.scale;
	            continue;
	        }
	    	instanceCount++;
	        Glyph& g = it->second;

	    	const float left   = (penXf + ((pos[i].x_offset / 64.0f) + g.bearingX) * ts.scale) / static_cast<float>(engine->window.screenDimensions.width);
	    	const float right  = left + (g.width * ts.scale) / static_cast<float>(engine->window.screenDimensions.width);

	    	const float descentPx = static_cast<float>(cellHeight - baselineOffset);
	    	const float bottom = (penYf - (pos[i].y_offset / 64.0f) * ts.scale) / static_cast<float>(engine->window.screenDimensions.height);
	    	const float top    = bottom - (g.height + descentPx * 4.f) * ts.scale / static_cast<float>(engine->window.screenDimensions.height);

	    	out[vertIndex].data.position = lab::vec3{left,  bottom, ts.z};
	    	out[vertIndex].data.uv = lab::vec2{g.uv0.x, g.uv0.y};
	    	vertIndex++;
	    	out[vertIndex].data.position = lab::vec3{right, bottom, ts.z};
	    	out[vertIndex].data.uv = lab::vec2{g.uv1.x, g.uv0.y};
	    	vertIndex++;
	    	out[vertIndex].data.position = lab::vec3{left,  top, ts.z};
	    	out[vertIndex].data.uv = lab::vec2{g.uv0.x, g.uv1.y};
	    	vertIndex++;
	    	out[vertIndex].data.position = lab::vec3{right, top, ts.z};
	    	out[vertIndex].data.uv = lab::vec2 {g.uv1.x, g.uv1.y};
	    	vertIndex++;

	        penXf += (pos[i].x_advance / 64.0f) * ts.scale;
	        penYf += (pos[i].y_advance / 64.0f) * ts.scale;
	    }
	    hb_buffer_destroy(buf);

		//if the glyph isnt' found, it's skipped, and won't be added to instance count
		return instanceCount;
	}

	float FontObject::GetStringWidth(TextStruct const& ts) const {
		hb_buffer_t* buf = hb_buffer_create();
		hb_buffer_add_utf8(buf, ts.string.c_str(), -1, 0, -1);
		hb_buffer_guess_segment_properties(buf);
		hb_shape(hbFont, buf, NULL, 0);

		uint32_t count;
		auto pos = hb_buffer_get_glyph_positions(buf, &count);

		float totalAdvance = 0.f;
		for (uint32_t i = 0; i < count; i++) {
			totalAdvance += (pos[i].x_advance / 64.0f) * ts.scale;
		}

		hb_buffer_destroy(buf);
		return totalAdvance;
	}

} //namespace EWE