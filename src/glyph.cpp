#include <stdio.h>
#include <assert.h>
#include "glyph.h"
#include "font.h"
#include "context.h"

// 26.6 fixed to int (truncates)
#define FIXED_TO_INT(n) (uint32_t)(n >> 6)

namespace gb {

Glyph::Glyph(uint32_t index, const Font& font) :
    m_key(index, font.GetIndex()),
    m_texObj(0),
    m_origin{0, 0}
{
    Context& context = Context::Get();
    assert(font.GetFTFace());
    FT_Face ftFace = font.GetFTFace();

    uint32_t ftLoadFlags;

    switch (font.GetHintOption())
    {
    default:
    case FontHintOption_Default:
        ftLoadFlags = FT_LOAD_DEFAULT;
        break;
    case FontHintOption_ForceAuto:
        ftLoadFlags = FT_LOAD_FORCE_AUTOHINT;
        break;
    case FontHintOption_NoAuto:
        ftLoadFlags = FT_LOAD_NO_AUTOHINT;
        break;
    case FontHintOption_None:
        ftLoadFlags = FT_LOAD_NO_HINTING;
        break;
    }

    switch (font.GetRenderOption())
    {
    default:
    case FontRenderOption_Normal:
        ftLoadFlags |= FT_LOAD_TARGET_NORMAL;
        break;
    case FontRenderOption_Light:
        ftLoadFlags |= FT_LOAD_TARGET_LIGHT;
        break;
    case FontRenderOption_Mono:
        ftLoadFlags |= FT_LOAD_TARGET_MONO;
        break;
    case FontRenderOption_LCD_RGB:
    case FontRenderOption_LCD_BGR:
        // Only do sub-pixel anti-aliasing if we are using RGBA textures.
        if (context.GetTextureFormat() == TextureFormat_RGBA)
            ftLoadFlags |= FT_LOAD_TARGET_LCD;
        break;
    case FontRenderOption_LCD_RGB_V:
    case FontRenderOption_LCD_BGR_V:
        // Only do sub-pixel anti-aliasing if we are using RGBA textures.
        if (context.GetTextureFormat() == TextureFormat_RGBA)
            ftLoadFlags |= FT_LOAD_TARGET_LCD_V;
        break;
    }

    FT_Error ftError = FT_Load_Glyph(font.GetFTFace(), index, ftLoadFlags);
    if (ftError)
        abort();

    FT_Render_Mode ftRenderMode;
    switch (font.GetRenderOption())
    {
    default:
    case FontRenderOption_Normal:
        ftRenderMode = FT_RENDER_MODE_NORMAL;
        break;
    case FontRenderOption_Light:
        ftRenderMode = FT_RENDER_MODE_LIGHT;
        break;
    case FontRenderOption_Mono:
        ftRenderMode = FT_RENDER_MODE_MONO;
        break;
    case FontRenderOption_LCD_RGB:
    case FontRenderOption_LCD_BGR:
        // Only do sub-pixel anti-aliasing if we are using RGBA textures.
        if (context.GetTextureFormat() == TextureFormat_RGBA)
            ftRenderMode = FT_RENDER_MODE_LCD;
        else
            ftRenderMode = FT_RENDER_MODE_NORMAL;
        break;
    case FontRenderOption_LCD_RGB_V:
    case FontRenderOption_LCD_BGR_V:
        // Only do sub-pixel anti-aliasing if we are using RGBA textures.
        if (context.GetTextureFormat() == TextureFormat_RGBA)
            ftRenderMode = FT_RENDER_MODE_LCD_V;
        else
            ftRenderMode = FT_RENDER_MODE_NORMAL;
        break;
    }

    // render glyph into ftFace->glyph->bitmap
    ftError = FT_Render_Glyph(ftFace->glyph, ftRenderMode);
    if (ftError)
        abort();

    // record post-hinting advance and bearing.
    m_advance = FIXED_TO_INT(ftFace->glyph->metrics.horiAdvance);
    m_bearing = {(int)FIXED_TO_INT(ftFace->glyph->metrics.horiBearingX),
                 (int)FIXED_TO_INT(ftFace->glyph->metrics.horiBearingY)};

    FT_Bitmap* ftBitmap = &ftFace->glyph->bitmap;
    InitImageAndSize(ftBitmap, context.GetTextureFormat(), font.GetRenderOption(), font.GetPaddingBorder());
}

Glyph::~Glyph()
{

}

void Glyph::InitImageAndSize(FT_Bitmap* ftBitmap, TextureFormat textureFormat,
                             FontRenderOption renderOption, uint32_t paddingBorder)
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

