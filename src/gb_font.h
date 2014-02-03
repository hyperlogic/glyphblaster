#ifndef GB_FONT_H
#define GB_FONT_H

#include <stdint.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <harfbuzz/hb.h>
#include "gb_error.h"

namespace gb {

class Context;

enum FontRenderOption {
    FontRenderOption_Normal = 0,  // normal anti-aliased font rendering
    FontRenderOption_Light,  // lighter anti-aliased outline hinting, this will force auto hinting.
    FontRenderOption_Mono,  // no anti-aliasing
    FontRenderOption_LCD_RGB,  // subpixel anti-aliasing, designed for LCD RGB displays
    FontRenderOption_LCD_BGR,  // subpixel anti-aliasing, designed for LCD BGR displays
    FontRenderOption_LCD_RGB_V,  // vertical subpixel anti-aliasing, designed for LCD RGB displays
    FontRenderOption_LCD_BGR_V   // vertical subpixel anti-aliasing, designed for LCD BGR displays
};

enum FontHintOption {
    FontHintOption_Default = 0,  // default hinting algorithm is chosen.
    FontHintOption_ForceAuto,  // always use the FreeType2 auto hinting algorithm
    FontHintOption_NoAuto,  // always use the fonts hinting algorithm
    FontHintOption_None  // use no hinting algorithm at all.
};

class Font
{
public:
    // filename - ttf or otf font
    // pointSize - pixels per em
    // renderOption - controls how anti-aliasing is preformed during glyph rendering.
    // hintOption - controls which hinting algorithm is chosen during glyph rendering.
    Font(Context& context, const std::string filename, uint32_t pointSize,
         FontRenderOption renderOption, FontHintOption hintOption);
    ~Font();

    FontRenderOption GetRenderOption() const { return m_renderOption; }
    FontHintOption GetHintOption() const { return m_hintOption; }
    uint32_t GetMaxAdvance() const;
    uint32_t GetLineHeight() const;

protected:
    FT_Face GetFTFace() const { return m_ftFace; }

    uint32_t m_index;
    FT_Face m_ftFace;
    hb_font_t* hbFont;
    FontRenderOption m_renderOption;
    FontHintOption m_hintOption;
};

} // namespace gb

#endif // GB_FONT_H
