#include <harfbuzz/hb.h>
#include <harfbuzz/hb-ft.h>
#include "utlist.h"
#include "uthash.h"
#include "gb_context.h"
#include "gb_glyph.h"
#include "gb_cache.h"
#include "gb_font.h"

// 26.6 fixed to int (truncates)
#define FIXED_TO_INT(n) (uint32_t)(n >> 6)

namespace gb {

Font::Font(Context& context, const std::string filename, uint32_t pointSize,
           FontRenderOption renderOption, FontHintOption hintOption) :
    m_index(context.GetNextFontIndex()),
    m_ftFace(nullptr),
    m_hbFont(nullptr),
    m_renderOption(renderOption),
    m_hintOption(hintOption)
{
    FT_New_Face(context.GetFTLibrary(), filename.c_str(), 0, &m_ftFace);
    if (!m_ftFace)
    {
        fprintf(stderr, "Error loading font \"%s\"\n", filename);
        abort();
    }

    // set point size
    FT_Set_Char_Size(m_ftFace, (int)(pointSize * 64), 0, 72, 72);

    // create harfbuzz font
    m_hbFont = hb_ft_font_create(m_ftFace, 0);
    hb_ft_font_set_funcs(m_hbFont);
}

Font::~Font()
{
    if (m_ftFace)
        FT_Done_Face(m_ftFace);

    if (m_hbFont)
        hb_font_destroy(font->hb_font);
}

uint32_t Font::GetMaxAdvance() const
{
    return FIXED_TO_INT(m_ftFace->size->metrics.max_advance);
}

uint32_t Font::GetLineHeight() const
{
    return FIXED_TO_INT(m_ftFace->size->metrics.height);
}
