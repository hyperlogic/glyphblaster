#include "glyphblaster.h"
#include "glyphcache.h"
#include <stdlib.h>
#include <assert.h>

GB_ERROR GB_Init(GB_GB** gb_out)
{
    *gb_out = (GB_GB*)malloc(sizeof(GB_GB));
    memset(*gb_out, 0, sizeof(GB_GB));
    if (*gb_out) {
        if (FT_Init_FreeType(&(*gb_out)->ft_library)) {
            return GB_ERROR_FTERR;
        }
        GB_GLYPH_CACHE* glyph_cache = 0;
        GB_ERROR err = GB_MakeGlyphCache(&glyph_cache);
        if (err == GB_ERROR_NONE) {
            (*gb_out)->glyph_cache = glyph_cache;
        }
        (*gb_out)->next_index = 1;
        return err;
    } else {
        return GB_ERROR_NOMEM;
    }
}

GB_ERROR GB_Shutdown(GB_GB* gb)
{
    if (gb) {
        if (gb->ft_library) {
            FT_Done_FreeType(gb->ft_library);
        }
        GB_DestroyGlyphCache((GB_GLYPH_CACHE*)gb->glyph_cache);
        free(gb);
        return GB_ERROR_NONE;
    } else {
        return GB_ERROR_INVAL;
    }
}

GB_ERROR GB_MakeFont(GB_GB* gb, const char* filename, uint32_t point_size,
                     GB_FONT** font_out)
{
    if (gb && filename && font_out) {
        *font_out = (GB_FONT*)malloc(sizeof(GB_FONT));
        if (font_out) {
            memset(*font_out, 0, sizeof(GB_FONT));

            (*font_out)->rc = 1;

            // create freetype face
            FT_New_Face(gb->ft_library, "dejavu-fonts-ttf-2.33/ttf/DejaVuSans.ttf", 0, &(*font_out)->ft_face);
            FT_Set_Char_Size((*font_out)->ft_face, (int)(point_size * 64), 0, 72, 72);

            // create harfbuzz font
            (*font_out)->hb_font = hb_ft_font_create((*font_out)->ft_face, 0);
            hb_ft_font_set_funcs((*font_out)->hb_font);

            // assign a "unique" index to this font.
            (*font_out)->index = gb->next_index++;

            return GB_ERROR_NONE;
        } else {
            return GB_ERROR_NOMEM;
        }
    } else {
        return GB_ERROR_INVAL;
    }
}

GB_ERROR GB_ReferenceFont(GB_GB* gb, GB_FONT* font)
{
    if (gb && font) {
        assert(font->rc > 0);
        font->rc++;
        return GB_ERROR_NONE;
    } else {
        return GB_ERROR_INVAL;
    }
}

static void _GB_DestroyFont(GB_GB* gb, GB_FONT* font)
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

    free(font);
}

GB_ERROR GB_ReleaseFont(GB_GB* gb, GB_FONT* font)
{
    if (gb && font) {
        font->rc--;
        assert(font->rc >= 0);
        if (font->rc == 0) {
            _GB_DestroyFont(gb, font);
        }
        return GB_ERROR_NONE;
    } else {
        return GB_ERROR_INVAL;
    }
}

static void _GB_UpdateGlyphCacheFromBuffer(GB_GB* gb, GB_TEXT* text)
{
    int num_glyphs = hb_buffer_get_length(text->hb_buffer);
    hb_glyph_info_t* glyphs = hb_buffer_get_glyph_infos(text->hb_buffer, 0);
    hb_glyph_info_t* info = hb_buffer_get_glyph_infos(text->hb_buffer, NULL);
    int i;
    for (i = 0; i < num_glyphs; i++) {

        int index = glyphs[i].codepoint;
        int font_index = text->font->index;
        uint32_t gl_tex = 0;
        GB_GLYPH* glyph = NULL;

        printf("Updating glyph index = %d, font_index = %d\n", index, font_index);

        GB_FindInGlyphCache(gb->glyph_cache, index, font_index, &gl_tex, &glyph);
        if (glyph) {
            // found in glyph_cache
            printf("    found in glyph_cache!\n");
        } else {
            // not found in glyph_cache
            printf("    not found in glyph_cache!\n");
            GB_GLYPH g;
            g.index = index;
            g.font_index = font_index;
            g.origin_x = 0;
            g.origin_y = 0;
            g.size_x = 10; // TODO: fill in with actual width
            g.size_y = 10;

            // TODO: fill with actual ft rasterized glyph
            uint32_t image[10*10];
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
        }
    }
}

