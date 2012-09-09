#ifndef GB_TEXT_H
#define GB_TEXT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <harfbuzz/hb.h>
#include "gb_error.h"

typedef enum GB_Horizontal_Align {
    GB_HORIZONTAL_ALIGN_LEFT = 0,
    GB_HORIZONTAL_ALIGN_RIGHT,
    GB_HORIZONTAL_ALIGN_CENTER
} GB_HORIZONTAL_ALIGN;

typedef enum GB_Vertical_Align {
    GB_VERTICAL_ALIGN_TOP = 0,
    GB_VERTICAL_ALIGN_BOTTOM,
    GB_VERTICAL_ALIGN_CENTER
} GB_VERTICAL_ALIGN;

// y axis points down
// origin is upper-left corner of glyph
struct GB_GlyphQuad {
    uint32_t origin[2];
    uint32_t size[2];
    float uv_origin[2];
    float uv_size[2];
    uint32_t color;
    uint32_t gl_tex_obj;
};

struct GB_GlyphQuadRun {
    struct GB_GlyphQuad *glyph_quads;
    uint32_t num_glyph_quads;
    uint32_t origin[2];
    uint32_t size[2];
};

struct GB_Text {
    int32_t rc;
    struct GB_Font *font;
    uint8_t *utf8_string;
    uint32_t utf8_string_len; // in bytes (not including null term)
    hb_buffer_t *hb_buffer;
    uint32_t color;  // ABGR
    uint32_t origin[2];
    uint32_t size[2];
    GB_HORIZONTAL_ALIGN horizontal_align;
    GB_VERTICAL_ALIGN vertical_align;
    struct GB_GlyphQuadRun *glyph_quad_runs;
    uint32_t num_glyph_quad_runs;
};

GB_ERROR GB_TextMake(struct GB_Context *gb, const uint8_t *utf8_string,
                     struct GB_Font *font, uint32_t color, uint32_t origin[2],
                     uint32_t size[2], GB_HORIZONTAL_ALIGN horizontal_align,
                     GB_VERTICAL_ALIGN vertical_align, struct GB_Text **text_out);
GB_ERROR GB_TextRetain(struct GB_Context *gb, struct GB_Text *text);
GB_ERROR GB_TextRelease(struct GB_Context *gb, struct GB_Text *text);

// this should really be in gb_context.h, but is not to prevent circular deps.
typedef void (*GB_TextRenderFunc)(struct GB_GlyphQuad *quads, uint32_t num_quads);
GB_ERROR GB_ContextSetTextRenderFunc(struct GB_Context *gb, GB_TextRenderFunc func);

// Renders given text using renderer func.
GB_ERROR GB_TextDraw(struct GB_Context *gb, struct GB_Text *text);

#ifdef __cplusplus
}
#endif

#endif // GB_TEXT_H
