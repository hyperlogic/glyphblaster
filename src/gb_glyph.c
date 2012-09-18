#include <stdio.h>
#include <assert.h>
#include "gb_font.h"
#include "gb_glyph.h"

// 26.6 fixed to int (truncates)
#define FIXED_TO_INT(n) (uint32_t)(n >> 6)

static void _InitGlyphImage(FT_Bitmap *ft_bitmap, enum GB_TextureFormat texture_format,
                            enum GB_FontRenderOptions render_options, uint8_t **image_out, uint32_t size_out[2])
{
    uint8_t *image = NULL;
    int i, j;
    if (ft_bitmap->width > 0 && ft_bitmap->rows > 0) {

        // Most of these glyph textures should be rendered using non-premultiplied alpha
        // i.e. glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        //
        // The exception is the lcd sub-pixel modes
        // These require 3 passes, one for each color channel. (Use glColorMask)
        // Also, a special shader must be used that copies a single color component into alpha.
        // For example: gl_FragColor.xyz = vec3(1, 1, 1); gl_FragColor.a = texture2D(glyph_texture, uv).r;
        // This can then be blended into the frame buffer using glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        switch (render_options) {
        case GB_RENDER_NORMAL:
        case GB_RENDER_LIGHT:
            if (texture_format == GB_TEXTURE_FORMAT_ALPHA) {
                // allocate an image to hold a copy of the rasterized glyph
                image = (uint8_t*)malloc(ft_bitmap->width * ft_bitmap->rows);

                // copy image from ft_bitmap.buffer into image, row by row.
                // The pitch of each row in the ft_bitmap maybe >= width,
                // so we cant do this as a single memcpy.
                for (i = 0; i < ft_bitmap->rows; i++) {
                    memcpy(image + (i * ft_bitmap->width), ft_bitmap->buffer + (i * ft_bitmap->pitch), ft_bitmap->width);
                }
            } else {
                // allocate an image to hold a copy of the rasterized glyph
                image = (uint8_t*)malloc(4 * ft_bitmap->width * ft_bitmap->rows);

                // copy image from ft_bitmap.buffer into image, row by row.
                // The pitch of each row in the ft_bitmap maybe >= width,
                // convert from ALPHA to RGBA
                // NOTE: This glyph texture should be rendered using non-premultiplied alpha
                // i.e. glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                for (i = 0; i < ft_bitmap->rows; i++) {
                    for (j = 0; j < ft_bitmap->width; j++) {
                        // white with alpha, i.e. non-premultiplied alpha.
                        image[i * ft_bitmap->width * 4 + (j * 4) + 0] = 0xff;
                        image[i * ft_bitmap->width * 4 + (j * 4) + 1] = 0xff;
                        image[i * ft_bitmap->width * 4 + (j * 4) + 2] = 0xff;
                        image[i * ft_bitmap->width * 4 + (j * 4) + 3] = ft_bitmap->buffer[i * ft_bitmap->pitch + j];
                    }
                }
            }
            size_out[0] = ft_bitmap->width;
            size_out[1] = ft_bitmap->rows;
            *image_out = image;
            return;
        case GB_RENDER_MONO:
            // ft_bitmap is 1 bit per pixel.
            if (texture_format == GB_TEXTURE_FORMAT_ALPHA) {
                // allocate an image to hold a copy of the rasterized glyph
                image = (uint8_t*)malloc(ft_bitmap->width * ft_bitmap->rows);

                // copy image from ft_bitmap.buffer into image
                uint8_t byte, mask;
                for (i = 0; i < ft_bitmap->rows; i++) {
                    for (j = 0; j < ft_bitmap->width; j++) {
                        byte = ft_bitmap->buffer[i * ft_bitmap->pitch + (j/8)];
                        mask = 0x1 << (7 - (j % 8));
                        image[i * ft_bitmap->width + j] = (byte & mask) ? 0xff : 0x00;
                    }
                }
            } else {
                // allocate an image to hold a copy of the rasterized glyph
                image = (uint8_t*)malloc(4 * ft_bitmap->width * ft_bitmap->rows);

                // copy image from ft_bitmap.buffer into image
                uint8_t byte, mask;
                for (i = 0; i < ft_bitmap->rows; i++) {
                    for (j = 0; j < ft_bitmap->width; j++) {
                        byte = ft_bitmap->buffer[i * ft_bitmap->pitch + (j/8)];
                        mask = 0x1 << (7 - (j % 8));
                        image[i * ft_bitmap->width * 4 + j * 4 + 0] = 0xff;
                        image[i * ft_bitmap->width * 4 + j * 4 + 1] = 0xff;
                        image[i * ft_bitmap->width * 4 + j * 4 + 2] = 0xff;
                        image[i * ft_bitmap->width * 4 + j * 4 + 3] = (byte & mask) ? 0xff : 0x00;
                    }
                }
            }
            size_out[0] = ft_bitmap->width;
            size_out[1] = ft_bitmap->rows;
            *image_out = image;
            return;
        case GB_RENDER_LCD_RGB:
        case GB_RENDER_LCD_BGR:
        case GB_RENDER_LCD_RGB_V:
        case GB_RENDER_LCD_BGR_V:
            assert(texture_format == GB_TEXTURE_FORMAT_RGBA);
            if (texture_format == GB_TEXTURE_FORMAT_RGBA) {
                // allocate an image to hold a copy of the rasterized glyph
                image = (uint8_t*)malloc(4 * ft_bitmap->width * ft_bitmap->rows);

                if (render_options == GB_RENDER_LCD_RGB || render_options == GB_RENDER_LCD_RGB_V) {
                    // copy image from ft_bitmap.buffer into image, row by row.
                    // The pitch of each row in the ft_bitmap maybe >= width,
                    // convert from RGB to RGBA
                    // TODO: Is this pre mulitplied alpha?  What should the alpha be, currently I'm just using g
                    const uint32_t width = ft_bitmap->width / 3;
                    for (i = 0; i < ft_bitmap->rows; i++) {
                        for (j = 0; j < width; j++) {
                            image[i * width * 4 + (j * 4) + 0] = ft_bitmap->buffer[i * ft_bitmap->pitch + (j * 3) + 0];
                            image[i * width * 4 + (j * 4) + 1] = ft_bitmap->buffer[i * ft_bitmap->pitch + (j * 3) + 1];
                            image[i * width * 4 + (j * 4) + 2] = ft_bitmap->buffer[i * ft_bitmap->pitch + (j * 3) + 2];
                            image[i * width * 4 + (j * 4) + 3] = 0xff;
                        }
                    }
                } else {
                    // copy image from ft_bitmap.buffer into image, row by row.
                    // The pitch of each row in the ft_bitmap maybe >= width,
                    // convert from RGB to RGBA
                    // TODO: Is this pre mulitplied alpha?  What should the alpha be, currently I'm just using g
                    const uint32_t width = ft_bitmap->width / 3;
                    for (i = 0; i < ft_bitmap->rows; i++) {
                        for (j = 0; j < width; j++) {
                            image[i * width * 4 + (j * 4) + 0] = ft_bitmap->buffer[i * ft_bitmap->pitch + (j * 3) + 2];
                            image[i * width * 4 + (j * 4) + 1] = ft_bitmap->buffer[i * ft_bitmap->pitch + (j * 3) + 1];
                            image[i * width * 4 + (j * 4) + 2] = ft_bitmap->buffer[i * ft_bitmap->pitch + (j * 3) + 0];
                            image[i * width * 4 + (j * 4) + 3] = 0xff;
                        }
                    }
                }
                size_out[0] = ft_bitmap->width / 3;
                size_out[1] = ft_bitmap->rows;
                *image_out = image;
                return;
            } else {
                size_out[0] = 0;
                size_out[1] = 0;
                *image_out = NULL;
                return;
            }
        default:
            return;
        }
    } else {
        size_out[0] = 0;
        size_out[1] = 0;
        *image_out = NULL;
    }
}

