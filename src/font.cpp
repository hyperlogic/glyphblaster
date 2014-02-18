#include <harfbuzz/hb.h>
#include <harfbuzz/hb-ft.h>
#include "font.h"
#include "context.h"
#include "glyph.h"
#include "cache.h"
#include "font.h"

// 26.6 fixed to int (truncates)
#define FIXED_TO_INT(n) (uint32_t)(n >> 6)

namespace gb {

Font::Font(const std::string filename, uint32_t pointSize,
           FontRenderOption renderOption, FontHintOption hintOption) :
    m_ftFace(nullptr),
    m_hbFont(nullptr),
    m_renderOption(renderOption),
    m_hintOption(hintOption)
{
    Context& context = Context::Get();

    FT_New_Face(context.GetFTLibrary(), filename.c_str(), 0, &m_ftFace);
    if (!m_ftFace)
    {
        fprintf(stderr, "Error loading font \"%s\"\n", filename.c_str());
        abort();
    }

    // set point size
    FT_Set_Char_Size(m_ftFace, (int)(pointSize * 64), 0, 72, 72);

    // create harfbuzz font
    m_hbFont = hb_ft_font_create(m_ftFace, 0);
    hb_ft_font_set_funcs(m_hbFont);

    // notify context
    context.OnFontCreate(this);
}

Font::~Font()
{
    Context& context = Context::Get();

    // notify context
    context.OnFontDestroy(this);

    if (m_ftFace)
        FT_Done_Face(m_ftFace);

    if (m_hbFont)
        hb_font_destroy(m_hbFont);
}

int Font::GetMaxAdvance() const
{
    return FIXED_TO_INT(m_ftFace->size->metrics.max_advance);
}

int Font::GetLineHeight() const
{
    return FIXED_TO_INT(m_ftFace->size->metrics.height);
}

} // namespace gb
