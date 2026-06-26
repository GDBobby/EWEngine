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
			.uv0{static_cast<float>(penX) / static_cast<float>(atlasW), penY},
			.uv1{static_cast<float>(penX + bmp->width) / static_cast<float>(atlasW), penY + cellHeight},
			.width = static_cast<float>(bmp->width) / DEFAULT_WIDTH<float>,
			.height = static_cast<float>(bmp->rows) / DEFAULT_HEIGHT<float>,
			.bearingX = static_cast<float>(face->glyph->bitmap_left) / DEFAULT_WIDTH<float>,
			.bearingY = static_cast<float>(face->glyph->bitmap_top) / DEFAULT_HEIGHT<float>,
			.advance = static_cast<float>(face->glyph->advance.x >> 6) / DEFAULT_WIDTH<float>
		};

		penX += bmp->width + 1;
	}

	void FontObject::PrebakeASCII() {
		cellHeight = static_cast<float>((face->size->metrics.ascender - face->size->metrics.descender) >> 6);
		baselineOffset = static_cast<float>(face->size->metrics.ascender >> 6);

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
	        totalAdvance += (static_cast<float>(pos[i].x_advance) / 64.f) * ts.scale / DEFAULT_WIDTH<float>;
	    }

	    float startX = ts.pos.x;
	    switch (ts.align) {
	        case TextAlign::center:
	            startX = ts.pos.x - (totalAdvance * 0.5f);
	            break;
	        case TextAlign::right:
	            startX = ts.pos.x - totalAdvance;
	            break;
	        case TextAlign::left:
	        default:
	            break;
	    }

	    float penXf = startX;
	    float penYf = ts.pos.y;
	    uint16_t vertIndex = 0;
		uint16_t instanceCount = 0;

	    for (uint32_t i = 0; i < count; i++) {
	        uint32_t gid = info[i].codepoint;
	        auto it = glyphs.find(gid);
			auto& glyph_pos = pos[i];
	        if (it == glyphs.end()) {
	            penXf += static_cast<float>(glyph_pos.x_advance) / 64.f * ts.scale / DEFAULT_WIDTH<float>;
	            penYf += static_cast<float>(glyph_pos.y_advance) / 64.f * ts.scale / DEFAULT_HEIGHT<float>;
	            continue;
	        }
	    	instanceCount++;
	        Glyph& g = it->second;


	    	const float left   = (penXf + ((static_cast<float>(glyph_pos.x_offset) / 64.0f / DEFAULT_WIDTH<float>) + g.bearingX) * ts.scale);
	    	const float right  = left + (g.width * ts.scale);

	    	const float descentPx = static_cast<float>(cellHeight - baselineOffset) / DEFAULT_HEIGHT<float>;


			static constexpr float descent_multiplier = 4.f;

            const float half_height = (g.height + descentPx * descent_multiplier) * ts.scale / 2.f;
			const float y_starting = penYf - (static_cast<float>(glyph_pos.y_offset) / 64.0f / DEFAULT_HEIGHT<float>) * ts.scale;

	    	const float bottom = y_starting - half_height;
	    	const float top    = y_starting + half_height;

	    	out[vertIndex].data.position = lab::vec3{left,  bottom, ts.pos.z};
	    	out[vertIndex].data.uv = lab::vec2{g.uv0.x, g.uv0.y};
	    	vertIndex++;
	    	out[vertIndex].data.position = lab::vec3{right, bottom, ts.pos.z};
	    	out[vertIndex].data.uv = lab::vec2{g.uv1.x, g.uv0.y};
	    	vertIndex++;
	    	out[vertIndex].data.position = lab::vec3{left,  top, ts.pos.z};
	    	out[vertIndex].data.uv = lab::vec2{g.uv0.x, g.uv1.y};
	    	vertIndex++;
	    	out[vertIndex].data.position = lab::vec3{right, top, ts.pos.z};
	    	out[vertIndex].data.uv = lab::vec2 {g.uv1.x, g.uv1.y};
	    	vertIndex++;

	        penXf += static_cast<float>(glyph_pos.x_advance) / 64.0f * ts.scale / DEFAULT_WIDTH<float>;
	        penYf += static_cast<float>(glyph_pos.y_advance) / 64.0f * ts.scale / DEFAULT_HEIGHT<float>;
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