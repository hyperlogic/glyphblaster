#include <stdio.h>
#include <assert.h>
#include "gb_glyph.h"

GB_ERROR GB_GlyphMake(uint32_t index, uint32_t font_index, uint32_t sheet_index,
                      uint32_t origin[2], uint32_t size[2], uint8_t* image,
                      struct GB_Glyph** glyph_out)
{
    if (glyph_out) {
        // allocate and init a new glyph structure
        struct GB_Glyph* glyph = (struct GB_Glyph*)malloc(sizeof(struct GB_Glyph));
        if (glyph) {
            uint64_t key = ((uint64_t)font_index << 32) | index;
            glyph->key = key;
            glyph->rc = 1;
            glyph->index = index;
            glyph->font_index = font_index;
            glyph->sheet_index = sheet_index;
            glyph->origin[0] = origin[0];
            glyph->origin[1] = origin[1];
            glyph->size[0] = size[0];
            glyph->size[1] = size[1];
            glyph->image = image;
            *glyph_out = glyph;
            return GB_ERROR_NONE;
        } else {
            return GB_ERROR_NOMEM;
        }
    } else {
        return GB_ERROR_INVAL;
    }
}

GB_ERROR GB_GlyphRetain(struct GB_Glyph* glyph)
{
    if (glyph) {
        printf("AJT: glyph->ref index = %d rc = %d++\n", glyph->index, glyph->rc);
        assert(glyph->rc > 0);
        glyph->rc++;
        return GB_ERROR_NONE;
    } else {
        return GB_ERROR_INVAL;
    }
}

static void _GB_GlyphDestroy(struct GB_Glyph* glyph)
{
    printf("AJT: deleting glyph %d\n", glyph->index);
    assert(glyph);
    if (glyph->image)
        free(glyph->image);
    free(glyph);
}

GB_ERROR GB_GlyphRelease(struct GB_Glyph* glyph)
{
    if (glyph) {
        printf("AJT: glyph->rel index = %d rc = %d--\n", glyph->index, glyph->rc);
        glyph->rc--;
        assert(glyph->rc >= 0);
        if (glyph->rc == 0) {
            _GB_GlyphDestroy(glyph);
        }
        return GB_ERROR_NONE;
    } else {
        return GB_ERROR_INVAL;
    }
}
