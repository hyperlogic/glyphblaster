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

static int is_space(uint32_t cp)
{
    switch (cp) {
    case 0x20:   // space
    case 0x1680: // ogham space mark
    case 0x180e: // mongolian vowel separator
    case 0x2000: // en quad
    case 0x2001: // em quad
    case 0x2002: // en space
    case 0x2003: // em space
    case 0x2004: // three-per-em space
    case 0x2005: // four-per-em space
    case 0x2006: // six-per-em space
    case 0x2008: // punctuation space
    case 0x2009: // thin space
    case 0x200a: // hair space
    case 0x200b: // zero width space
    case 0x200c: // zero width non joiner
    case 0x200d: // zero width joiner
    case 0x205f: // medium mathematical space
    case 0x3000: // ideograpfic space
        return 1;
    default:
        return 0;
    }
}

static int is_newline(uint32_t cp)
{
    switch (cp) {
    case 0x0a: // new line
    case 0x0b: // vertical tab
    case 0x0c: // form feed
    case 0x0d: // carriage return
    case 0x85: // NEL next line
    case 0x2028: // line separator
    case 0x2029: // paragraph separator
        return 1;
    default:
        return 0;
    }
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
        // p is not at a valid starting point. p is not utf8 encoded or is at a bad offset.
        assert(0);
        *cp_out = 0;
        return 1;
    }
}

