#ifndef GB_CONTEXT_H
#define GB_CONTEXT_H

#include <stdint.h>
#include <memory>
#include <vector>
#include <map>
#include <ft2build.h>
#include FT_FREETYPE_H

#include "glyphblaster.h"

namespace gb {

class Cache;
class Glyph;
class GlyphKey;
class Quad;
class Texture;

typedef std::vector<std::unique_ptr<Quad>> QuadVec;
typedef std::function<void (const QuadVec&)> RenderFunc;

class Context
{
public:
    static void Init(uint32_t textureSize, uint32_t numSheets, TextureFormat textureFormat);
    static void Shutdown();
    static Context* Get();

protected:
    Context(uint32_t textureSize, uint32_t numSheets, TextureFormat textureFormat);
    ~Context();

public:
    TextureFormat GetTextureFormat() const { return m_textureFormat; }
    void SetRenderFunc(RenderFunc renderFunc);
    void ClearRenderFunc();
    void Compact();

protected:
    // Used by Font to create FT_Face objects
    FT_Library GetFTLibrary() const { return m_ftLibrary; }
    uint32_t GetNextFontIndex() { return m_nextFontIndex++; }

    // Used by Text instances.
    // Notifies context that a new glyph was created.
    void InsertIntoMap(std::shared_ptr<Glyph> glyph);

    // Notifies context that a glyph is no longer needed.
    void RemoveFromMap(GlyphKey key);

    // Used to avoid creating multiple copies of the same glyph.
    std::shared_ptr<Glyph> FindInMap(GlyphKey key);

    FT_Library m_ftLibrary;
    std::unique_ptr<Cache> m_cache;

    // all glyphs currently in use by Text instances.
    // NOTE: this is a sub-set of all the glyphs held onto by m_cache.
    std::map<GlyphKey, std::shared_ptr<Glyph>> m_glyphMap;

    uint32_t m_nextFontIndex;
    std::shared_ptr<Texture> m_fallbackTexture;
    RenderFunc m_renderFunc;
    TextureFormat m_textureFormat;

    GB_NO_COPY(Context)
};

} // namespace gb

#endif // GB_CONTEXT_H
