#ifndef GB_TEXT_H
#define GB_TEXT_H

#include <stdint.h>
#include <harfbuzz/hb.h>
#include "gb_error.h"

namespace gb {

struct Point<type T>
{
    T x, y;
};

typedef Point<int> IntPoint;
typedef Point<float> FloatPoint;

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
    TextOptionFlags_DisableShaping = 0x01
};

// y axis points down
// origin is upper-left corner of glyph
struct Quad
{
    IntPoint pen;
    IntPoint origin;
    IntPoint size;
    FloatPoint uvOrigin;
    FloatPoint uvSize;
    void* userData;
    uint32_t glTexObj;
};

class Text
{
public:
    Text(Context& context, std::string utf8String, std::shared_ptr<Font> font,
         void* userData, IntPoint origin, IntPoint size,
         TextHorizontalAlign horizontalAlign, TextVerticalAlign verticalAlign,
         uint32_t flags);

protected:
    void UpdateCache(Context& context);

    std::shared_ptr<Font> m_font;
    std::string m_utf8String;
    hb_buffer_t *m_hbBuffer;  // harfbuzz buffer, used for shaping
    void* m_userData;
    IntPoint m_origin[2];  // bounding rectangle, used for word-wrapping & alignment
    IntPoint m_size[2];
    TextHorizontalAlign m_horizontalAlign;
    TextVerticalAlign m_verticalAlign;
    uint32_t m_optionFlags;
    std::vector<Quad> m_quadVec;
};

} // namespace gb

#endif // GB_TEXT_H
