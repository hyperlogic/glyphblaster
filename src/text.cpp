#include <assert.h>
#include <unicode/ubidi.h>
#include <unicode/ustring.h>
#include "uthash.h"
#include "gb_context.h"
#include "gb_font.h"
#include "gb_glyph.h"
#include "gb_cache.h"
#include "gb_text.h"

// 26.6 fixed to int (truncates)
#define FIXED_TO_INT(n) (uint32_t)(n >> 6)

// cp is a utf32 codepoint
static bool IsSpace(uint32_t codePoint)
{
    switch (codePoint)
    {
    case 0x0020: // space
    case 0x1680: // ogham space mark
    case 0x180e: // mongolian vowel separator
    case 0x2000: // en quad
    case 0x2001: // em quad
    case 0x2002: // en space
    case 0x2003: // em space
    case 0x2004: // three-per-em space
    case 0x2005: // four-per-em space
    case 0x2006: // six-per-em space
    case 0x2008: // punctuation space
    case 0x2009: // thin space
    case 0x200a: // hair space
    case 0x200b: // zero width space
    case 0x200c: // zero width non joiner
    case 0x200d: // zero width joiner
    case 0x205f: // medium mathematical space
    case 0x3000: // ideograpfic space
        return true;
    default:
        return false;
    }
}

// cp is a utf32 codepoint
static bool IsNewline(uint32_t codePoint)
{
    switch (codePoint)
    {
    case 0x000a: // new line
    case 0x000b: // vertical tab
    case 0x000c: // form feed
    case 0x000d: // carriage return
    case 0x0085: // NEL next line
    case 0x2028: // line separator
    case 0x2029: // paragraph separator
        return true;
    default:
        return false;
    }
}

// returns the number of bytes to advance
// fills cp_out with the code point at p.
static int NextCodePoint(const uint8_t *p, uint32_t *codePointOut)
{
    if ((*p & 0x80) == 0)
    {
        *codePointOut = *p;
        return 1;
    }
    else if ((*p & 0xe0) == 0xc0) // 110xxxxx 10xxxxxx
    {
        *codePointOut = ((*p & ~0xe0) << 6) | (*(p+1) & ~0xc0);
        return 2;
    }
    else if ((*p & 0xf0) == 0xe0) // 1110xxxx 10xxxxxx 10xxxxxx
    {
        *codePointOut = ((*p & ~0xf0) << 12) | ((*(p+1) & ~0xc0) << 6) | (*(p+2) & ~0xc0);
        return 3;
    }
    else if ((*p & 0xf8) == 0xf0) // 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
    {
        *codePointOut = ((*p & ~0xf8) << 18) | ((*(p+1) & ~0xc0) << 12) | ((*(p+1) & ~0xc0) << 6) | (*(p+2) & ~0xc0);
        return 4;
    }
    else
    {
        // p is not at a valid starting point. p is not utf8 encoded or is at a bad offset.
        assert(0);
        *codePointOut = 0;
        return 1;
    }
}

void Text::UpdateCache()
{
    // hold a temp array of glyph ptrs, this is so we can sort glyphs by height before
    // adding them to the GlyphCache, which improves texture utilization for long strings of glyphs.
    int numGlyphs = hb_buffer_get_length(m_hbBuffer);
    Context& context = Context::Get();

    assert(m_hbBuffer);

    // prepare to iterate over all the glyphs in the hbBuffer
    hb_glyph_info_t *glyphs = hb_buffer_get_glyph_infos(m_hbBuffer, NULL);

    // build a vector of GlyphKeys
    std::vector<GlyphKey> keyVec;
    keyVec.reserve(numGlyphs);
    for (int i = 0; i < numGlyphs; i++)
    {
        // skip new-lines
        uint32_t cp;
        NextCodePoint(m_string.c_str() + glyphs[i].cluster, &cp);
        if (!IsNewline(cp))
        {
            keyVec.push_back(GlyphKey(glyphs[i].codepoint, m_font->GetIndex()));
        }
    }

    // add glyphs to cache and context, doing rasterization and sub-loads if necessary.
    context.RasterizeAndSubloadGlyphs(keyVec, m_glyphVec);
}

enum GlyphType { NEWLINE_GLYPH = 0, SPACE_GLYPH, NORMAL_GLYPH };

