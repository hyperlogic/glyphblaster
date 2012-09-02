#include <assert.h>
#include <unicode/ubidi.h>
#include <unicode/ustring.h>
#include "uthash.h"
#include "gb_context.h"
#include "gb_font.h"
#include "gb_glyph.h"
#include "gb_cache.h"
#include "gb_text.h"

// 26.6 fixed to int (truncates)
#define FIXED_TO_INT(n) (uint32_t)(n >> 6)

static GB_ERROR _GB_TextUpdateCache(struct GB_Context* gb, struct GB_Text* text)
{
    assert(gb);
    assert(text);
    assert(text->hb_buffer);

    // prepare to iterate over all the glyphs in the hb_buffer
    int num_glyphs = hb_buffer_get_length(text->hb_buffer);

    hb_glyph_info_t* glyphs = hb_buffer_get_glyph_infos(text->hb_buffer, NULL);

    // hold a temp array of glyph ptrs, this is so we can sort glyphs by some heuristic before
    // adding them to the GlyphCache, to improve texture utilization for long strings of glyphs.
    struct GB_Glyph** glyph_ptrs = (struct GB_Glyph**)malloc(sizeof(struct GB_Glyph*) * num_glyphs);

    struct GB_Cache* cache = gb->cache;

    // iterate over each glyph
    int i, j, num_glyph_ptrs;
    for (i = 0, num_glyph_ptrs = 0; i < num_glyphs; i++) {

        int index = glyphs[i].codepoint;

        // check to see if this glyph already exists in the context
        struct GB_Glyph* glyph = GB_ContextHashFind(gb, index, text->font->index);
        if (!glyph) {
            // check to see if this glyph already exists in the cache
            glyph = GB_CacheHashFind(cache, index, text->font->index);
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

                // record post-hinted advance and bearing.
                uint32_t advance = FIXED_TO_INT(ft_face->glyph->metrics.horiAdvance);
                uint32_t bearing[2] = {FIXED_TO_INT(ft_face->glyph->metrics.horiBearingX),
                                       FIXED_TO_INT(ft_face->glyph->metrics.horiBearingY)};

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
                GB_ERROR gb_error = GB_GlyphMake(index, text->font->index, 0, origin, size,
                                                 advance, bearing, image, &glyph);
                if (gb_error)
                    return gb_error;

                // add to glyph_ptr array
                glyph_ptrs[num_glyph_ptrs++] = glyph;

                printf("AJT: adding glyph %u to context (%d x %d) image = %p\n", index, glyph->size[0], glyph->size[1], glyph->image);
            } else {
                printf("AJT: glyph %u already in cache\n", index);

                // add glyph to context
                GB_ContextHashAdd(gb, glyph);
            }
        } else {
            printf("AJT: glyph %u already in context\n", index);
        }
    }

    // add glyphs to cache and context
    GB_CacheInsert(gb, gb->cache, glyph_ptrs, num_glyph_ptrs);

    free(glyph_ptrs);

    return GB_ERROR_NONE;
}

static GB_ERROR _GB_TextUpdateQuads(struct GB_Context* gb, struct GB_Text* text)
{
    // allocate glyph_quad array
    int num_glyphs = hb_buffer_get_length(text->hb_buffer);
    text->glyph_quad = (struct GB_GlyphQuad*)malloc(sizeof(struct GB_GlyphQuad) * num_glyphs);
    if (text->glyph_quad == NULL)
        return GB_ERROR_NOMEM;
    memset(text->glyph_quad, 0, sizeof(struct GB_GlyphQuad) * num_glyphs);
    text->num_glyph_quads = num_glyphs;

    int32_t pen[2] = {text->origin[0], text->origin[1]};

    hb_glyph_info_t* glyphs = hb_buffer_get_glyph_infos(text->hb_buffer, NULL);

    int i;
    for (i = 0; i < num_glyphs; i++) {

        // apply kerning
        if (i > 0)
        {
            FT_Vector delta;
            FT_Get_Kerning(text->font->ft_face, glyphs[i-1].codepoint, glyphs[i].codepoint,
                           FT_KERNING_DEFAULT, &delta);
            pen[0] += FIXED_TO_INT(delta.x);
        }

        struct GB_Glyph* glyph = GB_ContextHashFind(gb, glyphs[i].codepoint, text->font->index);
        if (glyph) {

            const float texture_size = (float)gb->cache->texture_size;
            // NOTE: y axis points down, quad origin is upper-left corner of glyph
            // build quad
            struct GB_GlyphQuad* quad = text->glyph_quad + i;
            quad->origin[0] = pen[0] + glyph->bearing[0];
            quad->origin[1] = pen[1] - glyph->bearing[1];
            quad->size[0] = glyph->size[0];
            quad->size[1] = glyph->size[1];
            quad->uv_origin[0] = glyph->origin[0] / texture_size;
            quad->uv_origin[1] = glyph->origin[1] / texture_size;
            quad->uv_size[0] = glyph->size[0] / texture_size;
            quad->uv_size[1] = glyph->size[1] / texture_size;
            quad->color = text->color;
            quad->gl_tex_obj = glyph->gl_tex_obj;

            pen[0] += glyph->advance;
        }
    }

    return GB_ERROR_NONE;
}

