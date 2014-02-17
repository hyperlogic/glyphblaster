#ifndef GLYPH_CACHE_H
#define GLYPH_CACHE_H

#include <stdint.h>
#include "glyphblaster.h"

namespace gb {

class Cache
{
public:
    Cache(uint32_t textureSize, uint32_t numSheets, TextureFormat textureFormat);
    ~Cache();
    void Insert(std::vector<std::shared_ptr<Glyph>> glyphVec);
    void Compact();
protected:
    bool InsertIntoSheets(std::shared_ptr<Glyph> glyph);

    class SheetLevel
    {
    public:
        bool Insert(Glyph& glyph);
    protected:
        std::vector<std::shared_ptr<Glyph>> m_glyphVec;
        uint32_t m_textureSize;
        uint32_t m_baseline;
        uint32_t m_height;
    };

    class Sheet
    {
    public:
        Sheet(uint32_t textureSize, TextureFormat textureFormat);
        ~Sheet();
        bool Insert(Glyph& glyph);
        void Clear();
    protected:
        bool AddNewLevel(uint32_t height);
        void SubloadGlyph(Glyph& glyph);

        uint32_t m_glTexObj;
        uint32_t m_textureSize;
        TextureFormat m_textureFormat;
        std::vector<std::unique_ptr<Sheet>> m_sheetLevelVec;
    };

    std::vector<std::unique_ptr<Sheet>> m_sheetVec;
    uint32_t m_textureSize;

    GB_NO_COPY(Cache);
};

}

#endif
