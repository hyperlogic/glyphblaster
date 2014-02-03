#include <stdio.h>
#include <assert.h>
#include "gb_font.h"
#include "gb_glyph.h"

// 26.6 fixed to int (truncates)
#define FIXED_TO_INT(n) (uint32_t)(n >> 6)

namespace gb {

Glyph::Glyph(const Context& context, uint32_t index, const Font& font) :
    m_key(index, font.GetIndex()),
    m_glTexObj(0),
    m_origin{0, 0}
{
    assert(font.getFTFace());

    uint32_t ftLoadFlags;

    switch (font.getHintOption())
    {
    default:
    case Font::HintOption_Default:
        ftLoadFlags = FT_LOAD_DEFAULT;
        break;
    case Font::HintOption_ForceAuto:
        ftLoadFlags = FT_LOAD_FORCE_AUTOHINT;
        break;
    case Font::HintOption_NoAuto:
        ftLoadFlags = FT_LOAD_NO_AUTOHINT;
        break;
    case Font::HintOption_None:
        ftLoadFlags = FT_LOAD_NO_HINTING;
        break;
    }

    swtich (font.getRenderOption())
    {
    default:
    case Font::RenderOption_Normal:
        ftLoadFlags |= FT_LOAD_TARGET_NORMAL;
        break;
    case Font::RenderOption_Light:
        ftLoadFlags |= FT_LOAD_TARGET_LIGHT;
        break;
    case Font::RenderOption_Mono:
        ftLoadFlags |= FT_LOAD_TARGET_MONO;
        break;
    case Font::RenderOption_LCD_RGB:
    case Font::RenderOption_LCD_BGR:
        // Only do sub-pixel anti-aliasing if we are using RGBA textures.
        if (context->GetTextureFormat() == TextureFormat_RGBA)
            ftLoadFlags |= FT_LOAD_TARGET_LCD;
        break;
    case Font::RenderOption_LCD_RGB_V:
    case Font::RenderOption_LCD_BGR_V:
        // Only do sub-pixel anti-aliasing if we are using RGBA textures.
        if (context->GetTextureFormat() == TextureFormat_RGBA)
            ftLoadFlags |= FT_LOAD_TARGET_LCD_V;
        break;
    }

    FT_Error ftError = FT_Load_Glyph(font.getFTFace(), index, ftLoadFlags);
    if (ftError)
        abort();

    FT_Render_Mode ftRenderMode;
    switch (font.getRenderOption())
    {
    default:
    case Font::RenderOption_Normal:
        ftRenderMode = FT_RENDER_MODE_NORMAL;
        break;
    case Font::RenderOption_Light:
        ftRenderMode = FT_RENDER_MODE_LIGHT;
        break;
    case Font::RenderOption_Mono:
        ftRenderMode = FT_RENDER_MODE_MONO;
        break;
    case Font::RenderOption_LCD_RGB:
    case Font::RenderOption_LCD_BGR:
        // Only do sub-pixel anti-aliasing if we are using RGBA textures.
        if (context->GetTextureFormat() == TextureFormat_RGBA)
            ftRenderMode = FT_RENDER_MODE_LCD;
        break;
    case Font::RenderOption_LCD_RGB_V:
    case Font::RenderOption_LCD_BGR_V:
        // Only do sub-pixel anti-aliasing if we are using RGBA textures.
        if (context->GetTextureFormat() == TextureFormat_RGBA)
            ftRenderMode = FT_RENDER_MODE_LCD_V;
        break;
    }

    // render glyph into ftFace->glyph->bitmap
    ftError = FT_Render_Glyph(ft_face->glyph, render_mode);
    if (ftError)
        abort();

    // record post-hinting advance and bearing.
    m_advance = FIXED_TO_INT(ft_face->glyph->metrics.horiAdvance);
    m_bearing = {FIXED_TO_INT(ft_face->glyph->metrics.horiBearingX),
                 FIXED_TO_INT(ft_face->glyph->metrics.horiBearingY)};

    FT_Bitmap* ftBitmap = &ft_face->glyph->bitmap;
    InitImageAndSize(ftBitmap, context.GetTextureFormat(), font.GetRenderOption());
}

Glyph::~Glyph()
{

}

GlyphKey Glyph::GetKey() const
{
    return m_key;
}

void Glyph::InitImageAndSize(FT_Bitmap* ftBitmap, TextureFormat textureFormat, Font::RenderOption renderOption)
{
    if (ftBitmap->width > 0 && ftBitmap->rows > 0)
    {
        // Most of these glyph textures should be rendered using non-premultiplied alpha
        // i.e. glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        //
        // The exception is the lcd sub-pixel modes
        // These require 3 passes, one for each color channel. (Use glColorMask)
        // Also, a special shader must be used that copies a single color component into alpha.
        // For example: gl_FragColor.xyz = vec3(1, 1, 1); gl_FragColor.a = texture2D(glyph_texture, uv).r;
        // This can then be blended into the frame buffer using glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        swtich (renderOption)
        {
        case Font::RenderOption_Normal:
        case Font::RenderOption_Light:
            if (textureFormat == TextureFormat__ALPHA)
            {
                // allocate an image to hold a copy of the rasterized glyph
                m_image = new uint8_t[ftBitmap->width * ftBitmap->rows];

                // copy image from ft_bitmap.buffer into image, row by row.
                // The pitch of each row in the ft_bitmap maybe >= width,
                // so we can't do this as a single memcpy.
                uint8_t* img = m_image.get();
                for (int i = 0; i < ftBitmap->rows; i++) {
                    memcpy(img + (i * ftBitmap->width), ftBitmap->buffer + (i * ftBitmap->pitch), ftBitmap->width);
                }
            }
            else
            {
                // allocate an image to hold a copy of the rasterized glyph
                m_image = new uint8_t[4 * ftBitmap->width * ftBitmap->rows];
            }

            // copy image from ft_bitmap.buffer into image, row by row.
            // The pitch of each row in the ft_bitmap maybe >= width,
            // convert from ALPHA to RGBA
            // NOTE: This glyph texture should be rendered using non-premultiplied alpha
            // i.e. glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            uint8_t* img = m_image.get();
            for (int i = 0; i < ftBitmap->rows; i++)
            {
                for (int j = 0; j < ftBitmap->width; j++)
                {
                    // white with alpha, i.e. non-premultiplied alpha.
                    img[i * ftBitmap->width * 4 + (j * 4) + 0] = 0xff;
                    img[i * ftBitmap->width * 4 + (j * 4) + 1] = 0xff;
                    img[i * ftBitmap->width * 4 + (j * 4) + 2] = 0xff;
                    img[i * ftBitmap->width * 4 + (j * 4) + 3] = ftBitmap->buffer[i * ftBitmap->pitch + j];
                }
            }
            m_size = {ftBitmap->width, ftBitmap->rows};
            break;

        case Font::RenderOption_Mono:
            // ftBitmap is 1 bit per pixel.
            if (textureFormat == TextureFormat_Alpha)
            {
                // allocate an image to hold a copy of the rasterized glyph
                m_image = new uint8_t[ftBitmap->width * ftBitmap->rows];

                // copy image from ft_bitmap.buffer into image
                uint8_t byte, mask;
                uint8_t* img = m_image.get();
                for (int i = 0; i < ftBitmap->rows; i++)
                {
                    for (int j = 0; j < ftBitmap->width; j++)
                    {
                        byte = ftBitmap->buffer[i * ftBitmap->pitch + (j/8)];
                        mask = 0x1 << (7 - (j % 8));
                        img[i * ft_bitmap->width + j] = (byte & mask) ? 0xff : 0x00;
                    }
                }
            }
            else
            {
                // allocate an image to hold a copy of the rasterized glyph
                m_image = new uint8_t[4 * ftBitmap->width * ftBitmap->rows];

                // copy image from ft_bitmap.buffer into image
                uint8_t byte, mask;
                uint8_t* img = m_image.get();
                for (int i = 0; i < ftBitmap->rows; i++)
                {
                    for (int j = 0; j < ftBitmap->width; j++)
                    {
                        byte = ftBitmap->buffer[i * ftBitmap->pitch + (j/8)];
                        mask = 0x1 << (7 - (j % 8));
                        img[i * ftBitmap->width * 4 + j * 4 + 0] = 0xff;
                        img[i * ftBitmap->width * 4 + j * 4 + 1] = 0xff;
                        img[i * ftBitmap->width * 4 + j * 4 + 2] = 0xff;
                        img[i * ftBitmap->width * 4 + j * 4 + 3] = (byte & mask) ? 0xff : 0x00;
                    }
                }
            }
            m_size = {ftBitmap->width, ftBitmap->rows};
            break;

        case Font::RenderOption_LCD_RGB:
        case Font::RenderOption_LCD_BGR:
        case Font::RenderOption_LCD_RGB_V:
        case Font::RenderOption_LCD_BGR_V:
            assert(textureFormat == TextureFormat_RGBA);
            if (textureFormat == TextureFormat_RGBA)
            {
                // allocate an image to hold a copy of the rasterized glyph
                m_image = new uint8_t[4 * ftBitmap->width * ftBitmap->rows];

                if (renderOption == RenderOption_LCD_RGB || renderOption == RenderOption_LCD_RGB_V)
                {
                    // copy image from ftBitmap.buffer into image, row by row.
                    // The pitch of each row in the ft_bitmap maybe >= width,
                    // convert from RGB to RGBA
                    // TODO: Is this pre mulitplied alpha?  What should the alpha be, currently I'm just using 255.
                    const uint32_t width = ftBitmap->width / 3;
                    uint8_t* img = m_image.get();
                    for (int i = 0; i < ftBitmap->rows; i++) {
                        for (int j = 0; j < width; j++) {
                            img[i * width * 4 + (j * 4) + 0] = ftBitmap->buffer[i * ftBitmap->pitch + (j * 3) + 0];
                            img[i * width * 4 + (j * 4) + 1] = ftBitmap->buffer[i * ftBitmap->pitch + (j * 3) + 1];
                            img[i * width * 4 + (j * 4) + 2] = ftBitmap->buffer[i * ftBitmap->pitch + (j * 3) + 2];
                            img[i * width * 4 + (j * 4) + 3] = 0xff;
                        }
                    }
                }
                else
                {
                    // copy image from ft_bitmap.buffer into image, row by row.
                    // The pitch of each row in the ft_bitmap maybe >= width,
                    // convert from RGB to RGBA
                    // TODO: Is this pre mulitplied alpha?  What should the alpha be, currently I'm just using 255
                    const uint32_t width = ftBitmap->width / 3;
                    uint8_t* img = m_image.get();
                    for (i = 0; i < ftBitmap->rows; i++) {
                        for (j = 0; j < width; j++) {
                            img[i * width * 4 + (j * 4) + 0] = ftBitmap->buffer[i * ftBitmap->pitch + (j * 3) + 2];
                            img[i * width * 4 + (j * 4) + 1] = ftBitmap->buffer[i * ftBitmap->pitch + (j * 3) + 1];
                            img[i * width * 4 + (j * 4) + 2] = ftBitmap->buffer[i * ftBitmap->pitch + (j * 3) + 0];
                            img[i * width * 4 + (j * 4) + 3] = 0xff;
                        }
                    }
                }
                m_size = {ftBitmap->width / 3, ftBitmap->rows};
            }
            else
            {
                m_size = {0, 0};
            }
            break;
        default:
            m_size = {0, 0};
            break;
        }
    }
    else
    {
        m_size = {0, 0};
    }
}

} // namespace gb
