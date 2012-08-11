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
struct GB_SheetLevel {
    struct GB_Glyph* glyph[GB_MAX_GLYPHS_PER_LEVEL];
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

#define GB_MAX_SHEETS_PER_CACHE 1
struct GB_Cache {
    struct GB_Sheet sheet[GB_MAX_SHEETS_PER_CACHE];
    uint32_t num_sheets;
    struct GB_Glyph* glyph_hash;
};

GB_ERROR GB_CacheMake(struct GB_Cache** cache_out);
GB_ERROR GB_CacheFree(struct GB_Cache* cache);
GB_ERROR GB_CacheInsert(struct GB_Context* gb, struct GB_Cache* glyph,
                        struct GB_Glyph** glyph_ptrs, int num_glyph_ptrs);

#ifdef __cplusplus
}
#endif

#endif
