#ifndef GB_CONTEXT_H
#define GB_CONTEXT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include "gb_error.h"

struct GB_Context {
    int32_t rc;
    FT_Library ft_library;
    struct GB_Cache* cache;
    struct GB_Font* font_list;
    struct GB_Glyph* glyph_hash;  // retains all glyphs in use by GB_Text structs
    uint32_t next_font_index;
    void* text_render_func;
};

// texture_size - width of texture sheets used by glyph cache in pixels (must be power of two)
// num_sheets - number of texture sheets used by glyph cache.
GB_ERROR GB_ContextMake(uint32_t texture_size, uint32_t num_sheets, struct GB_Context** gb_out);
GB_ERROR GB_ContextRetain(struct GB_Context* gb);
GB_ERROR GB_ContextRelease(struct GB_Context* gb);

//GB_ERROR GB_ContextSetTextRenderFunc(struct GB_Context*, GB_TextRenderFunc func);
GB_ERROR GB_ContextCompact(struct GB_Context* gb);

// private
void GB_ContextHashAdd(struct GB_Context* gb, struct GB_Glyph* glyph);
struct GB_Glyph* GB_ContextHashFind(struct GB_Context* gb, uint32_t glyph_index, uint32_t font_index);
void GB_ContextHashRemove(struct GB_Context* gb, uint32_t glyph_index, uint32_t font_index);
struct GB_Glyph** GB_ContextHashValues(struct GB_Context* gb, uint32_t* num_ptrs_out);  // caller must free returned ptr

#ifdef __cplusplus
}
#endif

#endif // GB_CONTEXT_H
