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
    uint32_t pen[2];
    uint32_t origin[2];
    uint32_t size[2];
    float uv_origin[2];
    float uv_size[2];
    void *user_data;
    uint32_t gl_tex_obj;
};

// text object
// reference counted
struct GB_Text {
    int32_t rc;
    struct GB_Font *font;
    uint8_t *utf8_string;
    uint32_t utf8_string_len; // in bytes (not including null term)
    hb_buffer_t *hb_buffer;  // harfbuzz buffer, used for shaping
    void *user_data;
    uint32_t origin[2];  // bounding rectangle, used for word-wrapping & alignment
    uint32_t size[2];
    GB_HORIZONTAL_ALIGN horizontal_align;
    GB_VERTICAL_ALIGN vertical_align;
    struct GB_GlyphQuad *glyph_quads;
    uint32_t num_glyph_quads;
};

// NOTE: ownership of memory pointed to by user_data is passed to text.
// it will be deallocated when the text object ref-count goes to zero with free().
GB_ERROR GB_TextMake(struct GB_Context *gb, const uint8_t *utf8_string,
                     struct GB_Font *font, void* user_data, uint32_t origin[2],
                     uint32_t size[2], GB_HORIZONTAL_ALIGN horizontal_align,
                     GB_VERTICAL_ALIGN vertical_align, struct GB_Text **text_out);
GB_ERROR GB_TextRetain(struct GB_Context *gb, struct GB_Text *text);
GB_ERROR GB_TextRelease(struct GB_Context *gb, struct GB_Text *text);

// renders given text using renderer func.
GB_ERROR GB_TextDraw(struct GB_Context *gb, struct GB_Text *text);

#ifdef __cplusplus
}
#endif

#endif // GB_TEXT_H
