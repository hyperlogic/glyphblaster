#ifndef GLYPH_CACHE_H
#define GLYPH_CACHE_H

#include <stdint.h>
#include <memory>
#include <vector>
#include "glyphblaster.h"
#include "glyph.h"

namespace gb {

class Texture;

class Cache
{
    friend class Context;
public:
    Cache(uint32_t textureSize, uint32_t numSheets, TextureFormat textureFormat);
    ~Cache();
    void Compact();
    uint32_t GetTextureSize() const { return m_textureSize; }

    // for debugging
    // fills up texVec with the OpenGL GLuint texture handles for each sheet.
    void GetTextureObjects(std::vector<uint32_t>& texVec) const;

    void GenerateMipmap() const;

protected:
    bool InsertIntoSheets(std::shared_ptr<Glyph> glyph);

    class SheetLevel
    {
    public:
        SheetLevel(uint32_t textureSize, uint32_t baseline, uint32_t height) : m_textureSize(textureSize), m_baseline(baseline), m_height(height) {}
        bool Insert(std::shared_ptr<Glyph> glyph);
        uint32_t GetBaseline() const { return m_baseline; }
        uint32_t GetHeight() const { return m_height; }
    protected:
        std::vector<std::shared_ptr<Glyph>> m_glyphVec;
        uint32_t m_textureSize;
        uint32_t m_baseline;
        uint32_t m_height;

        GB_NO_COPY(SheetLevel);
    };

    class Sheet
    {
    public:
        Sheet(uint32_t textureSize, TextureFormat textureFormat);
        bool Insert(std::shared_ptr<Glyph> glyph);
        void Clear();
        uint32_t GetTexObj() const;
        const Texture* GetTexture() const;
    protected:
        bool AddNewLevel(uint32_t height);

        std::unique_ptr<Texture> m_texture;
        uint32_t m_textureSize;
        TextureFormat m_textureFormat;
        std::vector<std::unique_ptr<SheetLevel>> m_sheetLevelVec;

        GB_NO_COPY(Sheet);
    };

    std::vector<std::unique_ptr<Sheet>> m_sheetVec;
    uint32_t m_textureSize;

    GB_NO_COPY(Cache);
};

}

#endif