GB_ERROR GB_GlyphMake(struct GB_Context* gb, uint32_t index, struct GB_Font *font, struct GB_Glyph **glyph_out)
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

        // NOTE: only do sub-pixel anti-aliasing if we are using RGBA textures.
        case GB_RENDER_LCD_RGB:
        case GB_RENDER_LCD_BGR:
            if (gb->texture_format == GB_TEXTURE_FORMAT_RGBA)
                load_flags |= FT_LOAD_TARGET_LCD;
            break;
        case GB_RENDER_LCD_RGB_V:
        case GB_RENDER_LCD_BGR_V:
            if (gb->texture_format == GB_TEXTURE_FORMAT_RGBA)
                load_flags |= FT_LOAD_TARGET_LCD_V;
            break;
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

        // NOTE: only do sub-pixel anti-aliasing if we are using RGBA textures.
        case GB_RENDER_LCD_RGB:
        case GB_RENDER_LCD_BGR:
            if (gb->texture_format == GB_TEXTURE_FORMAT_RGBA)
                render_mode = FT_RENDER_MODE_LCD;
            break;
        case GB_RENDER_LCD_RGB_V:
        case GB_RENDER_LCD_BGR_V:
            if (gb->texture_format == GB_TEXTURE_FORMAT_RGBA)
                render_mode = FT_RENDER_MODE_LCD_V;
            break;
        }

        // render glyph into ft_face->glyph->bitmap
        ft_error = FT_Render_Glyph(ft_face->glyph, render_mode);
        if (ft_error)
            return GB_ERROR_FTERR;

        // record post-hinted advance and bearing.
        uint32_t advance = FIXED_TO_INT(ft_face->glyph->metrics.horiAdvance);
        uint32_t bearing[2] = {FIXED_TO_INT(ft_face->glyph->metrics.horiBearingX),
                               FIXED_TO_INT(ft_face->glyph->metrics.horiBearingY)};

        uint8_t *image = NULL;
        FT_Bitmap *ft_bitmap = &ft_face->glyph->bitmap;
        uint32_t size[2];
        _InitGlyphImage(ft_bitmap, gb->texture_format, font->render_options, &image, size);
        uint32_t origin[2] = {0, 0};

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
        assert(glyph->rc > 0);
        glyph->rc++;
        return GB_ERROR_NONE;
    } else {
        return GB_ERROR_INVAL;
    }
}

static void _GB_GlyphDestroy(struct GB_Glyph *glyph)
{
    assert(glyph);
    if (glyph->image)
        free(glyph->image);
    free(glyph);
}

GB_ERROR GB_GlyphRelease(struct GB_Glyph *glyph)
{
    if (glyph) {
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
