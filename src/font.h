#ifndef GB_FONT_H
#define GB_FONT_H

#include <stdint.h>
#include <string>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <harfbuzz/hb.h>
#include "glyphblaster.h"

namespace gb {

class Context;

class Font
{
    friend class Context;
    friend class Glyph;
    friend class Text;
public:
    // filename - ttf or otf font
    // pointSize - pixels per em
    // paddingBorder - border around each glyph in pixels
    // renderOption - controls how anti-aliasing is preformed during glyph rendering.
    // hintOption - controls which hinting algorithm is chosen during glyph rendering.
    Font(const std::string filename, uint32_t pointSize, uint32_t paddingBorder,
         FontRenderOption renderOption, FontHintOption hintOption);
    ~Font();

    uint32_t GetPaddingBorder() const { return m_paddingBorder; }
    FontRenderOption GetRenderOption() const { return m_renderOption; }
    FontHintOption GetHintOption() const { return m_hintOption; }
    int GetMaxAdvance() const;
    int GetLineHeight() const;

protected:
    uint32_t GetIndex() const { return m_index; }
    FT_Face GetFTFace() const { return m_ftFace; }
#ifdef GB_USE_HARFBUZZ
    hb_font_t* GetHarfBuzzFont() const { return m_hbFont; }
#endif

    uint32_t m_index;
    FT_Face m_ftFace;
#ifdef GB_USE_HARFBUZZ
    hb_font_t* m_hbFont;
#endif
    uint32_t m_paddingBorder;
    FontRenderOption m_renderOption;
    FontHintOption m_hintOption;
};

} // namespace gb

#endif // GB_FONT_H
