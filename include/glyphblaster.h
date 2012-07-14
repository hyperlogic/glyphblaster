#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

// Glyph blaster context.
typedef struct GB_Gb {
    // List of all fonts
    // List of all texts
    // Other resources..
} GB_GB;

typedef struct GB_Font {
    // FT_Face face;
    // atlas stuff.
} GB_FONT;

typedef struct GB_Text {
    uint32_t color;
    const char* utf8_string;
    uint32_t min[2];
    uint32_t max[2];
    uint32_t horizontal_align;
    uint32_t vertical_align;
} GB_TEXT;

typedef enum GB_Error_Code {
    GB_ERROR_NONE = 0,
    GB_ERROR_NOENT,
    GB_ERROR_NOMEM,
    GB_ERROR_INVAL,
    GB_ERROR_NOIMP,
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
GB_ERROR GB_ReleaseFont(GB_GB* gb, GB_FONT* font);

GB_ERROR GB_MakeText(GB_GB* gb, const char* utf8_string, GB_FONT* font,
                     uint32_t color, uint32_t min[2], uint32_t max[2],
                     GB_HORIZONTAL_ALIGN horizontal_align,
                     GB_VERTICAL_ALIGN vertical_align,
                     GB_TEXT** text_out);
GB_ERROR GB_ReleaseText(GB_GB* gb, GB_TEXT* text);

GB_ERROR GB_GetTextMetrics(GB_GB* gb, const char* utf8_string,
                           GB_FONT* font, uint32_t min[2], uint32_t max[2],
                           GB_HORIZONTAL_ALIGN horizontal_align,
                           GB_VERTICAL_ALIGN vertical_align,
                           GB_TEXT_METRICS* text_metrics_out);

// Creates textures, packs and subloads glyphs into texture cache.
// Should be called once a frame, before GB_DrawText
// NOTE: issues OpenGL texture bind commands.
GB_ERROR GB_Update(GB_GB* gb);

// Renders given text using renderer func.
GB_ERROR GB_DrawText(GB_GB* gb, GB_TEXT* text);

const char* GB_ErrorToString(GB_ERROR err);

// Renderer interface
void RenderGlyphs(uint32_t texture, GB_GLYPH_QUAD* glyphs, uint32_t numGlyphs);

#ifdef __cplusplus
}
#endif