static GB_ERROR _GB_TextUpdateCache(struct GB_Context *gb, struct GB_Text *text)
{
    assert(gb);
    assert(text);

    // hold a temp array of glyph ptrs, this is so we can sort glyphs by height before
    // adding them to the GlyphCache, which improves texture utilization for long strings of glyphs.
    int num_glyphs = hb_buffer_get_length(text->hb_buffer);
    struct GB_Glyph **glyph_ptrs = (struct GB_Glyph**)malloc(sizeof(struct GB_Glyph*) * num_glyphs);
    int num_glyph_ptrs = 0;

    struct GB_Cache *cache = gb->cache;

    assert(text->hb_buffer);

    // prepare to iterate over all the glyphs in the hb_buffer
    hb_glyph_info_t *glyphs = hb_buffer_get_glyph_infos(text->hb_buffer, NULL);

    // iterate over each glyph
    int i;
    for (i = 0; i < num_glyphs; i++) {
        int index = glyphs[i].codepoint;

        // check to see if this glyph already exists in the context
        struct GB_Glyph *glyph = GB_ContextHashFind(gb, index, text->font->index);
        if (!glyph) {
            // check to see if this glyph already exists in the cache
            glyph = GB_CacheHashFind(cache, index, text->font->index);
            if (!glyph) {

                // get FreeType face
                FT_Face ft_face = text->font->ft_face;

                // skip new-lines
                uint32_t cp;
                utf8_next_cp(text->utf8_string + glyphs[i].cluster, &cp);
                if (!is_newline(cp)) {

                    // will rasterize and initialize glyph
                    GB_ERROR gb_error = GB_GlyphMake(index, text->font->index, ft_face, &glyph);
                    if (gb_error)
                        return gb_error;

                    // add to glyph_ptr array
                    glyph_ptrs[num_glyph_ptrs++] = glyph;

                    printf("AJT: adding glyph %u to context (%d x %d) image = %p\n", index, glyph->size[0], glyph->size[1], glyph->image);
                }
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

#define MAX_NUM_RUNS 1000
static uint32_t s_run_counts[MAX_NUM_RUNS];

static GB_ERROR _GB_MakeGlyphQuadRuns(struct GB_Context *gb, struct GB_Text *text)
{
    // iterate over each glyph and build runs.
    int num_glyphs = hb_buffer_get_length(text->hb_buffer);
    hb_glyph_info_t *glyphs = hb_buffer_get_glyph_infos(text->hb_buffer, NULL);

    int32_t i = 0;
    uint32_t num_runs = 0;
    uint32_t num_glyphs_in_run = 0;
    uint32_t cp;
    int32_t x = 0;
    int32_t word_start = 0;
    int32_t word_end = 0;
    int32_t within_word = 0;
    for (i = 0; i < num_glyphs; i++) {
        utf8_next_cp(text->utf8_string + glyphs[i].cluster, &cp);

        // apply kerning
        int32_t dx = 0;
        if (i > 0)
        {
            FT_Vector delta;
            FT_Get_Kerning(text->font->ft_face, glyphs[i-1].codepoint, glyphs[i].codepoint,
                           FT_KERNING_DEFAULT, &delta);
            dx = FIXED_TO_INT(delta.x);
        }

        // keep track of if we are within a word or not.
        if (within_word) {
            if (is_space(cp) || is_newline(cp)) {
                // end of word
                within_word = 0;
                word_end = i;
            } else {
                // continue word
            }
        } else {
            if (is_space(cp) || is_newline(cp)) {
                // continue non-word
            } else {
                within_word = 1;
                word_start = i;
            }
        }

        if (is_newline(cp)) {
            s_run_counts[num_runs] = num_glyphs_in_run;
            num_glyphs_in_run = 0;
            num_runs++;
            x = 0; // reset x
        } else {
            // advance pen.
            struct GB_Glyph *glyph = GB_ContextHashFind(gb, glyphs[i].codepoint, text->font->index);
            if (glyph) {
                if (x + glyph->bearing[0] + glyph->size[0] + dx > text->size[0]) {

                    printf("XXX: wordWRap x = %d, i = %d\n", x, i);
                    // TODO: fix HORIBLE code structure.
                    // word wrap!
                    // new-line!
                    s_run_counts[num_runs] = num_glyphs_in_run;
                    num_glyphs_in_run = 0;
                    num_runs++;
                    x = 0; // reset x
                    i--;
                    continue;
                }
                x += glyph->advance + dx;
            }

            // don't count spaces
            if (!is_space(cp)) {
                num_glyphs_in_run++;
            }
        }
    }
    s_run_counts[num_runs] = num_glyphs_in_run;
    num_runs++;  // end the last run.


    // Allocate runs
    text->glyph_quad_runs = (struct GB_GlyphQuadRun*)malloc(sizeof(struct GB_GlyphQuadRun) * num_runs);
    text->num_glyph_quad_runs = num_runs;

    printf("XXX: num_runs = %d\n", num_runs);
    for (i = 0; i < num_runs; i++) {
        printf("XXX: runs[i].num_glyph_quads = %d\n", s_run_counts[i]);
        struct GB_GlyphQuadRun *run = text->glyph_quad_runs + i;
        run->glyph_quads = (struct GB_GlyphQuad*)malloc(sizeof(struct GB_GlyphQuad) * s_run_counts[i]);
        run->num_glyph_quads = 0;
        run->origin[0] = 0;
        run->origin[1] = 0;
        run->size[0] = 0;
        run->size[1] = 0; // TODO: keep track of this!
    }

    const float texture_size = (float)gb->cache->texture_size;
    int32_t line_height = FIXED_TO_INT(text->font->ft_face->size->metrics.height);
    int32_t pen[2] = {text->origin[0], text->origin[1]};

    pen[0] = text->origin[0];
    pen[1] = pen[1] + line_height;

    struct GB_GlyphQuadRun *run = text->glyph_quad_runs;
    for (i = 0; i < num_glyphs; i++) {

        int32_t dx = 0;
        // apply kerning
        if (i > 0)
        {
            FT_Vector delta;
            FT_Get_Kerning(text->font->ft_face, glyphs[i-1].codepoint, glyphs[i].codepoint,
                           FT_KERNING_DEFAULT, &delta);
            dx = FIXED_TO_INT(delta.x);
        }

        // TODO: word wrapping
        utf8_next_cp(text->utf8_string + glyphs[i].cluster, &cp);
        if (is_newline(cp)) {
            pen[0] = text->origin[0];
            pen[1] += line_height;
            run++;
        } else {
            // dont add space glyphs, but increment pen.
            struct GB_Glyph *glyph = GB_ContextHashFind(gb, glyphs[i].codepoint, text->font->index);
            if (glyph) {
                if (pen[0] + glyph->bearing[0] + glyph->size[0] + dx > text->origin[0] + text->size[0]) {

                    printf("> XXX: wordWRap x = %d, i = %d\n", pen[0], i);
                    // TODO: fix HORIBLE code structure.
                    // word wrap!
                    pen[0] = text->origin[0];
                    pen[1] += line_height;
                    run++;
                    i--;
                    continue;
                }

                if (!is_space(cp)) {

                    assert(glyph->size[0] > 0 && glyph->size[1] > 0);

                    // NOTE: y axis points down, quad origin is upper-left corner of glyph
                    // build quad
                    struct GB_GlyphQuad *quad = run->glyph_quads + run->num_glyph_quads;
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
                    run->num_glyph_quads++;
                }

                pen[0] += glyph->advance + dx;
            }
        }
    }

    return GB_ERROR_NONE;
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
            text->utf8_string_len = utf8_string_len;

            // create harfbuzz buffer
            text->hb_buffer = hb_buffer_create();
            hb_buffer_set_direction(text->hb_buffer, HB_DIRECTION_LTR);
            hb_buffer_add_utf8(text->hb_buffer, (const char*)text->utf8_string, text->utf8_string_len,
                               0, text->utf8_string_len);

            text->color = color;
            text->origin[0] = origin[0];
            text->origin[1] = origin[1];
            text->size[0] = size[0];
            text->size[1] = size[1];
            text->horizontal_align = horizontal_align;
            text->vertical_align = vertical_align;
            text->glyph_quad_runs = NULL;
            text->num_glyph_quad_runs = 0;

            // Use harf-buzz to perform glyph shaping
            hb_shape(text->font->hb_font, text->hb_buffer, NULL, 0);

            // Insert new glyphs into cache
            // This is where glyph rasterization occurs.
            GB_ERROR ret = _GB_TextUpdateCache(gb, text);
            if (ret != GB_ERROR_NONE)
                return ret;

            // Build array of GlyphQuadRuns, one for each line.
            // This is where word-wrapping and justification occurs.
            ret = _GB_MakeGlyphQuadRuns(gb, text);
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

    // prepare to iterate over all the glyphs in the hb_buffer
    int num_glyphs = hb_buffer_get_length(text->hb_buffer);
    hb_glyph_info_t *glyphs = hb_buffer_get_glyph_infos(text->hb_buffer, NULL);

    // remove each glyph from context
    int i, font_index = text->font->index;
    for (i = 0; i < num_glyphs; i++) {
        GB_ContextHashRemove(gb, glyphs[i].codepoint, font_index);
    }
    hb_buffer_destroy(text->hb_buffer);

    // delete each glyph_quad_run
    uint32_t j;
    for (j = 0; j < text->num_glyph_quad_runs; j++) {
        free(text->glyph_quad_runs[j].glyph_quads);
    }
    free(text->glyph_quad_runs);

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
            uint32_t i;
            for (i = 0; i < text->num_glyph_quad_runs; i++) {
                func(text->glyph_quad_runs[i].glyph_quads, text->glyph_quad_runs[i].num_glyph_quads);
            }
            return GB_ERROR_NONE;
        }
        else {
            return GB_ERROR_NOIMP;
        }
    } else {
        return GB_ERROR_INVAL;
    }
}
