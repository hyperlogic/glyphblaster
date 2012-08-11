#include <harfbuzz/hb.h>
#include <harfbuzz/hb-ft.h>
#include "utlist.h"
#include "uthash.h"
#include "gb_context.h"
#include "gb_glyph.h"
#include "gb_font.h"

GB_ERROR GB_FontMake(struct GB_Context* gb, const char* filename,
                     uint32_t point_size, struct GB_Font** font_out)
{
    if (gb && filename && font_out) {
        struct GB_Font* font = (struct GB_Font*)malloc(sizeof(struct GB_Font));
        if (font) {
            memset(font, 0, sizeof(struct GB_Font));

            font->rc = 1;

            // create freetype face
            FT_New_Face(gb->ft_library, "dejavu-fonts-ttf-2.33/ttf/DejaVuSans.ttf", 0, &font->ft_face);
            FT_Set_Char_Size(font->ft_face, (int)(point_size * 64), 0, 72, 72);

            // create harfbuzz font
            font->hb_font = hb_ft_font_create(font->ft_face, 0);
            hb_ft_font_set_funcs(font->hb_font);

            font->glyph_hash = NULL;

            // context holds a list of all fonts
            DL_PREPEND(gb->font_list, font);

            *font_out = font;
            return GB_ERROR_NONE;
        } else {
            return GB_ERROR_NOMEM;
        }
    } else {
        return GB_ERROR_INVAL;
    }
}

GB_ERROR GB_FontReference(struct GB_Context* gb, struct GB_Font* font)
{
    if (gb && font) {
        assert(font->rc > 0);
        font->rc++;
        return GB_ERROR_NONE;
    } else {
        return GB_ERROR_INVAL;
    }
}

static void _GB_FontDestroy(struct GB_Context* gb, struct GB_Font* font)
{
    assert(font);
    assert(font->rc == 0);

    // destroy freetype face
    if (font->ft_face) {
        FT_Done_Face(font->ft_face);
    }

    // destroy harfbuzz font
    if (font->hb_font) {
        hb_font_destroy(font->hb_font);
    }

    // release all glyphs
    struct GB_Glyph *glyph, *tmp;
    HASH_ITER(font_hh, font->glyph_hash, glyph, tmp) {
        HASH_DELETE(font_hh, font->glyph_hash, glyph);
        GB_GlyphRelease(glyph);
    }

    // context holds a list of all fonts
    DL_DELETE(gb->font_list, font);

    free(font);
}

GB_ERROR GB_FontRelease(struct GB_Context* gb, struct GB_Font* font)
{
    if (gb && font) {
        font->rc--;
        assert(font->rc >= 0);
        if (font->rc == 0) {
            _GB_FontDestroy(gb, font);
        }
        return GB_ERROR_NONE;
    } else {
        return GB_ERROR_INVAL;
    }
}
