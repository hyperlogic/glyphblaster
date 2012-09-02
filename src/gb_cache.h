#ifndef GLYPH_CACHE_H
#define GLYPH_CACHE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "gb_error.h"

#define GB_MAX_GLYPHS_PER_LEVEL 128
struct GB_SheetLevel {
    struct GB_Glyph *glyph[GB_MAX_GLYPHS_PER_LEVEL];
    uint32_t baseline;
    uint32_t height;
    uint32_t num_glyphs;
};

#define GB_MAX_LEVELS_PER_SHEET 64
struct GB_Sheet {
    uint32_t gl_tex_obj;
    struct GB_SheetLevel level[GB_MAX_LEVELS_PER_SHEET];
    uint32_t num_levels;
};

#define GB_MAX_SHEETS_PER_CACHE 10
struct GB_Cache {
    struct GB_Sheet sheet[GB_MAX_SHEETS_PER_CACHE];
    uint32_t num_sheets;
    uint32_t texture_size;
    struct GB_Glyph *glyph_hash;  // retains all glyphs in GB_Sheet structs.
};

GB_ERROR GB_CacheMake(uint32_t texture_size, uint32_t num_sheets, struct GB_Cache **cache_out);
GB_ERROR GB_CacheDestroy(struct GB_Cache *cache);
GB_ERROR GB_CacheInsert(struct GB_Context *gb, struct GB_Cache *cache,
                        struct GB_Glyph **glyph_ptrs, int num_glyph_ptrs);
GB_ERROR GB_CacheCompact(struct GB_Context *gb, struct GB_Cache *cache);

void GB_CacheHashAdd(struct GB_Cache *cache, struct GB_Glyph *glyph);
struct GB_Glyph *GB_CacheHashFind(struct GB_Cache *cache, uint32_t glyph_index, uint32_t font_index);

#ifdef __cplusplus
}
#endif

#endif
