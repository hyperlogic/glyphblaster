#ifndef GLYPH_CACHE_H
#define GLYPH_CACHE_H

#include "../include/glyphblaster.h"
#include <stdint.h>

// all openGL textures in the cache have this size.
#define GB_TEXTURE_SIZE 1024

typedef struct GB_Glyph {
    uint32_t index;
    uint32_t font_index;
    uint32_t origin_x;
    uint32_t origin_y;
    uint32_t size_x;
    uint32_t size_y;
    uint8_t* image;
} GB_GLYPH;

#define GB_MAX_GLYPHS_PER_LEVEL 128
typedef struct GB_GlyphSheetLevel {
    GB_GLYPH* glyph[GB_MAX_GLYPHS_PER_LEVEL];
    uint32_t baseline;
    uint32_t height;
    uint32_t num_glyphs;
} GB_GLYPH_SHEET_LEVEL;

#define GB_MAX_LEVELS_PER_SHEET 64
typedef struct GB_GlyphSheet {
    uint32_t gl_tex;
    GB_GLYPH_SHEET_LEVEL level[GB_MAX_LEVELS_PER_SHEET];
    uint32_t num_levels;
} GB_GLYPH_SHEET;

#define GB_MAX_SHEETS_PER_CACHE 1
typedef struct GB_GlyphCache {
    GB_GLYPH_SHEET sheet[GB_MAX_SHEETS_PER_CACHE];
    uint32_t num_sheets;
} GB_GLYPH_CACHE;

GB_ERROR GB_GlyphCache_Make(GB_GLYPH_CACHE** cache_out);
GB_ERROR GB_GlyphCache_Free(GB_GLYPH_CACHE* cache);

// uses index & font_index as keys to look up into the glyph cache
// returns ptr if present in cache, NULL otherwise
GB_GLYPH* GB_GlyphCache_Find(GB_GLYPH_CACHE* glyph, uint32_t index, uint32_t font_index);

// will add the given glyphs to the glyph_cache.
GB_ERROR GB_GlyphCache_Insert(GB_GLYPH_CACHE* glyph, GB_GLYPH* glyphs, int num_glyphs);

#endif
