#ifndef GLYPH_CACHE_H
#define GLYPH_CACHE_H

#include <stdint.h>
#include "gb_error.h"

namespace gb {

class Cache
{
public:
    Cache(uint32_t textureSize, uint32_t numSheets, TextureFormat textureFormat);
    ~Cache();
    Error::Code Insert(Context& context, std::vector<Glyph*> glyphs);
    Error::Code Compact(Context& context);
protected:
    void Add(Glyph* glyph);
    Glyph* Find(uint32_t glyphIndex, uint32_t fontIndex) const;

    class SheetLevel
    {
        std::vector< std::unique_ptr<Glyph> > m_glyphVec;
        uint32_t m_textureSize;
        uint32_t m_baseline;
        uint32_t m_height;
    };

    class Sheet
    {
    public:
        Sheet::Sheet(uint32_t textureSize, TextureFormat textureFormat);

    protected:
        bool Sheet::AddNewLevel(uint32_t height);
        void Sheet::SubloadGlyph(Glyph& glyph);

        uint32_t m_glTexObj;
        uint32_t m_textureSize;
        TextureFormat m_textureFormat;
        std::vector< std::unique_ptr <Sheet> > m_sheetLevelVec;
    };

    std::vector< std::unique_ptr<Sheet> > m_sheetVec;
    uint32_t textureSize;
    std::map<std::uint64_t, GB_Glyph> m_glyphMap;
};

}

#endif
