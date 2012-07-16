#ifdef __cplusplus
extern "C" {
#endif

#ifndef GLYPH_BLASTER_H
#define GLYPH_BLASTER_H

#include <stdint.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <harfbuzz/hb.h>
#include <harfbuzz/hb-ft.h>

// Glyph blaster context.
typedef struct GB_Gb {
    FT_Library ft_library;
    void* glyph_cache;
    uint32_t next_index;
} GB_GB;

typedef struct GB_Font {
    int rc;
    uint32_t index;
    FT_Face ft_face;
    hb_font_t* hb_font;
} GB_FONT;

typedef struct GB_Text {
    int rc;
    GB_FONT* font;
    hb_buffer_t* hb_buffer;
    char* utf8_string;
    uint32_t color;
    uint32_t min[2];
    uint32_t max[2];
    uint32_t horizontal_align;
    uint32_t vertical_align;
} GB_TEXT;

typedef enum GB_Error_Code {
    GB_ERROR_NONE = 0,
    GB_ERROR_NOENT,  // invalid file
    GB_ERROR_NOMEM,  // out of memory
    GB_ERROR_INVAL,  // invalid argument
    GB_ERROR_NOIMP,  // not implemented
    GB_ERROR_FTERR,  // freetype2 error
    GB_ERROR_NUM_ERRORS
} GB_ERROR;

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

typedef struct GB_TextMetrics {
    uint32_t min[2];
    uint32_t max[2];
} GB_TEXT_METRICS;

typedef struct GB_GlyphQuad {
    uint32_t min[2];
    uint32_t max[2];
    float min_uv[2];
    float max_uv[2];
    uint32_t color;
} GB_GLYPH_QUAD;

GB_ERROR GB_Init(GB_GB** gb_out);
GB_ERROR GB_Shutdown(GB_GB* gb);

GB_ERROR GB_MakeFont(GB_GB* gb, const char* filename, uint32_t point_size,
                     GB_FONT** font_out);
GB_ERROR GB_ReferenceFont(GB_GB* gb, GB_FONT* font);
GB_ERROR GB_ReleaseFont(GB_GB* gb, GB_FONT* font);

GB_ERROR GB_MakeText(GB_GB* gb, const char* utf8_string, GB_FONT* font,
                     uint32_t color, uint32_t min[2], uint32_t max[2],
                     GB_HORIZONTAL_ALIGN horizontal_align,
                     GB_VERTICAL_ALIGN vertical_align,
                     GB_TEXT** text_out);
GB_ERROR GB_ReferenceText(GB_GB* gb, GB_TEXT* text);
GB_ERROR GB_ReleaseText(GB_GB* gb, GB_TEXT* text);

// not implmented
GB_ERROR GB_GetTextMetrics(GB_GB* gb, const char* utf8_string,
                           GB_FONT* font, uint32_t min[2], uint32_t max[2],
                           GB_HORIZONTAL_ALIGN horizontal_align,
                           GB_VERTICAL_ALIGN vertical_align,
                           GB_TEXT_METRICS* text_metrics_out);

// Renders given text using renderer func.
GB_ERROR GB_DrawText(GB_GB* gb, GB_TEXT* text);

const char* GB_ErrorToString(GB_ERROR err);

// Renderer interface
void RenderGlyphs(uint32_t texture, GB_GLYPH_QUAD* glyphs, uint32_t numGlyphs);

#ifdef __cplusplus
}
#endif

#endif GLYPH_BLASTER_H
