#ifndef GLYPH_CACHE_H
#define GLYPH_CACHE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "glyphblaster.h"
#include <stdint.h>

// all openGL textures in the cache have this size.
#define GB_TEXTURE_SIZE 256

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

GB_ERROR GB_GlyphCacheMake(GB_GLYPH_CACHE** cache_out);
GB_ERROR GB_GlyphCacheFree(GB_GLYPH_CACHE* cache);

// will add the given glyphs to the glyph_cache.
GB_ERROR GB_GlyphCacheInsert(GB_GLYPH_CACHE* glyph, GB_GLYPH** glyph_ptrs, int num_glyph_ptrs);

#ifdef __cplusplus
}
#endif

#endif
