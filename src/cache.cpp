#include <assert.h>
#include <algorithm>
#include "cache.h"
#include "texture.h"
#include "context.h"

namespace gb {

Cache::Sheet::Sheet(uint32_t textureSize, TextureFormat textureFormat) :
    m_textureSize(textureSize),
    m_textureFormat(textureFormat)
{
#ifndef NDEBUG
    // in debug fill image with 128.
    const uint32_t kPixelSize = textureFormat == TextureFormat_Alpha ? 1 : 4;
    const uint32_t kImageSize = textureSize * textureSize * kPixelSize;
    std::unique_ptr<uint8_t> image(new uint8_t[kImageSize]);
    memset(image.get(), 0x80, kImageSize);
    m_texture = std::unique_ptr<Texture>(new Texture(textureFormat, textureSize, image.get()));
#else
    m_texture = std::unique_ptr<Texture>(new Texture(textureFormat, textureSize, nullptr));
#endif
}

bool Cache::Sheet::AddNewLevel(uint32_t height)
{
    SheetLevel* prevLevel = m_sheetLevelVec.empty() ? nullptr : m_sheetLevelVec.back().get();
    uint32_t baseline = prevLevel ? prevLevel->GetBaseline() + prevLevel->GetHeight() : 0;
    if ((baseline + height) <= m_textureSize)
    {
        std::unique_ptr<SheetLevel> sheetLevel(new SheetLevel(m_textureSize, baseline, height));
        m_sheetLevelVec.push_back(std::move(sheetLevel));
        return true;
    }
    else
    {
        return false;
    }
}

void Cache::Sheet::Clear()
{
    m_sheetLevelVec.clear();
}

uint32_t Cache::Sheet::GetTexObj() const
{
    return m_texture->GetTexObj();
}

const Texture* Cache::Sheet::GetTexture() const
{
    return m_texture.get();
}

bool Cache::SheetLevel::Insert(std::shared_ptr<Glyph> glyph)
{
    Glyph* prevGlyph = m_glyphVec.empty() ? nullptr : m_glyphVec.back().get();
    if (prevGlyph)
    {
        if (prevGlyph && prevGlyph->GetOrigin().x + prevGlyph->GetSize().x + glyph->GetSize().x <= m_textureSize)
        {
            auto o = IntPoint{prevGlyph->GetOrigin().x + prevGlyph->GetSize().x, (int)m_baseline};
            glyph->SetOrigin(o);
            m_glyphVec.push_back(glyph);
            return true;
        }
        else
        {
            // does not fit on this level
            return false;
        }
    }
    else if (glyph->GetSize().x <= m_textureSize)
    {
        auto o = IntPoint{0, (int)m_baseline};
        glyph->SetOrigin(o);
        m_glyphVec.push_back(glyph);
        return true;
    }
    else
    {
        // very rare glyph is larger then the width of the texture
        return false;
    }
}

bool Cache::Sheet::Insert(std::shared_ptr<Glyph> glyph)
{
    for (auto &sheetLevel : m_sheetLevelVec)
    {
        if (glyph->GetSize().y <= sheetLevel->GetHeight() && sheetLevel->Insert(glyph))
        {
            glyph->SetTexObj(m_texture->GetTexObj());
            m_texture->Subload(glyph->GetOrigin(), glyph->GetSize(), glyph->GetImage());
            return true;
        }
    }

    // need to add a new level
    if (AddNewLevel(glyph->GetSize().y) && m_sheetLevelVec.back()->Insert(glyph))
    {
        glyph->SetTexObj(m_texture->GetTexObj());
        m_texture->Subload(glyph->GetOrigin(), glyph->GetSize(), glyph->GetImage());
        return true;
    }
    else
    {
        // out of room
        glyph->SetTexObj(0);
        return false;
    }
}

Cache::Cache(uint32_t textureSize, uint32_t numSheets, TextureFormat textureFormat) :
    m_textureSize(textureSize)
{
    m_sheetVec.reserve(numSheets);
    for (uint32_t i = 0; i < numSheets; i++)
    {
        std::unique_ptr<Sheet> sheet(new Sheet(textureSize, textureFormat));
        m_sheetVec.push_back(std::move(sheet));
    }
}

Cache::~Cache()
{
    ;
}

bool Cache::InsertIntoSheets(std::shared_ptr<Glyph> glyph)
{
    for (auto &sheet : m_sheetVec)
    {
        if (sheet->Insert(glyph))
            return true;
    }
    return false;
}

void Cache::Compact()
{
    printf("Cache::Compact()\n");

    Context& context = Context::Get();

    // build a vector of all glyphs in the context.
    std::vector<std::shared_ptr<Glyph>> glyphVec;
    context.GetAllGlyphs(glyphVec);

    // clear all sheets
    for (auto &sheet : m_sheetVec)
    {
        sheet->Clear();
    }

    // sort glyphs in decreasing height
    std::sort(glyphVec.begin(), glyphVec.end(), [](const std::shared_ptr<Glyph>& a, const std::shared_ptr<Glyph>& b)
    {
        return b->GetSize().y < a->GetSize().y;
    });

    // re-insert glyphs into the sheets in sorted order.
    // which should improve packing efficiency.
    for (auto &glyph : glyphVec)
    {
        InsertIntoSheets(glyph);
    }
}

void Cache::GetTextureObjects(std::vector<uint32_t>& texVec) const
{
    texVec.clear();
    for (auto &sheet : m_sheetVec)
    {
        texVec.push_back(sheet->GetTexObj());
    }
}

void Cache::GenerateMipmap() const
{
    for (auto &sheet : m_sheetVec)
    {
        sheet->GetTexture()->GenerateMipmap();
    }
}

} // namespace gb