        switch (renderOption)
        {
        case FontRenderOption_Normal:
        case FontRenderOption_Light:
        {
            const int width = (ftBitmap->width + 2 * paddingBorder);
            const int height = (ftBitmap->rows + 2 * paddingBorder);
            const int numPixels =  width * height;

            if (textureFormat == TextureFormat_Alpha)
            {
                // allocate an image to hold a copy of the rasterized glyph
                std::unique_ptr<uint8_t> image(new uint8_t[numPixels]);
                m_image = std::move(image);

                // clear dest image
                memset(m_image.get(), 0, numPixels);

                // copy image from ft_bitmap.buffer into image, row by row.
                // The pitch of each row in the ft_bitmap maybe >= width,
                // so we can't do this as a single memcpy.
                uint8_t* img = m_image.get();
                for (int i = 0; i < ftBitmap->rows; i++)
                {
                    memcpy(img + ((i + paddingBorder) * width) + paddingBorder, ftBitmap->buffer + (i * ftBitmap->pitch), ftBitmap->width);
                }
            }
            else if (textureFormat == TextureFormat_RGBA)
            {
                // allocate an image to hold a copy of the rasterized glyph
                std::unique_ptr<uint8_t> image(new uint8_t[4 * numPixels]);
                m_image = std::move(image);

                // clear dest image
                memset(m_image.get(), 0, 4 * numPixels);

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
                        const uint8_t intensity = ftBitmap->buffer[i * ftBitmap->pitch + j];
                        img[(i + paddingBorder) * width * 4 + ((j + paddingBorder) * 4) + 0] = 0xff;
                        img[(i + paddingBorder) * width * 4 + ((j + paddingBorder) * 4) + 1] = 0xff;
                        img[(i + paddingBorder) * width * 4 + ((j + paddingBorder) * 4) + 2] = 0xff;
                        img[(i + paddingBorder) * width * 4 + ((j + paddingBorder) * 4) + 3] = intensity;
                    }
                }
            }
            else
            {
                assert(0);
            }
            m_size = {width, height};
            break;
        }

        case FontRenderOption_Mono:
        {
            const int width = (ftBitmap->width + 2 * paddingBorder);
            const int height = (ftBitmap->rows + 2 * paddingBorder);
            const int numPixels =  width * height;

            // ftBitmap is 1 bit per pixel.
            if (textureFormat == TextureFormat_Alpha)
            {
                // allocate an image to hold a copy of the rasterized glyph
                std::unique_ptr<uint8_t> image(new uint8_t[numPixels]);
                m_image = std::move(image);

                // clear dest image
                memset(m_image.get(), 0, numPixels);

                // copy image from ft_bitmap.buffer into image
                uint8_t byte, mask;
                uint8_t* img = m_image.get();
                for (int i = 0; i < ftBitmap->rows; i++)
                {
                    for (int j = 0; j < ftBitmap->width; j++)
                    {
                        byte = ftBitmap->buffer[i * ftBitmap->pitch + (j/8)];
                        mask = 0x1 << (7 - (j % 8));
                        img[(i + paddingBorder) * width + j + paddingBorder] = (byte & mask) ? 0xff : 0x00;
                    }
                }
            }
            else if (textureFormat == TextureFormat_RGBA)
            {
                // allocate an image to hold a copy of the rasterized glyph
                std::unique_ptr<uint8_t> image(new uint8_t[4 * numPixels]);
                m_image = std::move(image);

                // clear dest image
                memset(m_image.get(), 0, 4 * numPixels);

                // copy image from ft_bitmap.buffer into image
                uint8_t byte, mask;
                uint8_t* img = m_image.get();
                for (int i = 0; i < ftBitmap->rows; i++)
                {
                    for (int j = 0; j < ftBitmap->width; j++)
                    {
                        byte = ftBitmap->buffer[i * ftBitmap->pitch + (j/8)];
                        mask = 0x1 << (7 - (j % 8));
                        img[(i + paddingBorder) * width * 4 + (j + paddingBorder) * 4 + 0] = 0xff;
                        img[(i + paddingBorder) * width * 4 + (j + paddingBorder) * 4 + 1] = 0xff;
                        img[(i + paddingBorder) * width * 4 + (j + paddingBorder) * 4 + 2] = 0xff;
                        img[(i + paddingBorder) * width * 4 + (j + paddingBorder) * 4 + 3] = (byte & mask) ? 0xff : 0x00;
                    }
                }
            }
            else
            {
                assert(0);
            }
            m_size = {width, height};
            break;
        }

        case FontRenderOption_LCD_RGB:
        case FontRenderOption_LCD_BGR:
        case FontRenderOption_LCD_RGB_V:
        case FontRenderOption_LCD_BGR_V:
        {

            const int width = (ftBitmap->width / 3 + 2 * paddingBorder);
            const int height = (ftBitmap->rows + 2 * paddingBorder);
            const int numPixels =  width * height;

            assert(textureFormat == TextureFormat_RGBA);
            if (textureFormat == TextureFormat_RGBA)
            {
                // allocate an image to hold a copy of the rasterized glyph
                std::unique_ptr<uint8_t> image(new uint8_t[4 * numPixels]);
                m_image = std::move(image);

                // clear dest image
                memset(m_image.get(), 0, 4 * numPixels);

                if (renderOption == FontRenderOption_LCD_RGB || renderOption == FontRenderOption_LCD_RGB_V)
                {
                    // copy image from ftBitmap.buffer into image, row by row.
                    // The pitch of each row in the ft_bitmap maybe >= width,
                    // convert from RGB to RGBA
                    // TODO: Is this pre mulitplied alpha?  What should the alpha be, currently I'm just using 255.
                    uint8_t* img = m_image.get();
                    for (int i = 0; i < ftBitmap->rows; i++)
                    {
                        for (int j = 0; j < (ftBitmap->width / 3); j++)
                        {
                            img[(i + paddingBorder) * width * 4 + ((j + paddingBorder) * 4) + 0] = ftBitmap->buffer[i * ftBitmap->pitch + (j * 3) + 0];
                            img[(i + paddingBorder) * width * 4 + ((j + paddingBorder) * 4) + 1] = ftBitmap->buffer[i * ftBitmap->pitch + (j * 3) + 1];
                            img[(i + paddingBorder) * width * 4 + ((j + paddingBorder) * 4) + 2] = ftBitmap->buffer[i * ftBitmap->pitch + (j * 3) + 2];
                            img[(i + paddingBorder) * width * 4 + ((j + paddingBorder) * 4) + 3] = 0xff;
                        }
                    }
                }
                else if (textureFormat == TextureFormat_RGBA)
                {
                    // copy image from ft_bitmap.buffer into image, row by row.
                    // The pitch of each row in the ft_bitmap maybe >= width,
                    // convert from RGB to RGBA
                    // TODO: Is this pre mulitplied alpha?  What should the alpha be, currently I'm just using 255
                    const uint32_t width = ftBitmap->width / 3;
                    uint8_t* img = m_image.get();
                    for (int i = 0; i < ftBitmap->rows; i++)
                    {
                        for (int j = 0; j < (ftBitmap->width / 3); j++)
                        {
                            img[(i + paddingBorder) * width * 4 + ((j + paddingBorder) * 4) + 0] = ftBitmap->buffer[i * ftBitmap->pitch + (j * 3) + 2];
                            img[(i + paddingBorder) * width * 4 + ((j + paddingBorder) * 4) + 1] = ftBitmap->buffer[i * ftBitmap->pitch + (j * 3) + 1];
                            img[(i + paddingBorder) * width * 4 + ((j + paddingBorder) * 4) + 2] = ftBitmap->buffer[i * ftBitmap->pitch + (j * 3) + 0];
                            img[(i + paddingBorder) * width * 4 + ((j + paddingBorder) * 4) + 3] = 0xff;
                        }
                    }
                }
                else
                {
                    assert(0);
                }
                m_size = {width, height};
            }
            else
            {
                m_size = {0, 0};
            }
            break;
        }
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
