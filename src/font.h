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
    // renderOption - controls how anti-aliasing is preformed during glyph rendering.
    // hintOption - controls which hinting algorithm is chosen during glyph rendering.
    Font(const std::string filename, uint32_t pointSize,
         FontRenderOption renderOption, FontHintOption hintOption);
    ~Font();

    FontRenderOption GetRenderOption() const { return m_renderOption; }
    FontHintOption GetHintOption() const { return m_hintOption; }
    int GetMaxAdvance() const;
    int GetLineHeight() const;

protected:
    uint32_t GetIndex() const { return m_index; }
    FT_Face GetFTFace() const { return m_ftFace; }
    hb_font_t* GetHarfbuzzFont() const { return m_hbFont; }

    uint32_t m_index;
    FT_Face m_ftFace;
    hb_font_t* m_hbFont;
    FontRenderOption m_renderOption;
    FontHintOption m_hintOption;
};

} // namespace gb

#endif // GB_FONT_H
