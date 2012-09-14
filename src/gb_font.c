#include <harfbuzz/hb.h>
#include <harfbuzz/hb-ft.h>
#include "utlist.h"
#include "uthash.h"
#include "gb_context.h"
#include "gb_glyph.h"
#include "gb_cache.h"
#include "gb_font.h"

GB_ERROR GB_FontMake(struct GB_Context *gb, const char *filename, uint32_t point_size, 
                     enum GB_FontRenderOptions render_options, enum GB_FontHintOptions hint_options,
                     struct GB_Font **font_out)
{
    if (gb && filename && font_out) {

        // create freetype face
        FT_Face face = NULL;
        FT_New_Face(gb->ft_library, filename, 0, &face);
        if (face) {
            struct GB_Font *font = (struct GB_Font*)malloc(sizeof(struct GB_Font));
            if (font) {
                memset(font, 0, sizeof(struct GB_Font));

                font->rc = 1;
                font->index = gb->next_font_index++;
                font->ft_face = face;

                FT_Set_Char_Size(font->ft_face, (int)(point_size * 64), 0, 72, 72);

                // create harfbuzz font
                font->hb_font = hb_ft_font_create(font->ft_face, 0);
                hb_ft_font_set_funcs(font->hb_font);

                // context holds a list of all fonts
                DL_PREPEND(gb->font_list, font);

                font->render_options = render_options;
                font->hint_options = hint_options;

                *font_out = font;
                return GB_ERROR_NONE;
            } else {
                return GB_ERROR_NOMEM;
            }
        } else {
            fprintf(stderr, "Error loading font \"%s\"\n", filename);
            return GB_ERROR_NOENT;
        }
    } else {
        return GB_ERROR_INVAL;
    }
}

GB_ERROR GB_FontRetain(struct GB_Context *gb, struct GB_Font *font)
{
    if (gb && font) {
        assert(font->rc > 0);
        font->rc++;
        return GB_ERROR_NONE;
    } else {
        return GB_ERROR_INVAL;
    }
}

static void _GB_FontDestroy(struct GB_Context *gb, struct GB_Font *font)
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

    // context holds a list of all fonts
    DL_DELETE(gb->font_list, font);

    free(font);
}

GB_ERROR GB_FontRelease(struct GB_Context *gb, struct GB_Font *font)
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
