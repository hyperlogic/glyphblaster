#include <stdio.h>
#include <assert.h>
#include "gb_font.h"
#include "gb_glyph.h"

// 26.6 fixed to int (truncates)
#define FIXED_TO_INT(n) (uint32_t)(n >> 6)

GB_ERROR GB_GlyphMake(uint32_t index, struct GB_Font *font, struct GB_Glyph **glyph_out)
{
    if (glyph_out && font && font->ft_face) {

        FT_Face ft_face = font->ft_face;

        uint32_t load_flags;
        switch (font->hint_options) {
        default:
        case GB_HINT_DEFAULT: load_flags = FT_LOAD_DEFAULT; break;
        case GB_HINT_FORCE_AUTO: load_flags = FT_LOAD_FORCE_AUTOHINT; break;
        case GB_HINT_NO_AUTO: load_flags = FT_LOAD_NO_AUTOHINT; break;
        case GB_HINT_NONE: load_flags = FT_LOAD_NO_HINTING; break;
        }
        switch (font->render_options) {
        default:
        case GB_RENDER_NORMAL: load_flags |= FT_LOAD_TARGET_NORMAL; break;
        case GB_RENDER_LIGHT: load_flags |= FT_LOAD_TARGET_LIGHT; break;
        case GB_RENDER_MONO: load_flags |= FT_LOAD_TARGET_MONO; break;
        case GB_RENDER_LCD_RGB: 
        case GB_RENDER_LCD_BGR: load_flags |= FT_LOAD_TARGET_LCD; break;
        case GB_RENDER_LCD_RGB_V:
        case GB_RENDER_LCD_BGR_V: load_flags |= FT_LOAD_TARGET_LCD_V; break;
        }

        FT_Error ft_error = FT_Load_Glyph(ft_face, index, load_flags);
        if (ft_error)
            return GB_ERROR_FTERR;

        FT_Render_Mode render_mode;
        switch (font->render_options) {
        default:
        case GB_RENDER_NORMAL: render_mode = FT_RENDER_MODE_NORMAL; break;
        case GB_RENDER_LIGHT: render_mode = FT_RENDER_MODE_LIGHT; break;
        case GB_RENDER_MONO: render_mode = FT_RENDER_MODE_MONO; break;
        case GB_RENDER_LCD_RGB:
        case GB_RENDER_LCD_BGR: render_mode = FT_RENDER_MODE_LCD; break;
        case GB_RENDER_LCD_RGB_V:
        case GB_RENDER_LCD_BGR_V: render_mode = FT_RENDER_MODE_LCD_V; break;
        }

        // render glyph into ft_face->glyph->bitmap
        ft_error = FT_Render_Glyph(ft_face->glyph, render_mode);
        if (ft_error)
            return GB_ERROR_FTERR;

        // record post-hinted advance and bearing.
        uint32_t advance = FIXED_TO_INT(ft_face->glyph->metrics.horiAdvance);
        uint32_t bearing[2] = {FIXED_TO_INT(ft_face->glyph->metrics.horiBearingX),
                               FIXED_TO_INT(ft_face->glyph->metrics.horiBearingY)};

        FT_Bitmap *ft_bitmap = &ft_face->glyph->bitmap;
        uint8_t *image = NULL;
        if (ft_bitmap->width > 0 && ft_bitmap->rows > 0) {
            // allocate an image to hold a copy of the rasterized glyph
            image = (uint8_t*)malloc(sizeof(uint8_t) * ft_bitmap->width * ft_bitmap->rows);

            // copy image from ft_bitmap.buffer into image, row by row.
            // The pitch of each row in the ft_bitmap maybe >= width,
            // so we cant do this as a single memcpy.
            int i;
            for (i = 0; i < ft_face->glyph->bitmap.rows; i++) {
                memcpy(image + (i * ft_bitmap->width),
                       ft_bitmap->buffer + (i * ft_bitmap->pitch),
                       ft_bitmap->width);
            }
        }

        uint32_t origin[2] = {0, 0};
        uint32_t size[2] = {ft_bitmap->width, ft_bitmap->rows};

        struct GB_Glyph *glyph = (struct GB_Glyph*)malloc(sizeof(struct GB_Glyph));
        if (glyph) {
            uint64_t key = ((uint64_t)font->index << 32) | index;
            glyph->key = key;
            glyph->rc = 1;
            glyph->index = index;
            glyph->font_index = font->index;
            glyph->gl_tex_obj = 0;
            glyph->origin[0] = origin[0];
            glyph->origin[1] = origin[1];
            glyph->size[0] = size[0];
            glyph->size[1] = size[1];
            glyph->advance = advance;
            glyph->bearing[0] = bearing[0];
            glyph->bearing[1] = bearing[1];
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

GB_ERROR GB_GlyphRetain(struct GB_Glyph *glyph)
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

static void _GB_GlyphDestroy(struct GB_Glyph *glyph)
{
    printf("AJT: deleting glyph %d\n", glyph->index);
    assert(glyph);
    if (glyph->image)
        free(glyph->image);
    free(glyph);
}

GB_ERROR GB_GlyphRelease(struct GB_Glyph *glyph)
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