static void renderRun(const UChar* str, int32_t start, int32_t end, UBiDiDirection direction)
{
    fprintf(stdout, "BIDI: run from %d to %d, direction = %d\n", start, end, direction);
    fflush(stdout);
}

static void _GB_TextLogicalToVisualOrder(struct GB_Context* gb, struct GB_Text* text)
{
    int32_t str_len = 0;
    UErrorCode errorCode = 0;

    // icu4c wants utf-16 encoded strings for bidi
    // figure out how large the utf16 string will be.
    u_strFromUTF8(NULL, 0, &str_len, text->utf8_string, -1, &errorCode);

    // now allocate and convert from utf8 to utf16
    errorCode = 0;
    UChar* str = (UChar*)calloc(str_len + 1, sizeof(UChar*));
    u_strFromUTF8(str, (str_len + 1) * 1000000, &str_len, text->utf8_string, -1, &errorCode);
    if (!U_SUCCESS(errorCode)) {
        fprintf(stderr, "BIDI: u_strFromUTF8 convert failed, errorCode = %d\n", errorCode);
        return;
    }

    const UBiDiDirection textDirection = UBIDI_LTR;

    UBiDi* para = ubidi_openSized(str_len, 0, &errorCode);
    if (para == NULL)
        return;

    ubidi_setPara(para, str, str_len, textDirection ? UBIDI_DEFAULT_RTL : UBIDI_DEFAULT_LTR,
                  NULL, &errorCode);
    if (U_SUCCESS(errorCode)) {
        UBiDiDirection direction = ubidi_getDirection(para);
        if (direction != UBIDI_MIXED) {
            // unidirectional
            renderRun(str, 0, str_len, direction);
        } else {
            // mixed-directional
            int32_t count, i, length, start;

            count = ubidi_countRuns(para, &errorCode);
            if (U_SUCCESS(errorCode)) {
                // iterate over directional runs
                for (i = 0; i < count; ++i) {
                    direction = ubidi_getVisualRun(para, i, &start, &length);
                    renderRun(str, start, start + length, direction);
                }
            }
        }
    }

    free(str);
    ubidi_close(para);
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
            text->glyph_quad = NULL;
            text->num_glyph_quads = 0;

            // create harfbuzz buffer
            text->hb_buffer = hb_buffer_create();
            hb_buffer_set_direction(text->hb_buffer, HB_DIRECTION_LTR);
            hb_buffer_add_utf8(text->hb_buffer, utf8_string, utf8_string_len, 0, utf8_string_len);

            // shape text
            hb_shape(font->hb_font, text->hb_buffer, NULL, 0);

            // icu4c logical to visual bidi
            //_GB_TextLogicalToVisualOrder(gb, text);

            // insert new glyphs into cache
            GB_ERROR ret = _GB_TextUpdateCache(gb, text);
            if (ret != GB_ERROR_NONE)
                return ret;

            // allocate glyph_quad array with proper position & uvs.
            ret = _GB_TextUpdateQuads(gb, text);
            if (ret != GB_ERROR_NONE)
                return ret;

            *text_out = text;
            return GB_ERROR_NONE;
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

    // prepare to iterate over all the glyphs in the hb_buffer
    int num_glyphs = hb_buffer_get_length(text->hb_buffer);
    hb_glyph_info_t* glyphs = hb_buffer_get_glyph_infos(text->hb_buffer, NULL);

    // remove each glyph from context
    int i;
    int font_index = text->font->index;
    for (i = 0; i < num_glyphs; i++) {
        GB_ContextHashRemove(gb, glyphs[i].codepoint, font_index);
    }

    GB_FontRelease(gb, text->font);

    hb_buffer_destroy(text->hb_buffer);
    free(text->utf8_string);
    if (text->glyph_quad)
        free(text->glyph_quad);
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
        if (gb->text_render_func) {
            GB_TextRenderFunc func = (GB_TextRenderFunc)gb->text_render_func;
            func(text->glyph_quad, text->num_glyph_quads);
            return GB_ERROR_NONE;
        }
        else {
            return GB_ERROR_NOIMP;
        }
    } else {
        return GB_ERROR_INVAL;
    }
}
