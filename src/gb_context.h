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
    struct GB_GlyphCache* glyph_cache;
    struct GB_Font* font_list;
};

GB_ERROR GB_ContextMake(struct GB_Context** gb_out);
GB_ERROR GB_ContextReference(struct GB_Context* gb);
GB_ERROR GB_ContextRelease(struct GB_Context* gb);

#ifdef __cplusplus
}
#endif

#endif // GB_CONTEXT_H
