#include <assert.h>
#include "uthash.h"
#include "gb_context.h"
#include "gb_font.h"
#include "gb_glyph.h"
#include "gb_cache.h"
#include "gb_text.h"

static GB_ERROR _GB_UpdateGlyphCacheFromBuffer(struct GB_Context* gb, struct GB_Text* text)
{
    // prepare to iterate over all the glyphs in the hb_buffer
    int num_glyphs = hb_buffer_get_length(text->hb_buffer);
    hb_glyph_info_t* glyphs = hb_buffer_get_glyph_infos(text->hb_buffer, NULL);

    // hold a temp array of glyph ptrs, this is so we can sort glyphs by some heuristic before
    // adding them to the GlyphCache, to improve texture utilization for long strings of glyphs.
    struct GB_Glyph** glyph_ptrs = (struct GB_Glyph**)malloc(sizeof(struct GB_Glyph*) * num_glyphs);

    struct GB_Cache* cache = gb->cache;

    // iterate over each glyph
    int num_glyph_ptrs;
    int i, j;
    for (i = 0, num_glyph_ptrs = 0; i < num_glyphs; i++) {

        int index = glyphs[i].codepoint;

        // check to see if this glyph already exists in the font glyph_hash
        struct GB_Glyph* glyph = NULL;
        HASH_FIND(font_hh, text->font->glyph_hash, &index, sizeof(uint32_t), glyph);
        if (!glyph) {
            // check to see if this glyph already exists in the cache glyph_hash
            HASH_FIND(cache_hh, cache->glyph_hash, &index, sizeof(uint32_t), glyph);
            if (!glyph) {

                // rasterize glyph
                FT_Face ft_face = text->font->ft_face;
                FT_Error ft_error = FT_Load_Glyph(ft_face, index, FT_LOAD_DEFAULT);
                if (ft_error)
                    return GB_ERROR_FTERR;

                // render glyph into ft_face->glyph->bitmap
                ft_error = FT_Render_Glyph(ft_face->glyph, FT_RENDER_MODE_NORMAL);
                if (ft_error)
                    return GB_ERROR_FTERR;

                FT_Bitmap* ft_bitmap = &ft_face->glyph->bitmap;

                uint8_t* image = NULL;
                if (ft_bitmap->width > 0 && ft_bitmap->rows > 0) {
                    // allocate an image to hold a copy of the rasterized glyph
                    image = (uint8_t*)malloc(sizeof(uint8_t) * ft_bitmap->width * ft_bitmap->rows);

                    // copy image from ft_bitmap.buffer into image, row by row.
                    // The pitch of each row in the ft_bitmap maybe >= width,
                    // so we cant do this as a single memcpy.
                    for (j = 0; j < ft_face->glyph->bitmap.rows; j++) {
                        memcpy(image + (j * ft_bitmap->width),
                               ft_bitmap->buffer + (j * ft_bitmap->pitch),
                               ft_bitmap->width);
                    }
                }

                uint32_t origin[2] = {0, 0};
                uint32_t size[2] = {ft_bitmap->width, ft_bitmap->rows};
                GB_ERROR gb_error = GB_GlyphMake(index, -1, origin, size, image, &glyph);
                if (gb_error)
                    return gb_error;

                // add to glyph_ptr array
                glyph_ptrs[num_glyph_ptrs++] = glyph;

                // add glyph to font glyph_hash
                HASH_ADD(font_hh, text->font->glyph_hash, index, sizeof(uint32_t), glyph);

                printf("AJT: adding index %u to hash (%d x %d) image = %p\n", index, glyph->size[0], glyph->size[1], glyph->image);
            } else {
                printf("AJT: index %u already in cache glyph_hash\n", index);

                // add glyph to font glyph_hash
                HASH_ADD(font_hh, text->font->glyph_hash, index, sizeof(uint32_t), glyph);
                GB_GlyphRetain(glyph);
            }
        } else {
            printf("AJT: index %u already in font glyph_hash\n", index);
        }
    }

    // add glyphs to GlyphCache
    GB_CacheInsert(gb, gb->cache, glyph_ptrs, num_glyph_ptrs);

    free(glyph_ptrs);

    return GB_ERROR_NONE;
}

GB_ERROR GB_TextMake(struct GB_Context* gb, const char* utf8_string,
                     struct GB_Font* font, uint32_t color, uint32_t origin[2],
                     uint32_t size[2], GB_HORIZONTAL_ALIGN horizontal_align,
                     GB_VERTICAL_ALIGN vertical_align, struct GB_Text** text_out)
{
    if (gb && utf8_string && font && font->hb_font && text_out) {
        struct GB_Text* text = (struct GB_Text*)malloc(sizeof(struct GB_Text));
        if (text) {
            memset(text, 0, sizeof(struct GB_Text));
            text->rc = 1;

            // reference font
            text->font = font;
            GB_FontRetain(gb, font);

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

            GB_ERROR ret = _GB_UpdateGlyphCacheFromBuffer(gb, text);
            if (ret == GB_ERROR_NONE)
                *text_out = text;
            return ret;
        } else {
            return GB_ERROR_NOMEM;
        }
    } else {
        return GB_ERROR_INVAL;
    }
}

GB_ERROR GB_TextRetain(struct GB_Context* gb, struct GB_Text* text)
{
    if (gb && text) {
        assert(text->rc > 0);
        text->rc++;
        return GB_ERROR_NONE;
    } else {
        return GB_ERROR_INVAL;
    }
}

static void _GB_TextDestroy(struct GB_Context* gb, struct GB_Text* text)
{
    assert(text);
    assert(text->rc == 0);
    GB_FontRelease(gb, text->font);

    // prepare to iterate over all the glyphs in the hb_buffer
    int num_glyphs = hb_buffer_get_length(text->hb_buffer);
    hb_glyph_info_t* glyphs = hb_buffer_get_glyph_infos(text->hb_buffer, NULL);

    // iterate over each glyph
    int i;
    for (i = 0; i < num_glyphs; i++) {
        int index = glyphs[i].codepoint;
        struct GB_Glyph* glyph = NULL;
        HASH_FIND(font_hh, text->font->glyph_hash, &index, sizeof(uint32_t), glyph);
        if (glyph) {
            HASH_DELETE(font_hh, text->font->glyph_hash, glyph);
            GB_GlyphRelease(glyph);
        }
    }

    hb_buffer_destroy(text->hb_buffer);
    free(text->utf8_string);
    free(text);
}

GB_ERROR GB_TextRelease(struct GB_Context* gb, struct GB_Text* text)
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

GB_ERROR GB_GetTextMetrics(struct GB_Context* gb, const char* utf8_string,
                           struct GB_Font* font, uint32_t min[2], uint32_t max[2],
                           GB_HORIZONTAL_ALIGN horizontal_align,
                           GB_VERTICAL_ALIGN vertical_align,
                           struct GB_TextMetrics* text_metrics_out)
{
    if (gb && utf8_string && font && text_metrics_out) {
        return GB_ERROR_NOIMP;
    } else {
        return GB_ERROR_INVAL;
    }
}

// Renders given text using renderer func.
GB_ERROR GB_TextDraw(struct GB_Context* gb, struct GB_Text* text)
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
