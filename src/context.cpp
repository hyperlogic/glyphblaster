#include <assert.h>
#include "gb_context.h"
#include "gb_glyph.h"
#include "gb_cache.h"
#include "gb_text.h"
#include "gb_texture.h"

namespace gb {

static Context* s_context;

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

void Context::Init(uint32_t textureSize, uint32_t numSheets, TextureFormat textureFormat)
{
    assert(!s_context);
    if (!s_context)
    {
        s_context = new Context(textureSize, numSheets, textureFormat);
    }
}

void Context::Shutdown()
{
    assert(s_context);
    if (s_context)
    {
        delete s_context;
        s_context = nullptr;
    }
}

Context& Context::Get()
{
    return *s_context;
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

void Context::InsertIntoMap(std::shared_ptr<Glyph> glyph)
{
    m_glyphMap[glyph->GetKey()] = glyph;
}

std::weak_ptr<Glyph> Context::FindInMap(GlyphKey key)
{
    auto iter = m_glyphMap.find(key);
    if (iter != m_glyphMap.end())
        return iter->second;
    else
        return std::weak_ptr<Glyph>();
}

void Context::OnNotifyCreate(Font* font)
{
    font->m_index = m_nextFontIndex++;
    m_fontMap[font->m_index] = font;
}

void Context::OnNotifyDestroy(Font* font)
{
    m_fontMap.erase(font->m_index);
}

void RasterizeAndSubloadGlyphs(const std::vector<GlyphKey>& keyVecIn,
                               std::vector<std::shared_ptr<Glyph>>& glyphVecOut)
{
    for (auto key : keyVecIn)
    {
        // check to see if this glyph already exists.
        auto glyph = FindInMap(key);
        if (glyph.expired())
        {
            // look up font by index.
            auto iter = m_fontMap.find(key.GetFontIndex());
            Font* font = iter != m_fontMap.end() ? iter.second : nullptr;
            assert(font);

            // will rasterize and initialize glyph
            std::shared_ptr<Glyph> glyph = new Glyph(key.GetGlyphIndex(), font);

            // will subload glyph into texture atlas
            m_cache->InsertIntoSheets(glyph);

            InsertIntoMap(glyph);
            glyphVecOut.push_back(glyph);
        }
        else
        {
            glyphVecOut.push_back(glyph.lock());
        }
    }
}
