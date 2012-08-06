#ifndef GLYPH_BLASTER_H
#define GLYPH_BLASTER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <harfbuzz/hb.h>
#include <harfbuzz/hb-ft.h>
#include "uthash.h"

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

typedef struct GB_Glyph {
    int rc;
    uint32_t index;
    uint32_t sheet_index;
    uint32_t origin[2];
    uint32_t size[2];
    uint8_t* image;
    UT_hash_handle font_hh;
    UT_hash_handle cache_hh;
} GB_GLYPH;

typedef struct GB_Font {
    int rc;
    FT_Face ft_face;
    hb_font_t* hb_font;
    struct GB_Font* prev;
    struct GB_Font* next;
    GB_GLYPH* glyph_hash;
} GB_FONT;

typedef struct GB_Context {
    int rc;
    FT_Library ft_library;
    void* glyph_cache;
    GB_FONT* font_list;
} GB_CONTEXT;

typedef struct GB_Text {
    int rc;
    GB_FONT* font;
    hb_buffer_t* hb_buffer;
    char* utf8_string;
    uint32_t color;  // ABGR
    uint32_t origin[2];
    uint32_t size[2];
    GB_HORIZONTAL_ALIGN horizontal_align;
    GB_VERTICAL_ALIGN vertical_align;
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

GB_ERROR GB_ContextMake(GB_CONTEXT** gb_out);
GB_ERROR GB_ContextReference(GB_CONTEXT* gb);
GB_ERROR GB_ContextRelease(GB_CONTEXT* gb);

GB_ERROR GB_FontMake(GB_CONTEXT* gb, const char* filename, uint32_t point_size,
                     GB_FONT** font_out);
GB_ERROR GB_FontReference(GB_CONTEXT* gb, GB_FONT* font);
GB_ERROR GB_FontRelease(GB_CONTEXT* gb, GB_FONT* font);

GB_ERROR GB_TextMake(GB_CONTEXT* gb, const char* utf8_string, GB_FONT* font,
                     uint32_t color, uint32_t origin[2], uint32_t size[2],
                     GB_HORIZONTAL_ALIGN horizontal_align,
                     GB_VERTICAL_ALIGN vertical_align,
                     GB_TEXT** text_out);
GB_ERROR GB_TextReference(GB_CONTEXT* gb, GB_TEXT* text);
GB_ERROR GB_TextRelease(GB_CONTEXT* gb, GB_TEXT* text);

// not implmented
GB_ERROR GB_GetTextMetrics(GB_CONTEXT* gb, const char* utf8_string,
                           GB_FONT* font, uint32_t min[2], uint32_t max[2],
                           GB_HORIZONTAL_ALIGN horizontal_align,
                           GB_VERTICAL_ALIGN vertical_align,
                           GB_TEXT_METRICS* text_metrics_out);

// Renders given text using renderer func.
GB_ERROR GB_TextDraw(GB_CONTEXT* gb, GB_TEXT* text);

const char* GB_ErrorToString(GB_ERROR err);

// Renderer interface
//void RenderGlyphs(uint32_t texture, GB_GLYPH_QUAD* glyphs, uint32_t numGlyphs);

// private functions
GB_ERROR GB_GlyphMake(uint32_t index, uint32_t sheet_index, uint32_t origin[2],
                      uint32_t size[2], uint8_t* image, GB_GLYPH** glyph_out);
GB_ERROR GB_GlyphReference(GB_GLYPH* glyph);
GB_ERROR GB_GlyphRelease(GB_GLYPH* glyph);

#ifdef __cplusplus
}
#endif

#endif GLYPH_BLASTER_H
