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

static GB_ERROR _GB_TextUpdateCache(struct GB_Context *gb, struct GB_Text *text)
{
    assert(gb);
    assert(text);

    // count up the total number of glyphs in all the runs.
    int i, num_glyphs_total = 0;
    for (i = 0; i < text->num_runs; i++) {
        struct GB_TextRun *run = &text->runs[i];
        assert(run->hb_buffer);
        num_glyphs_total += hb_buffer_get_length(run->hb_buffer);
    }

    // hold a temp array of glyph ptrs, this is so we can sort glyphs by height before
    // adding them to the GlyphCache, which improves texture utilization for long strings of glyphs.
    struct GB_Glyph **glyph_ptrs = (struct GB_Glyph**)malloc(sizeof(struct GB_Glyph*) * num_glyphs_total);
    int num_glyph_ptrs = 0;

    struct GB_Cache *cache = gb->cache;

    // for each run
    for (i = 0; i < text->num_runs; i++) {
        struct GB_TextRun *run = &text->runs[i];
        assert(run->hb_buffer);

        // prepare to iterate over all the glyphs in the hb_buffer
        int num_glyphs = hb_buffer_get_length(run->hb_buffer);
        hb_glyph_info_t *glyphs = hb_buffer_get_glyph_infos(run->hb_buffer, NULL);

        // iterate over each glyph in this run
        int j;
        for (j = 0; j < num_glyphs; j++) {
            int index = glyphs[j].codepoint;

            // check to see if this glyph already exists in the context
            struct GB_Glyph *glyph = GB_ContextHashFind(gb, index, text->font->index);
            if (!glyph) {
                // check to see if this glyph already exists in the cache
                glyph = GB_CacheHashFind(cache, index, text->font->index);
                if (!glyph) {

                    // get FreeType face
                    FT_Face ft_face = text->font->ft_face;

                    // will rasterize and initialize glyph
                    GB_ERROR gb_error = GB_GlyphMake(index, text->font->index, ft_face, &glyph);
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
    }

    // add glyphs to cache and context
    GB_CacheInsert(gb, gb->cache, glyph_ptrs, num_glyph_ptrs);
    free(glyph_ptrs);

    return GB_ERROR_NONE;
}

static GB_ERROR _GB_TextUpdateQuads(struct GB_Context *gb, struct GB_Text *text)
{
    // count up the total number of glyphs in all the runs.
    int i, num_glyphs_total = 0;
    for (i = 0; i < text->num_runs; i++) {
        struct GB_TextRun *run = &text->runs[i];
        assert(run->hb_buffer);
        num_glyphs_total += hb_buffer_get_length(run->hb_buffer);
    }

    // allocate glyph_quads array
    text->glyph_quads = (struct GB_GlyphQuad*)malloc(sizeof(struct GB_GlyphQuad) * num_glyphs_total);
    if (text->glyph_quads == NULL)
        return GB_ERROR_NOMEM;
    memset(text->glyph_quads, 0, sizeof(struct GB_GlyphQuad) * num_glyphs_total);
    text->num_glyph_quads = 0;

    int32_t pen[2] = {text->origin[0], text->origin[1]};

    int32_t line_height = FIXED_TO_INT(text->font->ft_face->size->metrics.height);

    for (i = 0; i < text->num_runs; i++) {

        pen[0] = text->origin[0];
        pen[1] = pen[1] + line_height;

        struct GB_TextRun *run = &text->runs[i];
        int num_glyphs = hb_buffer_get_length(run->hb_buffer);
        hb_glyph_info_t *glyphs = hb_buffer_get_glyph_infos(run->hb_buffer, NULL);

        int j;
        for (j = 0; j < num_glyphs; j++) {

            // apply kerning
            if (j > 0)
            {
                FT_Vector delta;
                FT_Get_Kerning(text->font->ft_face, glyphs[j-1].codepoint, glyphs[j].codepoint,
                               FT_KERNING_DEFAULT, &delta);
                pen[0] += FIXED_TO_INT(delta.x);
            }

            struct GB_Glyph *glyph = GB_ContextHashFind(gb, glyphs[j].codepoint, text->font->index);
            if (glyph) {
                const float texture_size = (float)gb->cache->texture_size;

                // skip whitespace glyphs.
                if (glyph->size[0] > 0 && glyph->size[1] > 0) {
                    // NOTE: y axis points down, quad origin is upper-left corner of glyph
                    // build quad
                    struct GB_GlyphQuad *quad = text->glyph_quads + text->num_glyph_quads;
                    quad->origin[0] = pen[0] + glyph->bearing[0];
                    quad->origin[1] = pen[1] - glyph->bearing[1];
                    quad->size[0] = glyph->size[0];
                    quad->size[1] = glyph->size[1];
                    quad->uv_origin[0] = glyph->origin[0] / texture_size;
                    quad->uv_origin[1] = glyph->origin[1] / texture_size;
                    quad->uv_size[0] = glyph->size[0] / texture_size;
                    quad->uv_size[1] = glyph->size[1] / texture_size;
                    quad->color = text->color;
                    quad->gl_tex_obj = glyph->gl_tex_obj ? glyph->gl_tex_obj : gb->fallback_gl_tex_obj;
                    text->num_glyph_quads++;
                }

                pen[0] += glyph->advance;
            }
        }
    }
    return GB_ERROR_NONE;
}

static void renderRun(const UChar *str, int32_t start, int32_t end, UBiDiDirection direction)
{
    fprintf(stdout, "BIDI: run from %d to %d, direction = %d\n", start, end, direction);
    fflush(stdout);
}

static void _GB_TextLogicalToVisualOrder(struct GB_Context *gb, struct GB_Text *text)
{
    int32_t str_len = 0;
    UErrorCode errorCode = 0;

    // icu4c wants utf-16 encoded strings for bidi
    // figure out how large the utf16 string will be.
    u_strFromUTF8(NULL, 0, &str_len, (const char*)text->utf8_string, -1, &errorCode);

    // now allocate and convert from utf8 to utf16
    errorCode = 0;
    UChar *str = (UChar*)calloc(str_len + 1, sizeof(UChar*));
    u_strFromUTF8(str, (str_len + 1) * 1000000, &str_len, (const char*)text->utf8_string, -1, &errorCode);
    if (!U_SUCCESS(errorCode)) {
        fprintf(stderr, "BIDI: u_strFromUTF8 convert failed, errorCode = %d\n", errorCode);
        return;
    }

    const UBiDiDirection textDirection = UBIDI_LTR;

    UBiDi *para = ubidi_openSized(str_len, 0, &errorCode);
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

// returns the number of bytes to advance
// fills cp_out with the code point at p.
static uint32_t utf8_next_cp(const uint8_t *p, uint32_t *cp_out)
{
    if ((*p & 0x80) == 0) {
        *cp_out = *p;
        return 1;
    } else if ((*p & 0xe0) == 0xc0) { // 110xxxxx 10xxxxxx
        *cp_out = ((*p & ~0xe0) << 6) | (*(p+1) & ~0xc0);
        return 2;
    } else if ((*p & 0xf0) == 0xe0) { // 1110xxxx 10xxxxxx 10xxxxxx
        *cp_out = ((*p & ~0xf0) << 12) | ((*(p+1) & ~0xc0) << 6) | (*(p+2) & ~0xc0);
        return 3;
    } else if ((*p & 0xf8) == 0xf0) { // 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
        *cp_out = ((*p & ~0xf8) << 18) | ((*(p+1) & ~0xc0) << 12) | ((*(p+1) & ~0xc0) << 6) | (*(p+2) & ~0xc0);
        return 4;
    } else {
        assert(0);
        *cp_out = 0;
        return 1;
    }
}

static void _GB_InitRun(struct GB_Text *text, struct GB_TextRun *run, const uint8_t *start, const uint8_t *end)
{
    assert(run);
    assert(start <= end);

    run->utf8_string_len = end - start;

    // alloc and copy line into runs
    run->utf8_string = (uint8_t*)malloc(run->utf8_string_len + 1);
    memcpy((void*)run->utf8_string, start, run->utf8_string_len);
    run->utf8_string[run->utf8_string_len] = 0; // null term.

    // create harfbuzz buffer
    run->hb_buffer = hb_buffer_create();
    hb_buffer_set_direction(run->hb_buffer, HB_DIRECTION_LTR);
    hb_buffer_add_utf8(run->hb_buffer, (const char*)run->utf8_string, run->utf8_string_len, 0, run->utf8_string_len);

    // shape text
    hb_shape(text->font->hb_font, run->hb_buffer, NULL, 0);

    // icu4c logical to visual bidi
    //_GB_TextLogicalToVisualOrder(gb, run);
}

static void _GB_MakeRuns(struct GB_Text *text, struct GB_TextRun **runs_out, uint32_t *num_runs_out,
                         const uint8_t *utf8_string)
{
    // iterate over utf8 string looking for new-lines.
    uint32_t line_count = 1;
    uint32_t cp = 0, next_cp = 0, offset = 0, next_offset = 0;
    const uint8_t *p = (const uint8_t*)utf8_string;
    while (*p != 0) {
        offset = utf8_next_cp(p, &cp);
        next_offset = utf8_next_cp(p+offset, &next_cp);

        // CR+LF
        if (cp == 0x0d && cp == 0x0a) {
            line_count++;
            p += offset + next_offset;
        } else {
            switch (cp) {
            case 0x0a: // new line
            case 0x0b: // vertical tab
            case 0x0c: // form feed
            case 0x0d: // carriage return
            case 0x85: // NEL next line
            case 0x2028: // line separator
            case 0x2029: // paragraph separator
                line_count++;
                p += offset;
                break;
            default:
                p += offset;
                break;
            }
        }
    }

    // allocate runs
    struct GB_TextRun *runs = (struct GB_TextRun*)calloc(line_count, sizeof(struct GB_TextRun));

    // iterate over utf8 string again, looking for new lines.
    // and copy each line string into runs.
    uint32_t i = 0;
    cp = 0;
    next_cp = 0;
    offset = 0;
    next_offset = 0;
    p = (const uint8_t*)utf8_string;
    const uint8_t *prev_p = (const uint8_t*)utf8_string;
    while (*p != 0) {
        offset = utf8_next_cp(p, &cp);
        next_offset = utf8_next_cp(p+offset, &next_cp);

        // CR+LF
        if (cp == 0x0d && cp == 0x0a) {
            _GB_InitRun(text, &runs[i], prev_p, p);
            i++;
            p += offset + next_offset;
            prev_p = p;
        } else {
            switch (cp) {
            case 0x0a: // new line
            case 0x0b: // vertical tab
            case 0x0c: // form feed
            case 0x0d: // carriage return
            case 0x85: // NEL next line
            case 0x2028: // line separator
            case 0x2029: // paragraph separator
                _GB_InitRun(text, &runs[i], prev_p, p);
                i++;
                p += offset;
                prev_p = p;
                break;
            default:
                p += offset;
                break;
            }
        }
    }
    // last line
    _GB_InitRun(text, &runs[i], prev_p, p);
    i++;
    assert(i == line_count);

    *runs_out = runs;
    *num_runs_out = line_count;
}

void _GB_DestroyRuns(struct GB_Context* gb, struct GB_Text* text)
{
    uint32_t i;
    for (i = 0; i < text->num_runs; i++) {
        struct GB_TextRun *run = &text->runs[i];

        free(run->utf8_string);

        // prepare to iterate over all the glyphs in the hb_buffer
        int num_glyphs = hb_buffer_get_length(run->hb_buffer);
        hb_glyph_info_t *glyphs = hb_buffer_get_glyph_infos(run->hb_buffer, NULL);

        // remove each glyph from context
        int j;
        int font_index = text->font->index;
        for (j = 0; j < num_glyphs; j++) {
            GB_ContextHashRemove(gb, glyphs[j].codepoint, font_index);
        }
        hb_buffer_destroy(run->hb_buffer);
    }
    free(text->runs);
}

GB_ERROR GB_TextMake(struct GB_Context *gb, const uint8_t *utf8_string,
                     struct GB_Font *font, uint32_t color, uint32_t origin[2],
                     uint32_t size[2], GB_HORIZONTAL_ALIGN horizontal_align,
                     GB_VERTICAL_ALIGN vertical_align, struct GB_Text **text_out)
{
    if (gb && utf8_string && font && font->hb_font && text_out) {
        struct GB_Text *text = (struct GB_Text*)malloc(sizeof(struct GB_Text));
        if (text) {
            memset(text, 0, sizeof(struct GB_Text));
            text->rc = 1;

            // reference font
            text->font = font;
            GB_FontRetain(gb, font);

            // allocate and copy utf8 string
            size_t utf8_string_len = strlen((const char*)utf8_string);
            text->utf8_string = (uint8_t*)malloc(sizeof(char) * utf8_string_len + 1);
            strcpy((char*)text->utf8_string, (const char*)utf8_string);

            // break utf8_strings into seperate runs, one per line.
            _GB_MakeRuns(text, &(text->runs), &(text->num_runs), utf8_string);

            fprintf(stdout, "num_runs = %d\n", text->num_runs);
            uint32_t i;
            for (i = 0; i < text->num_runs; i++) {
                fprintf(stdout, "run[%d] = \"%s\"\n", i, text->runs[i].utf8_string);
            }

            // copy other arguments
            text->color = color;
            text->origin[0] = origin[0];
            text->origin[1] = origin[1];
            text->size[0] = size[0];
            text->size[1] = size[1];
            text->horizontal_align = horizontal_align;
            text->vertical_align = vertical_align;
            text->glyph_quads = NULL;
            text->num_glyph_quads = 0;

            // insert new glyphs into cache
            GB_ERROR ret = _GB_TextUpdateCache(gb, text);
            if (ret != GB_ERROR_NONE)
                return ret;

            // allocate glyph_quads array with proper position & uvs.
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

GB_ERROR GB_TextRetain(struct GB_Context *gb, struct GB_Text *text)
{
    if (gb && text) {
        assert(text->rc > 0);
        text->rc++;
        return GB_ERROR_NONE;
    } else {
        return GB_ERROR_INVAL;
    }
}

static void _GB_TextDestroy(struct GB_Context *gb, struct GB_Text *text)
{
    assert(text);
    assert(text->rc == 0);

    GB_FontRelease(gb, text->font);

    free(text->utf8_string);
    _GB_DestroyRuns(gb, text);
    if (text->glyph_quads)
        free(text->glyph_quads);
    free(text);
}

GB_ERROR GB_TextRelease(struct GB_Context *gb, struct GB_Text *text)
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

GB_ERROR GB_GetTextMetrics(struct GB_Context *gb, const uint8_t *utf8_string,
                           struct GB_Font *font, uint32_t min[2], uint32_t max[2],
                           GB_HORIZONTAL_ALIGN horizontal_align,
                           GB_VERTICAL_ALIGN vertical_align,
                           struct GB_TextMetrics *text_metrics_out)
{
    if (gb && utf8_string && font && text_metrics_out) {
        return GB_ERROR_NOIMP;
    } else {
        return GB_ERROR_INVAL;
    }
}

// Renders given text using renderer func.
GB_ERROR GB_TextDraw(struct GB_Context *gb, struct GB_Text *text)
{
    // text.glyphs.each do |glyph|
    //   build run of glyph_quads that share same opengl texture
    // end
    // send each run of glyph_quads to rendering func.
    if (gb && text) {
        if (gb->text_render_func) {
            GB_TextRenderFunc func = (GB_TextRenderFunc)gb->text_render_func;
            func(text->glyph_quads, text->num_glyph_quads);
            return GB_ERROR_NONE;
        }
        else {
            return GB_ERROR_NOIMP;
        }
    } else {
        return GB_ERROR_INVAL;
    }
}
