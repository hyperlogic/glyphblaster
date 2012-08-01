#include "glyphblaster.h"
#include "glyphcache.h"
#include <stdlib.h>
#include <assert.h>
#include "uthash/src/utlist.h"

GB_ERROR GB_ContextMake(GB_CONTEXT** gb_out)
{
    GB_CONTEXT* gb = (GB_CONTEXT*)malloc(sizeof(GB_CONTEXT));
    if (gb) {
        memset(gb, 0, sizeof(GB_CONTEXT));
        gb->rc = 1;
        if (FT_Init_FreeType(&gb->ft_library)) {
            return GB_ERROR_FTERR;
        }
        GB_GLYPH_CACHE* glyph_cache = NULL;
        GB_ERROR err = GB_GlyphCache_Make(&glyph_cache);
        if (err == GB_ERROR_NONE) {
            gb->glyph_cache = glyph_cache;
        }
        gb->font_list = NULL;
        *gb_out = gb;
        return err;
    } else {
        return GB_ERROR_NOMEM;
    }
}

GB_ERROR GB_ContextReference(GB_CONTEXT* gb)
{
    if (gb) {
        assert(gb->rc > 0);
        gb->rc++;
        return GB_ERROR_NONE;
    } else {
        return GB_ERROR_INVAL;
    }
}

static void _GB_ContextDestroy(GB_CONTEXT* gb)
{
    assert(gb);
    if (gb->ft_library) {
        FT_Done_FreeType(gb->ft_library);
    }
    GB_GlyphCache_Free((GB_GLYPH_CACHE*)gb->glyph_cache);
    free(gb);
}

GB_ERROR GB_ContextRelease(GB_CONTEXT* gb)
{
    if (gb) {
        gb->rc--;
        assert(gb->rc >= 0);
        if (gb->rc == 0) {
            _GB_ContextDestroy(gb);
        }
        return GB_ERROR_NONE;
    } else {
        return GB_ERROR_INVAL;
    }
}

GB_ERROR GB_FontMake(GB_CONTEXT* gb, const char* filename, uint32_t point_size,
                     GB_FONT** font_out)
{
    if (gb && filename && font_out) {
        GB_FONT* font = (GB_FONT*)malloc(sizeof(GB_FONT));
        if (font) {
            memset(font, 0, sizeof(GB_FONT));

            font->rc = 1;

            // create freetype face
            FT_New_Face(gb->ft_library, "dejavu-fonts-ttf-2.33/ttf/DejaVuSans.ttf", 0, &font->ft_face);
            FT_Set_Char_Size(font->ft_face, (int)(point_size * 64), 0, 72, 72);

            // create harfbuzz font
            font->hb_font = hb_ft_font_create(font->ft_face, 0);
            hb_ft_font_set_funcs(font->hb_font);

            font->glyph_hash = NULL;

            // context holds a list of all fonts
            DL_PREPEND(gb->font_list, font);

            *font_out = font;
            return GB_ERROR_NONE;
        } else {
            return GB_ERROR_NOMEM;
        }
    } else {
        return GB_ERROR_INVAL;
    }
}

GB_ERROR GB_FontReference(GB_CONTEXT* gb, GB_FONT* font)
{
    if (gb && font) {
        assert(font->rc > 0);
        font->rc++;
        return GB_ERROR_NONE;
    } else {
        return GB_ERROR_INVAL;
    }
}

static void _GB_FontDestroy(GB_CONTEXT* gb, GB_FONT* font)
{
    assert(font);
    assert(font->rc == 0);

    // destroy freetype face
    if (font->ft_face) {
        FT_Done_Face(font->ft_face);
    }

    // destroy harfbuzz font
    if (font->hb_font) {
        hb_font_destroy(font->hb_font);
    }

    // context holds a list of all fonts
    DL_DELETE(gb->font_list, font);

    free(font);
}

