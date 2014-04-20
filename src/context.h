#ifndef GB_CONTEXT_H
#define GB_CONTEXT_H

#include <stdint.h>
#include <memory>
#include <vector>
#include <map>
#include <functional>
#include <ft2build.h>
#include FT_FREETYPE_H

#include "glyphblaster.h"
#include "texture.h"
#include "glyph.h"

namespace gb {

class Cache;
class Font;

typedef std::vector<Quad> QuadVec;
typedef std::function<void (const QuadVec&)> RenderFunc;

class Context
{
    friend class Cache;
    friend class Font;
    friend class Text;
public:
    static void Init(uint32_t textureSize, uint32_t numSheets, TextureFormat textureFormat);
    static void Shutdown();
    static Context& Get();

protected:
    Context(uint32_t textureSize, uint32_t numSheets, TextureFormat textureFormat);
    ~Context();

public:
    TextureFormat GetTextureFormat() const { return m_textureFormat; }
    void SetRenderFunc(RenderFunc renderFunc);
    void ClearRenderFunc();
    void Compact();
    const Cache& GetCache() { return *(m_cache.get()); }

protected:
    // Used by Font objects
    FT_Library GetFTLibrary() const { return m_ftLibrary; }
    void OnFontCreate(Font* font);
    void OnFontDestroy(Font* font);

    // Used by Text instances.
    void RasterizeAndSubloadGlyphs(const std::vector<GlyphKey>& keyVecIn,
                                   std::vector<std::shared_ptr<Glyph>>& glyphVecOut);

    void GetAllGlyphs(std::vector<std::shared_ptr<Glyph>>& glyphVecOut) const;

    void InsertIntoMap(std::shared_ptr<Glyph> glyph);
    const Texture& GetFallbackTexture() { return *(m_fallbackTexture.get()); }

    // Used to avoid creating multiple copies of the same glyph.
    std::weak_ptr<Glyph> FindInMap(GlyphKey key);

    FT_Library m_ftLibrary;
    std::unique_ptr<Cache> m_cache;

    // holds all glyph instances
    std::map<GlyphKey, std::weak_ptr<Glyph>> m_glyphMap;

    // holds all font instances
    std::map<uint32_t, Font*> m_fontMap;

    uint32_t m_nextFontIndex;
    std::unique_ptr<Texture> m_fallbackTexture;
    RenderFunc m_renderFunc;
    TextureFormat m_textureFormat;

    GB_NO_COPY(Context)
};

} // namespace gb

#endif // GB_CONTEXT_H
