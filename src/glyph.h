#ifndef GB_GLYPH_H
#define GB_GLYPH_H

#include <stdint.h>
#include <memory>
#include <ft2build.h>
#include FT_FREETYPE_H
#include "glyphblaster.h"

namespace gb {

class Font;

struct GlyphKey
{
    GlyphKey(uint32_t glyphIndex, uint32_t fontIndex) : value(((uint64_t)fontIndex << 32) | glyphIndex) {}
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

    GlyphKey GetKey() const { return m_key; }
    IntPoint GetSize() const { return m_size; }
    IntPoint GetBearing() const { return m_bearing; }
    IntPoint GetOrigin() const { return m_origin; }
    void SetOrigin(IntPoint& origin) { m_origin = origin; }
    uint8_t* GetImage() const { return m_image.get(); }
    uint32_t GetTexObj() const { return m_texObj; }
    void SetTexObj(uint32_t texObj) { m_texObj = texObj; }
    int GetAdvance() const { return m_advance; }

protected:
    void InitImageAndSize(FT_Bitmap* ftBitmap, TextureFormat textureFormat,
                          FontRenderOption renderOption, uint32_t paddingBorder);

    GlyphKey m_key;
    uint32_t m_texObj;
    IntPoint m_origin;
    IntPoint m_size;
    int m_advance;
    IntPoint m_bearing;
    std::unique_ptr<uint8_t> m_image;
};

} // namespace gb

#endif // GB_GLYPH_H
