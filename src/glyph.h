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
    uint32_t GetFontIndex() const { return (uint32_t)(value >> 32); }
    uint32_t GetGlyphIndex() const { return (uint32_t)value; }
    uint64_t value;
};

class Glyph
{
public:
    Glyph(uint32_t index, const Font& font);
    ~Glyph();

    GlyphKey GetKey() const;
    IntPoint GetSize() const;
    IntPoint GetBearing() const;
    IntPoint GetOrigin() const;

protected:
    void InitImageAndSize(FT_Bitmap* ftBitmap, TextureFormat textureFormat, FontRenderOption renderOption);

    GlyphKey m_key;
    uint32_t m_glTexObj;
    IntPoint m_origin;
    IntPoint m_size;
    int m_advance;
    IntPoint m_bearing;
    std::unique_ptr<uint8_t> m_image;
};

} // namespace gb

#endif // GB_GLYPH_H
