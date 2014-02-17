#ifndef GB_GLYPH_H
#define GB_GLYPH_H

#include <stdint.h>
#include "gb_error.h"
#include "gb_context.h"

namespace gb {

struct GlyphKey
{
    GlyphKey(uint32_t glyphIndex, uint32_t fontIndex) : key(((uint64_t)fontIndex << 32) | glyphIndex) {}
    bool operator<(const GlyphKey& rhs) const { return value < rhs.value; }
    uint64_t value;
};

class Glyph
{
public:
    Glyph(const Context& context, uint32_t index, const Font& font);
    ~Glyph();

    GlyphKey GetKey() const;
    Point GetSize() const;

protected:
    void InitImageAndSize(FT_Bitmap* ftBitmap, TextureFormat textureFormat, FontRenderOption renderOption);

    GlyphKey m_key;
    uint32_t m_glTexObj;
    Point m_origin;
    Point m_size;
    int m_advance;
    Point m_bearing;
    std::unique_ptr<uint8_t> m_image;
};

} // namespace gb

#endif // GB_GLYPH_H