GB_ERROR GB_FontRelease(GB_CONTEXT* gb, GB_FONT* font)
{
    if (gb && font) {
        font->rc--;
        assert(font->rc >= 0);
        if (font->rc == 0) {
            _GB_FontDestroy(gb, font);
        }
        return GB_ERROR_NONE;
    } else {
        return GB_ERROR_INVAL;
    }
}

static void _GB_UpdateGlyphCacheFromBuffer(GB_CONTEXT* gb, GB_TEXT* text)
{
    // prepare to iterate over all the glyphs in the hb_buffer
    int num_glyphs = hb_buffer_get_length(text->hb_buffer);
    hb_glyph_info_t* glyphs = hb_buffer_get_glyph_infos(text->hb_buffer, NULL);

    // the font keeps a hash of all glyphs in use.
    GB_GLYPH* glyph_hash = text->font->glyph_hash;

    // hold a temp array of glyph ptrs, this is so we can sort glyphs by some heuristic before
    // adding them to the glyph cache, to improve texture utilization for long strings of glyphs.
    GB_GLYPH** glyph_ptrs = (GB_GLYPH**)malloc(sizeof(GB_GLYPH*) * num_glyphs);

    // iterate over each glyph
    int num_glyph_ptrs;
    int i;
    for (i = 0, num_glyph_ptrs = 0; i < num_glyphs; i++) {

        int index = glyphs[i].codepoint;

        // check to see if this glyph already exists in the font glyph_hash
        GB_GLYPH* glyph = NULL;
        HASH_FIND_INT(glyph_hash, &index, glyph);
        if (!glyph) {

            // allocate a new glyph
            glyph = (GB_GLYPH*)malloc(sizeof(GB_GLYPH));

            // TODO: use FreeType to rasterize and hint glyph
            // for now just use 10x10.
            glyph->rc = 1;
            glyph->index = index;
            glyph->sheet_index = -1;
            glyph->origin[0] = 0;
            glyph->origin[1] = 0;
            glyph->size[0] = 10;
            glyph->size[1] = 10;
            glyph->image = NULL;

            // owner ship of image is passed to glyph
            uint8_t* image = (uint8_t*)malloc(sizeof(uint8_t) * glyph->size[0] * glyph->size[1]);
            int x, y;
            for (y = 0; y < 10; y++) {
                for (x = 0; x < 10; x++) {
                    if (y % 2 && x % 2) {
                        image[y * 10 + x] = 0;
                    } else {
                        image[y * 10 + x] = 255;
                    }
                }
            }
            glyph->image = image;

            // add to glyph_ptr array
            glyph_ptrs[num_glyph_ptrs++] = glyph;

            // add glyph to font glyph_hash
            HASH_ADD_INT(glyph_hash, index, glyph);

            printf("AJT: adding index %u to hash\n", index);
        } else {
            printf("AJT: index %u already in hash\n", index);
        }
    }

    /*
    // add glyphs to glyph cache
    // ownership of glyph structures passes to cache, so we should not free them.
    GB_GlyphCache_Insert(gb->glyph_cache, glyph_ptrs, num_glyph_ptrs);
    */
    free(glyph_ptrs);
}

