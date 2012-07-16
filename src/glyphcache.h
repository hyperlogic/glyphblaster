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
} GB_GLYPH;

#define GB_MAX_GLYPHS 512
typedef struct GB_GlyphSheet {
    uint32_t gl_tex;
    GB_GLYPH* glyphs[GB_MAX_GLYPHS];
    uint32_t num_glyphs;
} GB_GLYPH_SHEET;

#define GB_MAX_SHEETS 10
typedef struct GB_GlyphCache {
    GB_GLYPH_SHEET sheet[GB_MAX_SHEETS];
    uint32_t num_sheets;
} GB_GLYPH_CACHE;

GB_ERROR GB_MakeGlyphCache(GB_GLYPH_CACHE** glyph_cache_out);
GB_ERROR GB_DestroyGlyphCache(GB_GLYPH_CACHE* glyph_cache);

// uses index & font_index as keys to look up into the glyph cache
// if glyph is not in the cache gl_tex_out and glyph_out will be set to 0.
// if found the gl_tex_out will containe the openGL texutre object, and the glyph_out will hold a pointer to a GB_GLYPH structure.
GB_ERROR GB_FindInGlyphCache(GB_GLYPH_CACHE* glyph_cache, uint32_t index, uint32_t font_index, uint32_t* gl_tex_out, GB_GLYPH** glyph_out);

// will add the given glyph to the glyph_cache.
// the image data is expected to be 8-bit gray scale.
GB_ERROR GB_InsertIntoGlyphCache(GB_GLYPH_CACHE* glyph_cache, GB_GLYPH* glyph, uint8_t* image);

#endif
