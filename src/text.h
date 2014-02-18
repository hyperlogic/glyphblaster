#ifndef GB_TEXT_H
#define GB_TEXT_H

#include <stdint.h>
#include <string>
#include <harfbuzz/hb.h>
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
    TextVerticalAligh_Bottom,
    TextVerticalAligh_Center
};

enum TextOptionFlags {
    TextOptionFlags_None = 0,
    TextOptionFlags_DisableShaping = 0x01
};

class Text
{
public:
    // string is assumed to be utf8 encoded.
    Text(const std::string& string, std::shared_ptr<Font> font,
         void* userData, IntPoint origin, IntPoint size,
         TextHorizontalAlign horizontalAlign, TextVerticalAlign verticalAlign,
         uint32_t optionFlags);
    ~Text();
    void Draw();

protected:
    void UpdateCache();
    void WordWrapAndGenerateQuads();

    std::shared_ptr<Font> m_font;
    std::string m_string; // utf8 encoding.
    hb_buffer_t *m_hbBuffer;  // harfbuzz buffer, used for shaping
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
