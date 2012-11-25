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

// cp is a utf32 codepoint
static int is_space(uint32_t cp)
{
    switch (cp) {
    case 0x0020: // space
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

// cp is a utf32 codepoint
static int is_newline(uint32_t cp)
{
    switch (cp) {
    case 0x000a: // new line
    case 0x000b: // vertical tab
    case 0x000c: // form feed
    case 0x000d: // carriage return
    case 0x0085: // NEL next line
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

                // skip new-lines
                uint32_t cp;
                utf8_next_cp(text->utf8_string + glyphs[i].cluster, &cp);
                if (!is_newline(cp)) {

                    // will rasterize and initialize glyph
                    GB_ERROR gb_error = GB_GlyphMake(gb, index, text->font, &glyph);
                    if (gb_error)
                        return gb_error;

                    // add to glyph_ptr array
                    glyph_ptrs[num_glyph_ptrs++] = glyph;
                }
            } else {
                // add glyph to context
                GB_ContextHashAdd(gb, glyph);
            }
        }
    }

    // add glyphs to cache and context
    GB_CacheInsert(gb, gb->cache, glyph_ptrs, num_glyph_ptrs);
    free(glyph_ptrs);

    return GB_ERROR_NONE;
}

enum GlyphType { NEWLINE_GLYPH = 0, SPACE_GLYPH, NORMAL_GLYPH };

struct GB_GlyphInfo {
    enum GlyphType type;
    hb_glyph_info_t *hb_glyph;
    struct GB_Glyph *gb_glyph;
    int32_t x;
};

struct GB_GlyphInfoQueue {
    struct GB_GlyphInfo *data;
    uint32_t count;
    uint32_t capacity;
};

static void _GB_QueueMake(struct GB_GlyphInfoQueue **q_out)
{
    struct GB_GlyphInfoQueue* q = (struct GB_GlyphInfoQueue*)malloc(sizeof(struct GB_GlyphInfoQueue));
    const uint32_t initial_capacity = 8;
    q->data = (struct GB_GlyphInfo*)malloc(sizeof(struct GB_GlyphInfo) * initial_capacity);
    q->count = 0;
    q->capacity = initial_capacity;
    *q_out = q;
}

static void _GB_QueueDestroy(struct GB_GlyphInfoQueue *q)
{
    free(q->data);
    free(q);
}

static void _GB_QueuePush(struct GB_GlyphInfoQueue *q, struct GB_GlyphInfo* elem)
{
    // grow data if necessary
    if (q->count == q->capacity) {
        const uint32_t new_capacity = q->capacity * 2;
        q->data = (struct GB_GlyphInfo*)realloc(q->data, sizeof(struct GB_GlyphInfo) * new_capacity);
        q->capacity = new_capacity;
    }

    memcpy(q->data + q->count, elem, sizeof(struct GB_GlyphInfo));
    q->count++;
}

static void _GB_QueuePushGlyph(struct GB_GlyphInfoQueue *q, enum GlyphType type, hb_glyph_info_t *hb_glyph,
                               struct GB_Glyph *gb_glyph, int32_t x)
{
    struct GB_GlyphInfo info;
    info.type = type;
    info.hb_glyph = hb_glyph;
    info.gb_glyph = gb_glyph;
    info.x = x;
    _GB_QueuePush(q, &info);
}

static void _GB_QueuePopBack(struct GB_GlyphInfoQueue *q)
{
    if (q->count > 0)
        q->count--;
}

static void _GB_QueueDump(struct GB_GlyphInfoQueue *q, struct GB_Text* text)
{
    static const char *s_typeToStringMap[] = {"Newline", "Space", "Normal"};
    uint32_t i;
    uint32_t cp;
    for (i = 0; i < q->count; i++) {
        struct GB_Glyph *gb_glyph = q->data[i].gb_glyph;
        hb_glyph_info_t *hb_glyph = q->data[i].hb_glyph;
        if (gb_glyph && hb_glyph) {
            utf8_next_cp(text->utf8_string + hb_glyph->cluster, &cp);
            printf("    cp = %u\n", cp);
            printf("        type = %s\n", s_typeToStringMap[q->data[i].type]);
            printf("        x = %d\n", q->data[i].x);
            printf("        bearingX = %d\n", gb_glyph->bearing[0]);
            printf("        advance = %d\n", gb_glyph->advance);
        } else {
            printf("    cp = ???\n");
            printf("        type = %s\n", s_typeToStringMap[q->data[i].type]);
            printf("        x = %d\n", q->data[i].x);
        }
    }
}