struct GlyphInfo
{
    enum GlyphType type;
    hb_glyph_info_t *hbGlyph;
    struct GB_Glyph *gbGlyph;
    int32_t x;
};

static uint32_t loop_begin_rtl(uint32_t num_glyphs) { return num_glyphs - 1; }
static uint32_t loop_begin_ltr(uint32_t num_glyphs) { return 0; }

static uint32_t loop_end_rtl(uint32_t num_glyphs) { return -1; }
static uint32_t loop_end_ltr(uint32_t num_glyphs) { return num_glyphs; }

static uint32_t loop_next_rtl(uint32_t i) { return i - 1; }
static uint32_t loop_next_ltr(uint32_t i) { return i + 1; }

// TODO: fix inf. loop if fit always returns false.
static int loop_fit_ltr(int32_t pen_x, uint32_t advance, int32_t kern, uint32_t size) { return (pen_x + advance + kern) <= size; }
static int loop_fit_rtl(int32_t pen_x, uint32_t advance, int32_t kern, uint32_t size) { return (-pen_x + advance + kern) <= size; }

static int32_t loop_advance_ltr(int32_t pen_x, uint32_t advance, int32_t kern) { return pen_x + advance + kern; }
static int32_t loop_advance_rtl(int32_t pen_x, uint32_t advance, int32_t kern) { return pen_x - advance - kern; }
static int32_t loop_advance_none(int32_t pen_x, uint32_t advance, int32_t kern) { return pen_x; }

typedef uint32_t (*iter_func_t)(uint32_t i);
typedef int (*fit_func_t)(int32_t pen_x, uint32_t advance, int32_t kern, uint32_t size);
typedef int32_t (*advance_func_t)(int32_t pen_x, uint32_t advance, int32_t kern);

