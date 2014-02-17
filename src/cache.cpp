#include <assert.h>
#include "cache.h"

namespace gb {

Cache::Sheet::Sheet(uint32_t textureSize, TextureFormat textureFormat) :
    m_textureSize(textureSize),
    m_textureFormat(textureFormat)
{
#ifndef NDEBUG
    // in debug fill image with 128.
    const uint32_t kPixelSize = textureFormat == GB_TEXTURE_FORMAT_ALPHA ? 1 : 4;
    const uint32_t kImageSize = textureSize * textureSize * kPixelSize;
    std::unique_ptr<uint8_t> image(new uint8_t[kImageSize]);
    memset(image.get(), 0x80, kImageSize);
    TextureInit(textureFormat, textureSize, image.get(), &this->m_glTexObj);
#else
    TextureInit(textureFormat, textureSize, nullptr, &this->m_glTexObj)
#endif
}

Cache::Sheet::~Sheet()
{
    TextureDestroy(this->m_glTexObj);
}

bool Cache::Sheet::AddNewLevel(uint32_t height)
{
    SheetLevel* prevLevel = m_sheetLevelVec.empty() ? nullptr : m_sheetLevelVec.last().get();
    uint32_t baseline = prevLevel ? prevLevel->m_baseline + prevLevel->m_height : 0;
    if ((baseline + height) <= m_textureSize)
    {
        m_sheetLevelVec.push_back(new SheetLevel(baseline, height));
        return true;
    }
    else
    {
        return false;
    }
}

void Cache::Sheet::SubloadGlyph(Glyph& glyph)
{
    TextureSubLoad(m_glTexObj, m_textureFormat, glyph.m_origin, glyph.m_size, glyph.m_image);
}

void Cache::Sheet::Clear()
{
    m_sheetLevelVec.clear();
}

bool Cache::SheetLevel::Insert(Glyph& glyph)
{
    Glyph* prevGlyph = m_glyphVec.empty() ? nullptr : m_glyphVec.last().get();
    if (prevGlyph && prevGlyph->m_origin.x + prevGlyph->m_size.x + glyph.m_size.x <= m_textureSize)
    {
        glyph->origin.x = prevGlyph->origin.x + prevGlyph->m_size.x;
        glyph->origin.y = m_baseline;
        m_glyphVec.push_back(glyph);
        return true;
    }
    else if (glyph.m_size.x <= m_textureSize)
    {
        glyph.m_origin.x = 0;
        glyph.m_origin.y = m_baseline;
        m_glyphVec.push_back(glyph);
        return true;
    }
    else
    {
        // very rare glyph is larger then the width of the texture
        return false;
    }
}

bool Cache::Sheet::Insert(Glyph& glyph)
{
    for (auto sheetLevel : m_sheetLevelVec)
    {
        if (glyph.size.y <= sheetLevel->m_height && sheetLevel->InsertGlyph(glyph))
        {
            glyph->m_glTexObj = m_glTexObj;
            sheet->SubloadGlyph(glyph);
            return true;
        }
    }

    // need to add a new level
    if (AddNewLevel(glyph->size.y) && m_sheetLevelVec.last()->InsertGlyph(glyph))
    {
        glyph->m_glTexObj = m_glTexObj;
        sheet->SubloadGlyph(glyph);
        return true;
    }
    else
    {
        // out of room
        glyph->glTexObj = 0;
        return false;
    }
}

Cache::Cache(uint32_t textureSize, uint32_t numSheets, TextureFormat textureFormat) :
    m_textureSize(textureSize),
    m_full(false)
{
    m_sheetVec.reserve(numSheets);
    for (uint32_t i = 0; i < numSheets; i++)
        m_sheetVec.push_back(new Sheet(textureSize, textureFormat));
}

Cache::~Cache()
{
    ;
}

bool Cache::InsertIntoSheets(Glyph& glyph)
{
    for (auto sheet : m_sheetVec)
    {
        if (sheet->Insert(glyph))
            return true;
    }
    return false;
}

void Cache::Compact()
{
    Context* context = Context::Get();

    // build a vector of all glyphs in the context.
    std::vector<std::shared_ptr<Glyph>> glyphVec;
    glyphVec.reserve(context->m_glyphMap.size());
    for (auto kv : m_glyphMap)
    {
        glyphVec.push_back(kv->second);
    }

    // clear all sheets
    for (auto sheet : m_sheetVec)
    {
        sheet->Clear();
    }

    // clear all glyph instances in the map.
    m_glyphVec.clear();

    // sort glyphs in decreasing height
    std::sort(glyphVec.begin(), glyphVec.end(), [](Glyph* a, Glyph* b)
    {
        return b->GetSize().y - a->GetSize().y;
    });

    // re-insert glyphs into the sheets in sorted order.
    // which should improve packing efficiency.
    for (auto glyph : glyphVec)
    {
        InsertGlyph(glyph);
    }
}

void Cache::Insert(std::vector<std::shared_ptr<Glyph>> glyphVec);
{
    Context* context = Context::Get();

    // sort glyphs in decreasing height
    std::sort(glyphVec.begin(), glyphVec.end(), [](Glyph* a, Glyph* b)
    {
        return b->GetSize().y - a->GetSize().y;
    });

    bool full = false;

    // decreasing height find-first heuristic.
    for (auto glyph : glyphVec)
    {
        // make sure duplicates don't end up in the texture
        if (m_glyphVec.find(glyph->GetKey()) == m_glyphVec.end())
        {
            if (!InsertIntoSheets(glyph))
            {
                if (!full)
                {
                    // compact and try again.
                    Compact();
                    if (!InsertIntoSheets(glyph))
                    {
                        // TODO: could try adding another texture sheet in the future.
                        full = true;
                    }
                }
            }

            // add to both context and cache maps.
            InsertIntoMap(glyph);
            context->InsertIntoMap(glyph);
        }
    }
}

void InsertIntoMap(std::shared_ptr<Glyph> glyph)
{
#ifndef NDEBUG
    if (FindInMap(glyph)) {
        printf("GB_CacheHashAdd() WARNING glyph index = %d, font_index = %d is already in cache!\n", glyph->index, glyph->font_index);
    }
#endif
    m_glyphMap[glyph->GetKey()] = glyph;
}

std::shared_ptr<Glyph> Cache::FindInMap(GlyphKey key) const
{
    auto iter = m_glyphMap.find(key);
    if (iter != m_glyphMap.end())
        return iter->second;
    else
        return std::shared_ptr<Glyph>();
}

} // namespace gb