GB_ERROR GB_TextMake(GB_CONTEXT* gb, const char* utf8_string, GB_FONT* font,
                     uint32_t color, uint32_t origin[2], uint32_t size[2],
                     GB_HORIZONTAL_ALIGN horizontal_align,
                     GB_VERTICAL_ALIGN vertical_align,
                     GB_TEXT** text_out)
{
    if (gb && utf8_string && font && font->hb_font && text_out) {
        GB_TEXT* text = (GB_TEXT*)malloc(sizeof(GB_TEXT));
        if (text) {
            memset(text, 0, sizeof(GB_TEXT));
            text->rc = 1;

            // reference font
            text->font = font;
            GB_FontReference(gb, font);

            // allocate and copy utf8 string
            size_t utf8_string_len = strlen(utf8_string);
            text->utf8_string = (char*)malloc(sizeof(char) * utf8_string_len + 1);
            strcpy(text->utf8_string, utf8_string);

            // copy other arguments
            text->color = color;
            text->origin[0] = origin[0];
            text->origin[1] = origin[1];
            text->size[0] = size[0];
            text->size[1] = size[1];
            text->horizontal_align = horizontal_align;
            text->vertical_align = vertical_align;

            // create harfbuzz buffer
            text->hb_buffer = hb_buffer_create();
            hb_buffer_set_direction(text->hb_buffer, HB_DIRECTION_LTR);
            hb_buffer_add_utf8(text->hb_buffer, utf8_string, utf8_string_len, 0, utf8_string_len);

            // shape text
            hb_shape(font->hb_font, text->hb_buffer, NULL, 0);

            _GB_UpdateGlyphCacheFromBuffer(gb, text);

            *text_out = text;

            return GB_ERROR_NONE;
        } else {
            return GB_ERROR_NOMEM;
        }
    } else {
        return GB_ERROR_INVAL;
    }
}

GB_ERROR GB_TextReference(GB_CONTEXT* gb, GB_TEXT* text)
{
    if (gb && text) {
        assert(text->rc > 0);
        text->rc++;
        return GB_ERROR_NONE;
    } else {
        return GB_ERROR_INVAL;
    }
}

static void _GB_TextDestroy(GB_CONTEXT* gb, GB_TEXT* text)
{
    assert(text);
    assert(text->rc == 0);
    GB_FontRelease(gb, text->font);
    hb_buffer_destroy(text->hb_buffer);
    free(text->utf8_string);
    free(text);
}

GB_ERROR GB_TextRelease(GB_CONTEXT* gb, GB_TEXT* text)
{
    if (gb && text) {
        assert(text->rc > 0);
        text->rc--;
        if (text->rc == 0) {
            _GB_TextDestroy(gb, text);
        }
        return GB_ERROR_NONE;
    } else {
        return GB_ERROR_INVAL;
    }
}

GB_ERROR GB_GetTextMetrics(GB_CONTEXT* gb, const char* utf8_string,
                           GB_FONT* font, uint32_t min[2], uint32_t max[2],
                           GB_HORIZONTAL_ALIGN horizontal_align,
                           GB_VERTICAL_ALIGN vertical_align,
                           GB_TEXT_METRICS* text_metrics_out)
{
    if (gb && utf8_string && font && text_metrics_out) {
        return GB_ERROR_NOIMP;
    } else {
        return GB_ERROR_INVAL;
    }
}

GB_ERROR GB_Update(GB_CONTEXT* gb)
{
    // text_list.each do |text|
    //   text.hb_buffer.glyphs.each do |glyph|
    //      cache glyph
    //      update hinted glyph info.
    //   end
    // end
    if (gb) {
        return GB_ERROR_NOIMP;
    } else {
        return GB_ERROR_INVAL;
    }
}

// Renders given text using renderer func.
GB_ERROR GB_TextDraw(GB_CONTEXT* gb, GB_TEXT* text)
{
    // text.glyphs.each do |glyph|
    //   build run of glyph_quads that share same opengl texture
    // end
    // send each run of glyph_quads to rendering func.
    if (gb && text) {
        return GB_ERROR_NOIMP;
    } else {
        return GB_ERROR_INVAL;
    }
}

const char* s_error_strings[GB_ERROR_NUM_ERRORS] = {
    "GB_ERROR_NONE",
    "GB_ERROR_NOENT",
    "GB_ERROR_NOMEM",
    "GB_ERROR_INVAL",
    "GB_ERROR_NOIMP",
    "GB_ERROR_FTERR"
};

const char* GB_ErrorToString(GB_ERROR err)
{
    if (err >= 0 && err < GB_ERROR_NUM_ERRORS) {
        return s_error_strings[err];
    } else {
        return "???";
    }
}