// TODO: do word wrapping without shaping option
// TODO: split this into several methods
// TODO: vertical justification
// TODO: more c++ like impl, use traits or interfaces instead of function pointers.
void Text::WordWrapAndGenerateQuads()
{
    uint32_t num_glyphs = hb_buffer_get_length(m_hbBuffer);
    hb_glyph_info_t *glyphs = hb_buffer_get_glyph_infos(m_hbBuffer, NULL);
    hb_direction_t dir = hb_buffer_get_direction(text->hb_buffer);

    // create a queue to hold word-wrapped glyphs
    std::deque<GlyphInfo> q;

    fit_func_t fit;
    advance_func_t pre_advance, post_advance;
    iter_func_t begin, end, next, prev;
    if (dir == HB_DIRECTION_RTL)
    {
        fit = loop_fit_rtl;
        pre_advance = loop_advance_rtl;
        post_advance = loop_advance_none;
        begin = loop_begin_rtl;
        end = loop_end_rtl;
        next = loop_next_rtl;
        prev = loop_next_ltr;
    }
    else
    {
        fit = loop_fit_ltr;
        pre_advance = loop_advance_none;
        post_advance = loop_advance_ltr;
        begin = loop_begin_ltr;
        end = loop_end_ltr;
        next = loop_next_ltr;
        prev = loop_next_rtl;
    }

    int32_t i = 0;
    int32_t pen_x = 0;
    uint32_t cp;
    int32_t inside_word = 0;
    uint32_t word_start_i = 0, word_end_i = 0;
    int32_t word_start_x = 0, word_end_x = 0;
    for (i = begin(num_glyphs); i != end(num_glyphs); i = next(i))
    {
        // NOTE: cluster is an offset to the first byte in the utf8 encoded string which represents this glyph.
        NextCodePoint(text->utf8_string + glyphs[i].cluster, &cp);

        // lookup kerning
        int32_t kern = 0;
        if (next(i) != end(num_glyphs))
        {
            FT_Vector delta;
            FT_Get_Kerning(text->font->ft_face, glyphs[i].codepoint, glyphs[next(i)].codepoint,
                           FT_KERNING_DEFAULT, &delta);
            kern = FIXED_TO_INT(delta.x);
        }

        if (IsNewline(cp))
        {
            q.push_back(GlyphInfo{NEWLINE_GLYPH, nullptr, nullptr, pen_x});
            pen_x = 0;
            inside_word = 0;
        }
        else
        {
            auto glyph = context->FindInMap(GlyphKey(glyphs[i].codepoint, m_font->GetIndex()));
            assert(glyph);

            if (inside_word)
            {
                // does glyph fit on this line?
                if (fit(pen_x, glyph->advance, kern, text->size[0]))
                {
                    pen_x = pre_advance(pen_x, glyph->advance, kern);
                    if (IsSpace(cp))
                    {
                        q.push_back(GlyphInfo{SPACE_GLYPH, glyphs + i, glyph, pen_x});
                        // exiting word
                        word_end_i = i;
                        word_end_x = pen_x;
                        inside_word = 0;
                    }
                    else
                    {
                        q.push_back(GlyphInfo{NORMAL_GLYPH, glyphs + i, glyph, pen_x});
                    }
                    pen_x = post_advance(pen_x, glyph->advance, kern);
                }
                else
                {
                    if (IsSpace(cp))
                    {
                        // skip spaces
                        while (IsSpace(cp))
                        {
                            i = next(i);
                            NextCodePoint(text->utf8_string + glyphs[i].cluster, &cp);
                        }
                        prev(i);
                    }
                    else
                    {
                        // if the current word starts at the beginning of the line.
                        if (word_start_x == 0)
                        {
                            // we will have to split the word in the middle.
                            i = prev(i);
                        }
                        else
                        {
                            // backtrack to one before word_start_i, and restore pen_x
                            while (i != prev(word_start_i))
                            {
                                if (dir == HB_DIRECTION_LTR)
                                    pen_x = q.back().x;
                                q.pop_back();
                                if (dir == HB_DIRECTION_RTL)
                                    pen_x = q.back().x;
                                i = prev(i);
                            }
                        }
                    }
                    q.push_back(GlyphInfo{NEWLINE_GLYPH, nullptr, nullptr, pen_x});
                    pen_x = 0;
                    inside_word = 0;
                }
            }
            else  // !inside_word
            {
                // does glyph fit on this line?
                if (fit(pen_x, glyph->advance, kern, text->size[0]))
                {
                    pen_x = pre_advance(pen_x, glyph->advance, kern);
                    if (IsSpace(cp))
                    {
                        q.push_back(GlyphInfo{SPACE_GLYPH, glyphs + i, glyph, pen_x});
                    }
                    else
                    {
                        q.push_back(GlyphInfo{NORMAL_GLYPH, glyphs + i, glyph, pen_x});
                        // entering word
                        word_start_i = i;
                        word_start_x = pen_x;
                        inside_word = 1;
                    }
                    pen_x = post_advance(pen_x, glyph->advance, kern);
                }
                else
                {
                    // skip spaces
                    while (IsSpace(cp))
                    {
                        i = next(i);
                        NextCodePoint(text->utf8_string + glyphs[i].cluster, &cp);
                    }
                    // backup one char, so the next iteration thru the loop will be a non-space character
                    i = prev(i);
                    q.push_back(GlyphInfo{NEWLINE_GLYPH, NULL, NULL, word_end_x});
                    pen_x = 0;
                    inside_word = 0;
                }
            }
        }
    }

    // end with a new line, (makes justification easier)
    q.push_back(GlyphInfo{NEWLINE_GLYPH, nullptr, nullptr, pen_x});

    // allocate quads
    m_quadVec.clear();
    m_quadVec.reserve(q.size());

    const float texture_size = (float)gb->cache->texture_size;
    int32_t line_height = FIXED_TO_INT(text->font->ft_face->size->metrics.height);
    int32_t y = text->origin[1] + line_height;

    // horizontal justification
    uint32_t j;
    uint32_t line_start = 0;
    for (auto &info : q)
    {
        if (info.type == NEWLINE_GLYPH)
        {
            int32_t left_edge = (dir == HB_DIRECTION_RTL) ? info.x : 0;
            int32_t right_edge = (dir == HB_DIRECTION_RTL) ? 0 : info.x;
            int32_t offset = 0;
            switch (m_horizontalAlign)
            {
            case TextHorizontalAlign_Left:
                offset = -left_edge;
                break;
            case TextHorizontalAlign_Right:
                offset = text->size[0] - right_edge;
                break;
            case TextHorizontalAlign_Center:
                offset = (text->size[0] - left_edge - right_edge) / 2;
                break;
            }
            // apply offset to each glyphinfo.x
            for (j = line_start; j < i; j++)
            {
                info.x += offset;
            }
            line_start = i + 1;
        }
    }

    // initialize quads
    for (auto &info : q)
    {
        if (info.type == NEWLINE_GLYPH)
        {
            y += line_height;
        }
        else
        {
            // NOTE: y axis points down, quad origin is upper-left corner of glyph
            // build quad
            struct GB_Glyph *gbGlyph = info.gbGlyph;
            struct GB_GlyphQuad *quad = text->glyph_quads + text->num_glyph_quads;

            IntPoint glyphBearing = info.gbGlyph.GetBearing();
            IntPoint glyphOrigin = info.gbGlyph.GetOrigin();
            IntPoint glyphSize = info.gbGlyph.GetSize();

            IntPoint pen = {m_origin.x + info.x, y};
            IntPoint origin = {m_origin.x + info.x + glyphBearing.x, y - glyphBearing.y};
            IntPoint size = info.GetSize();
            FloatPoint uvOrigin = {glyphOrigin.x / texture_size, glyphOrigin.y / texture_size};
            FloatPoint uvSize = {glyphSize.x / texture_size, glyphSize.y / texture_size};
            uint32_t glTexObj = info.gbGlyph->gl_tex_obj ? info.gbGlyph->gl_tex_obj : context->GetFallbackTexture().GetGLTexObj();

            m_quadVec.push_back(Quad{pen, origin, size, uvOrigin, uvSize, m_userData, glTexObj});
        }
    }
}