GB_ERROR GB_MakeText(GB_GB* gb, const char* utf8_string, GB_FONT* font,
                     uint32_t color, uint32_t min[2], uint32_t max[2],
                     GB_HORIZONTAL_ALIGN horizontal_align,
                     GB_VERTICAL_ALIGN vertical_align,
                     GB_TEXT** text_out)
{
    if (gb && utf8_string && font && font->hb_font && text_out) {
        *text_out = (GB_TEXT*)malloc(sizeof(GB_TEXT));
        if (text_out) {
            memset(*text_out, 0, sizeof(GB_TEXT));
            (*text_out)->rc = 1;

            // reference font
            (*text_out)->font = font;
            GB_ReferenceFont(gb, font);

            // allocate and copy utf8 string
            size_t utf8_string_len = strlen(utf8_string);
            (*text_out)->utf8_string = (char*)malloc(sizeof(char) * utf8_string_len + 1);
            strcpy((*text_out)->utf8_string, utf8_string);

            // copy other arguments
            (*text_out)->color = color;
            (*text_out)->min[0] = min[0];
            (*text_out)->min[1] = min[1];
            (*text_out)->max[0] = max[0];
            (*text_out)->max[1] = max[1];
            (*text_out)->horizontal_align = horizontal_align;
            (*text_out)->vertical_align = vertical_align;

            // create harfbuzz buffer
            (*text_out)->hb_buffer = hb_buffer_create();
            hb_buffer_set_direction((*text_out)->hb_buffer, HB_DIRECTION_LTR);
            hb_buffer_add_utf8((*text_out)->hb_buffer, utf8_string, utf8_string_len, 0, utf8_string_len);

            // shape text
            hb_shape(font->hb_font, (*text_out)->hb_buffer, NULL, 0);

            _GB_UpdateGlyphCacheFromBuffer(gb, *text_out);

            return GB_ERROR_NONE;
        } else {
            return GB_ERROR_NOMEM;
        }
    } else {
        return GB_ERROR_INVAL;
    }
}

GB_ERROR GB_ReferenceText(GB_GB* gb, GB_TEXT* text)
{
    if (gb && text) {
        assert(text->rc > 0);
        text->rc++;
        return GB_ERROR_NONE;
    } else {
        return GB_ERROR_INVAL;
    }
}

static void _GB_DestroyText(GB_GB* gb, GB_TEXT* text)
{
    assert(text);
    assert(text->rc == 0);
    GB_ReleaseFont(gb, text->font);
    hb_buffer_destroy(text->hb_buffer);
    free(text->utf8_string);
    free(text);
}

GB_ERROR GB_ReleaseText(GB_GB* gb, GB_TEXT* text)
{
    if (gb && text) {
        assert(text->rc > 0);
        text->rc--;
        if (text->rc == 0) {
            _GB_DestroyText(gb, text);
        }
        return GB_ERROR_NONE;
    } else {
        return GB_ERROR_INVAL;
    }
}

GB_ERROR GB_GetTextMetrics(GB_GB* gb, const char* utf8_string,
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

GB_ERROR GB_Update(GB_GB* gb)
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
GB_ERROR GB_DrawText(GB_GB* gb, GB_TEXT* text)
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