static uint32_t loop_begin_rtl(uint32_t num_glyphs) { return num_glyphs - 1; }
static uint32_t loop_begin_ltr(uint32_t num_glyphs) { return 0; }

static uint32_t loop_end_rtl(uint32_t num_glyphs) { return -1; }
static uint32_t loop_end_ltr(uint32_t num_glyphs) { return num_glyphs; }

static uint32_t loop_next_rtl(uint32_t i) { return i - 1; }
static uint32_t loop_next_ltr(uint32_t i) { return i + 1; }

// TODO: fix inf. loop if fit always returns false.
static int loop_fit_ltr(int32_t pen_x, uint32_t advance, int32_t kern, uint32_t size) { return (pen_x + advance + kern) <= size; }
static int loop_fit_rtl(int32_t pen_x, uint32_t advance, int32_t kern, uint32_t size) { return (-pen_x + advance + kern) <= size; }

static int32_t loop_advance_ltr(int32_t pen_x, uint32_t advance, int32_t kern) { return pen_x + advance + kern; }
static int32_t loop_advance_rtl(int32_t pen_x, uint32_t advance, int32_t kern) { return pen_x - advance - kern; }
static int32_t loop_advance_none(int32_t pen_x, uint32_t advance, int32_t kern) { return pen_x; }

typedef uint32_t (*iter_func_t)(uint32_t i);
typedef int (*fit_func_t)(int32_t pen_x, uint32_t advance, int32_t kern, uint32_t size);
typedef int32_t (*advance_func_t)(int32_t pen_x, uint32_t advance, int32_t kern);

