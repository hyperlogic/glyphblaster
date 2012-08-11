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

struct GB_Text {
    int32_t rc;
    struct GB_Font* font;
    hb_buffer_t* hb_buffer;
    char* utf8_string;
    uint32_t color;  // ABGR
    uint32_t origin[2];
    uint32_t size[2];
    GB_HORIZONTAL_ALIGN horizontal_align;
    GB_VERTICAL_ALIGN vertical_align;
};

struct GB_TextMetrics {
    uint32_t min[2];
    uint32_t max[2];
};

struct GB_GlyphQuad {
    uint32_t min[2];
    uint32_t max[2];
    float min_uv[2];
    float max_uv[2];
    uint32_t color;
};

GB_ERROR GB_TextMake(struct GB_Context* gb, const char* utf8_string,
                     struct GB_Font* font, uint32_t color, uint32_t origin[2],
                     uint32_t size[2], GB_HORIZONTAL_ALIGN horizontal_align,
                     GB_VERTICAL_ALIGN vertical_align, struct GB_Text** text_out);
GB_ERROR GB_TextReference(struct GB_Context* gb, struct GB_Text* text);
GB_ERROR GB_TextRelease(struct GB_Context* gb, struct GB_Text* text);

// not implmented
GB_ERROR GB_GetTextMetrics(struct GB_Context* gb, const char* utf8_string,
                           struct GB_Font* font, uint32_t min[2], uint32_t max[2],
                           GB_HORIZONTAL_ALIGN horizontal_align,
                           GB_VERTICAL_ALIGN vertical_align,
                           struct GB_TextMetrics* text_metrics_out);

// not implemented
// Renders given text using renderer func.
GB_ERROR GB_TextDraw(struct GB_Context* gb, struct GB_Text* text);

#ifdef __cplusplus
}
#endif

#endif // GB_TEXT_H
