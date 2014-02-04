#include <assert.h>
#include "gb_context.h"
#include "gb_glyph.h"
#include "gb_cache.h"
#include "gb_text.h"
#include "gb_texture.h"

namespace gb {

static std::shared_ptr<Texture> CreateFallbackTexture()
{
    uint32_t texObj;
    const int imageSize = 16 * 16;
    std::unique_ptr<uint8_t> image = uint8_t[imageSize];

    // fallback texture is gray
    memset(image.get(), 128, imageSize);

    return new Texture(TextureFormat_Alpha, texture_size, image.get());
}

static void NullRenderFunc(const std::vector<std::unique_ptr<Quad>>& quads) {}

static int IsPowerOfTwo(uint32_t x)
{
    return ((x != 0) && !(x & (x - 1)));
}

static int PowerOfTwoRoundUp(int x)
{
    if (x < 0)
        return 0;
    --x;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    return x + 1;
}

Context::Context(uint32_t textureSize, uint32_t numSheets, TextureFormat textureFormat) :
    m_ftLibrary(nullptr),
    m_cache(new Cache(PowerOfTwoRoundUp(textureSize), numSheets, textureFormat)),
    m_nextFontIndex(0),
    m_fallbackTexture(CreateFallbackTexture()),
    m_renderFunc = NullRenderFunc,
    m_textureFormat = textureFormat
{
    if (FT_Init_FreeType(&m_ftLibrary))
    {
        fprintf(stderr, "FT_Init_FreeType failed");
        abort();
    }

#ifndef NDEBUG
    FT_Int major, minor, patch;
    FT_Library_Version(m_ftLibrary, &major, &minor, &patch);
    printf("FT_Version %d.%d.%d\n", major, minor, patch);
#endif

    m_fallbackTexObj = CreateFallbackTexture();
}

Context::~Context()
{
    if (m_ftLibrary)
        FT_Done_FreeType(m_ftLibrary);
}

void Context::SetTextRenderFunc(RenderFunc renderFunc)
{
    m_renderFunc = renderFunc;
}

void Context::ClearRenderFunc()
{
    m_renderFunc = NullRenderFunc;
}

void Context::Compact()
{
    m_cache->Compact();
}

void Context::GlyphAdd(std::shared_ptr<Glyph> glyph)
{
    m_glyphMap[glyph->GetKey()] = glyph;
}

void Context::GlyphRemove(GlyphKey key)
{
    m_glyphMap.erase(key);
}

std::shared_ptr<Glyph> Context::HashFind(GlyphKey key)
{
    auto iter = m_glyphMap.find(key);
    if (iter != m_glyphMap.end())
        return iter->second;
    else
        return std::shared_ptr<Glyph>();
}
