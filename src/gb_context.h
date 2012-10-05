#ifndef GB_CONTEXT_H
#define GB_CONTEXT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include "gb_error.h"

enum GB_TextureFormat { GB_TEXTURE_FORMAT_ALPHA, GB_TEXTURE_FORMAT_RGBA = 1 };

struct GB_Context {
    int32_t rc;  // reference count
    FT_Library ft_library;  // freetype2
    struct GB_Cache *cache;  // holds textures which contain rasterized glyphs
    struct GB_Font *font_list;  // list of all GB_Font instances
    struct GB_Glyph *glyph_hash;  // retains all glyphs in use by GB_Text structs
    uint32_t next_font_index;  // counter used to uniquely identify GB_Font objects
    void *text_render_func;  // user supplied render function
    uint32_t fallback_gl_tex_obj;  // this texture is used to render glyphs which do not fit in the cache
    enum GB_TextureFormat texture_format;  // pixel format of cache textures
};

// texture_size - width of texture sheets used by glyph cache in pixels (must be power of two)
// num_sheets - number of texture sheets used by glyph cache.
// texture_format - pixel format of each texture used by the glyph cache.
//     use GB_TEXTURE_FORMAT_ALPHA unless you are using LCD sub-pixel rendering
GB_ERROR GB_ContextMake(uint32_t texture_size, uint32_t num_sheets, enum GB_TextureFormat texture_format,
                        struct GB_Context **gb_out);

// reference count
GB_ERROR GB_ContextRetain(struct GB_Context *gb);
GB_ERROR GB_ContextRelease(struct GB_Context *gb);

//GB_ERROR GB_ContextSetTextRenderFunc(struct GB_Context*, GB_TextRenderFunc func);
GB_ERROR GB_ContextCompact(struct GB_Context *gb);

// private
void GB_ContextHashAdd(struct GB_Context *gb, struct GB_Glyph *glyph);
struct GB_Glyph *GB_ContextHashFind(struct GB_Context *gb, uint32_t glyph_index, uint32_t font_index);
void GB_ContextHashRemove(struct GB_Context *gb, uint32_t glyph_index, uint32_t font_index);
struct GB_Glyph **GB_ContextHashValues(struct GB_Context *gb, uint32_t *num_ptrs_out);  // caller must free returned ptr

#ifdef __cplusplus
}
#endif

#endif // GB_CONTEXT_H
