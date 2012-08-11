#ifndef GB_FONT_H
#define GB_FONT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <harfbuzz/hb.h>
#include "gb_error.h"

struct GB_Context;

struct GB_Font {
    int32_t rc;
    FT_Face ft_face;
    hb_font_t* hb_font;
    struct GB_Font* prev;
    struct GB_Font* next;
    struct GB_Glyph* glyph_hash;
};

GB_ERROR GB_FontMake(struct GB_Context* gb, const char* filename, uint32_t point_size,
                     struct GB_Font** font_out);
GB_ERROR GB_FontReference(struct GB_Context* gb, struct GB_Font* font);
GB_ERROR GB_FontRelease(struct GB_Context* gb, struct GB_Font* font);

#ifdef __cplusplus
}
#endif

#endif // GB_FONT_H
