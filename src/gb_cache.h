#ifndef GLYPH_CACHE_H
#define GLYPH_CACHE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "gb_error.h"

// all openGL textures in the cache have this size.
#define GB_TEXTURE_SIZE 128

#define GB_MAX_GLYPHS_PER_LEVEL 128
struct GB_GlyphSheetLevel {
    struct GB_Glyph* glyph[GB_MAX_GLYPHS_PER_LEVEL];
    uint32_t baseline;
    uint32_t height;
    uint32_t num_glyphs;
};

#define GB_MAX_LEVELS_PER_SHEET 64
struct GB_GlyphSheet {
    uint32_t gl_tex_obj;
    struct GB_GlyphSheetLevel level[GB_MAX_LEVELS_PER_SHEET];
    uint32_t num_levels;
};

#define GB_MAX_SHEETS_PER_CACHE 1
struct GB_GlyphCache {
    struct GB_GlyphSheet sheet[GB_MAX_SHEETS_PER_CACHE];
    uint32_t num_sheets;
    struct GB_Glyph* glyph_hash;
};

GB_ERROR GB_GlyphCacheMake(struct GB_GlyphCache** cache_out);
GB_ERROR GB_GlyphCacheFree(struct GB_GlyphCache* cache);

// will add the given glyphs to the glyph_cache.
GB_ERROR GB_GlyphCacheInsert(struct GB_Context* gb, struct GB_GlyphCache* glyph,
                             struct GB_Glyph** glyph_ptrs, int num_glyph_ptrs);

#ifdef __cplusplus
}
#endif

#endif