static void ft_shape(hb_font_t *hb_font, hb_buffer_t *hb_buffer, FT_Face ft_face, uint8_t* utf8_string)
{
    assert(hb_font && hb_buffer && ft_face);
    int num_glyphs = hb_buffer_get_length(hb_buffer);
    hb_glyph_info_t *glyphs = hb_buffer_get_glyph_infos(hb_buffer, NULL);

    // iterate over each glyph
    int i;
    for (i = 0; i < num_glyphs; i++) {
        uint32_t cp;
        NextCodePoint(utf8_string + glyphs[i].cluster, &cp);
        glyphs[i].codepoint = FT_Get_Char_Index(ft_face, cp);
    }
}

Text::Text(const std::string& string, std::shared_ptr<Font> font,
           void* userData, IntPoint origin, IntPoint size,
           TextHorizontalAlign horizontalAlign, TextVerticalAlign verticalAlign,
           uint32_t optionFlags) :
    m_string(string),
    m_hbBuffer(hb_buffer_create()),
    m_font(font),
    m_origin(origin),
    m_size(size),
    m_horizontalAlign(horizontalAlign),
    m_verticalAlign(verticalAlign),
    m_optionFlags(optionFlags)
{
    hb_buffer_add_utf8(m_hbBuffer, m_string.c_str(), m_string.size(), 0, m_string.size());

    if (!(m_optionFlags & TextOptionFlags_DisableShaping))
    {
        // Use harf-buzz to perform glyph shaping
        hb_shape(m_font->GetHarfbuzzFont(), text->hb_buffer, NULL, 0);

        /*
        // debug print detected direction & script
        hb_direction_t dir = hb_buffer_get_direction(text->hb_buffer);
        hb_script_t script = hb_buffer_get_script(text->hb_buffer);
        hb_tag_t tag = hb_script_to_iso15924_tag(script);
        printf("AJT: direction = %s\n", hb_direction_to_string(dir));
        char tag_str[5];
        tag_str[0] = tag >> 24;
        tag_str[1] = tag >> 16;
        tag_str[2] = tag >> 8;
        tag_str[3] = tag;
        tag_str[4] = 0;
        printf("AJT: script = %s\n", tag_str);
        */

    } else {
        // TODO: need a compile time option to remove dependency on harf-buzz
        // just use FT_Get_Char_Index to look up glyph index
        ft_shape(text->font->hb_font, text->hb_buffer, text->font->ft_face, text->utf8_string);
    }

    UpdateCache();
    WordWrapAndGenerateQuads();
}

Text::~Text()
{
    if (m_userData)
        free(text->userData);

    hb_buffer_destroy(m_hbBuffer);
}

// Renders given text using renderer func.
Text::Draw()
{
    Context& context = Context::Get();
    context.m_renderFunc(m_quadVec);
}
