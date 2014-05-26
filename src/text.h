#ifndef GB_TEXT_H
#define GB_TEXT_H

#include <stdint.h>
#include <string>
#include <vector>
#include "glyphblaster.h"
#include "context.h"

namespace gb {

class Font;

enum TextHorizontalAlign {
    TextHorizontalAlign_Left = 0,
    TextHorizontalAlign_Right,
    TextHorizontalAlign_Center
};

enum TextVerticalAlign {
    TextVerticalAlign_Top = 0,
    TextVerticalAlign_Bottom,
    TextVerticalAlign_Center
};

enum TextOptionFlags {
    TextOptionFlags_None = 0,
    TextOptionFlags_DisableShaping = 0x01,
    TextOptionFlags_DirectionRightToLeft = 0x02
};

class Text
{
public:
    // string is assumed to be utf8 encoded.
    // script tag is 4 char code from iso 15952, http://unicode.org/iso15924/
    Text(const std::string& string, std::shared_ptr<Font> font,
         void* userData, IntPoint origin, IntPoint size,
         TextHorizontalAlign horizontalAlign, TextVerticalAlign verticalAlign,
         uint32_t optionFlags = TextOptionFlags_None, const char* script = nullptr);
    ~Text();
    void Draw();
    const QuadVec& GetQuadVec() const { return m_quadVec; }

protected:
    struct GlyphCursor
    {
        uint32_t index;
        uint32_t cluster;
        uint32_t cp;
    };
    typedef std::vector<GlyphCursor> GlyphCursorVec;

    const GlyphCursorVec Shape() const;
#ifdef GB_USE_HARFBUZZ
    const GlyphCursorVec HarfBuzzShape() const;
#endif
    const GlyphCursorVec FreeTypeShape() const;
    void UpdateCache(const GlyphCursorVec& glyphCursorVec);
    void WordWrapAndGenerateQuads(const GlyphCursorVec& glyphCursorVec);

    std::shared_ptr<Font> m_font;
    std::string m_string; // utf8 encoding.
    std::string m_script; // 4 char code from iso 15952, http://unicode.org/iso15924/
    enum Direction { Direction_LTR = 0, Direction_RTL };
    Direction m_dir;
    void* m_userData;
    IntPoint m_origin;  // bounding rectangle, used for word-wrapping & alignment
    IntPoint m_size;
    TextHorizontalAlign m_horizontalAlign;
    TextVerticalAlign m_verticalAlign;
    uint32_t m_optionFlags;
    QuadVec m_quadVec;
    std::vector<std::shared_ptr<Glyph>> m_glyphVec;
};

} // namespace gb

#endif // GB_TEXT_H