static GB_ERROR _GB_MakeGlyphQuadRuns(struct GB_Context *gb, struct GB_Text *text)
{
    // iterate over each glyph and build runs.
    uint32_t num_glyphs = hb_buffer_get_length(text->hb_buffer);
    hb_glyph_info_t *glyphs = hb_buffer_get_glyph_infos(text->hb_buffer, NULL);
    hb_direction_t dir = hb_buffer_get_direction(text->hb_buffer);

    // create a queue to hold word-wrapped glyphs
    struct GB_GlyphInfoQueue *q;
    _GB_QueueMake(&q);

    fit_func_t fit;
    advance_func_t pre_advance, post_advance;
    iter_func_t begin, end, next, prev;
    if (dir == HB_DIRECTION_RTL)
    {
        fit = loop_fit_rtl;
        pre_advance = loop_advance_rtl;
        post_advance = loop_advance_none;
        begin = loop_begin_rtl;
        end = loop_end_rtl;
        next = loop_next_rtl;
        prev = loop_next_ltr;
    } else {
        fit = loop_fit_ltr;
        pre_advance = loop_advance_none;
        post_advance = loop_advance_ltr;
        begin = loop_begin_ltr;
        end = loop_end_ltr;
        next = loop_next_ltr;
        prev = loop_next_rtl;
    }

    int32_t i = 0;
    int32_t pen_x = 0;
    uint32_t cp;
    int32_t inside_word = 0;
    uint32_t word_start_i = 0, word_end_i = 0;
    int32_t word_start_x = 0, word_end_x = 0;
    for (i = begin(num_glyphs); i != end(num_glyphs); i = next(i)) {
        // NOTE: cluster is an offset to the first byte in the utf8 encoded string which represents this glyph.
        utf8_next_cp(text->utf8_string + glyphs[i].cluster, &cp);

        // lookup kerning
        int32_t kern = 0;
        if (next(i) != end(num_glyphs)) {
            FT_Vector delta;
            FT_Get_Kerning(text->font->ft_face, glyphs[i].codepoint, glyphs[next(i)].codepoint,
                           FT_KERNING_DEFAULT, &delta);
            kern = FIXED_TO_INT(delta.x);
        }

        if (is_newline(cp)) {
            _GB_QueuePushGlyph(q, NEWLINE_GLYPH, NULL, NULL, pen_x);
            pen_x = 0;
            inside_word = 0;
        } else {
            struct GB_Glyph *glyph = GB_ContextHashFind(gb, glyphs[i].codepoint, text->font->index);
            assert(glyph);

            if (inside_word) {
                // does glyph fit on this line?
                if (fit(pen_x, glyph->advance, kern, text->size[0])) {
                    pen_x = pre_advance(pen_x, glyph->advance, kern);
                    if (is_space(cp)) {
                        _GB_QueuePushGlyph(q, SPACE_GLYPH, glyphs + i, glyph, pen_x);
                        // exiting word
                        word_end_i = i;
                        word_end_x = pen_x;
                        inside_word = 0;
                    } else {
                        _GB_QueuePushGlyph(q, NORMAL_GLYPH, glyphs + i, glyph, pen_x);
                    }
                    pen_x = post_advance(pen_x, glyph->advance, kern);
                } else {
                    if (is_space(cp)) {
                        // skip spaces
                        while (is_space(cp)) {
                            i = next(i);
                            utf8_next_cp(text->utf8_string + glyphs[i].cluster, &cp);
                        }
                        prev(i);
                    } else {
                        // if the current word starts at the beginning of the line.
                        if (word_start_x == 0) {
                            // we will have to split the word in the middle.
                            i = prev(i);
                        } else {
                            // backtrack to one before word_start_i, and restore pen_x
                            while (i != prev(word_start_i)) {
                                if (dir == HB_DIRECTION_LTR)
                                    pen_x = q->data[q->count-1].x;
                                _GB_QueuePopBack(q);
                                if (dir == HB_DIRECTION_RTL)
                                    pen_x = q->data[q->count-1].x;
                                i = prev(i);
                            }
                        }
                    }
                    _GB_QueuePushGlyph(q, NEWLINE_GLYPH, NULL, NULL, pen_x);
                    pen_x = 0;
                    inside_word = 0;
                }
            } else { // !inside_word
                // does glyph fit on this line?
                if (fit(pen_x, glyph->advance, kern, text->size[0])) {
                    pen_x = pre_advance(pen_x, glyph->advance, kern);
                    if (is_space(cp)) {
                        _GB_QueuePushGlyph(q, SPACE_GLYPH, glyphs + i, glyph, pen_x);
                    } else {
                        _GB_QueuePushGlyph(q, NORMAL_GLYPH, glyphs + i, glyph, pen_x);
                        // entering word
                        word_start_i = i;
                        word_start_x = pen_x;
                        inside_word = 1;
                    }
                    pen_x = post_advance(pen_x, glyph->advance, kern);
                } else {
                    // skip spaces
                    while (is_space(cp)) {
                        i = next(i);
                        utf8_next_cp(text->utf8_string + glyphs[i].cluster, &cp);
                    }
                    i = prev(i); // backup one char, so the next iteration thru the loop will be a non-space character
                    _GB_QueuePushGlyph(q, NEWLINE_GLYPH, NULL, NULL, word_end_x);
                    pen_x = 0;
                    inside_word = 0;
                }
            }
        }
    }
    // end with a new line, (makes justification easier)
    _GB_QueuePushGlyph(q, NEWLINE_GLYPH, NULL, NULL, pen_x);

    // allocate glyph quads
    // TODO: q->count will be slightly larger then the exact number required.
    text->glyph_quads = (struct GB_GlyphQuad*)malloc(sizeof(struct GB_GlyphQuad) * q->count);
    text->num_glyph_quads = 0;

    const float texture_size = (float)gb->cache->texture_size;
    int32_t line_height = FIXED_TO_INT(text->font->ft_face->size->metrics.height);
    int32_t y = text->origin[1] + line_height;

    /*
    printf("AJT: queue before justification!\n");
    _GB_QueueDump(q, text);
    */

    // horizontal justification
    uint32_t j;
    uint32_t line_start = 0;
    for (i = 0; i < q->count; i++) {
        if (q->data[i].type == NEWLINE_GLYPH) {
            int32_t left_edge = (dir == HB_DIRECTION_RTL) ? q->data[i].x : 0;
            int32_t right_edge = (dir == HB_DIRECTION_RTL) ? 0 : q->data[i].x;
            int32_t offset = 0;
            switch (text->horizontal_align) {
            case GB_HORIZONTAL_ALIGN_LEFT:
                offset = -left_edge;
                break;
            case GB_HORIZONTAL_ALIGN_RIGHT:
                offset = text->size[0] - right_edge;
                break;
            case GB_HORIZONTAL_ALIGN_CENTER:
                offset = (text->size[0] - left_edge - right_edge) / 2;
                break;
            }
            // apply offset to each glyphinfo.x
            for (j = line_start; j < i; j++) {
                q->data[j].x += offset;
            }
            line_start = i + 1;
        }
    }

    // initialize quads
    for (i = 0; i < q->count; i++) {
        if (q->data[i].type == NEWLINE_GLYPH) {
            y += line_height;
        } else if (q->data[i].type == NORMAL_GLYPH) {
            // NOTE: y axis points down, quad origin is upper-left corner of glyph
            // build quad
            struct GB_Glyph *gb_glyph = q->data[i].gb_glyph;
            struct GB_GlyphQuad *quad = text->glyph_quads + text->num_glyph_quads;
            quad->origin[0] = text->origin[0] + q->data[i].x + gb_glyph->bearing[0];
            quad->origin[1] = y - gb_glyph->bearing[1];
            quad->size[0] = gb_glyph->size[0];
            quad->size[1] = gb_glyph->size[1];
            quad->uv_origin[0] = gb_glyph->origin[0] / texture_size;
            quad->uv_origin[1] = gb_glyph->origin[1] / texture_size;
            quad->uv_size[0] = gb_glyph->size[0] / texture_size;
            quad->uv_size[1] = gb_glyph->size[1] / texture_size;
            quad->color = text->color;
            quad->gl_tex_obj = gb_glyph->gl_tex_obj ? gb_glyph->gl_tex_obj : gb->fallback_gl_tex_obj;
            text->num_glyph_quads++;
        }
    }

    _GB_QueueDestroy(q);

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
            hb_buffer_add_utf8(text->hb_buffer, (const char*)text->utf8_string,
                               text->utf8_string_len, 0, text->utf8_string_len);

            text->color = color;
            text->origin[0] = origin[0];
            text->origin[1] = origin[1];
            text->size[0] = size[0];
            text->size[1] = size[1];
            text->horizontal_align = horizontal_align;
            text->vertical_align = vertical_align;
            text->glyph_quads = NULL;
            text->num_glyph_quads = 0;

            // Use harf-buzz to perform glyph shaping
            hb_shape(text->font->hb_font, text->hb_buffer, NULL, 0);

            // debug print detected direction & script
            //hb_direction_t dir = hb_buffer_get_direction(text->hb_buffer);
            hb_script_t script = hb_buffer_get_script(text->hb_buffer);
            hb_tag_t tag = hb_script_to_iso15924_tag(script);
            //printf("AJT: direction = %s\n", hb_direction_to_string(dir));
            char tag_str[5];
            tag_str[0] = tag >> 24;
            tag_str[1] = tag >> 16;
            tag_str[2] = tag >> 8;
            tag_str[3] = tag;
            tag_str[4] = 0;
            //printf("AJT: script = %s\n", tag_str);

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

// Renders given text using renderer func.
GB_ERROR GB_TextDraw(struct GB_Context *gb, struct GB_Text *text)
{
    // text.glyphs.each do |glyph|
    //   build run of glyph_quads that share same opengl texture
    // end
    // send each run of glyph_quads to rendering func.
    if (gb && text) {
        if (gb->text_render_func) {
            gb->text_render_func(text->glyph_quads, text->num_glyph_quads);
            return GB_ERROR_NONE;
        }
        else {
            return GB_ERROR_NOIMP;
        }
    } else {
        return GB_ERROR_INVAL;
    }
}
