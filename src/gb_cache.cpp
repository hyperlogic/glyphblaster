#include "utlist.h"
#include <assert.h>
#include "gb_context.h"
#include "gb_glyph.h"
#include "gb_font.h"
#include "gb_cache.h"
#include "gb_texture.h"

Sheet::Sheet(uint32_t textureSize, TextureFormat textureFormat) :
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

bool Sheet::AddNewLevel(uint32_t height)
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

void Sheet::SubloadGlyph(Glyph& glyph)
{
    TextureSubLoad(m_glTexObj, m_textureFormat, glyph.m_origin, glyph.m_size, glyph.m_image);
}

bool SheetLevel::InsertGlyph(Glyph& glyph)
{
    Glyph* prevGlyph = m_glyphVec.empty() ? nullptr : m_glyphVec.last().get();
    if (prevGlyph && prevGlyph->m_origin.x + prevGlyph->size.x + glyph->size.x <= m_textureSize)
    {
        glyph->origin.x = prevGlyph->origin.x + prevGlyph->size.x;
        glyph->origin.y = m_baseline;
        m_glyphVec.push_back(glyph);
        return true;
    }
    else if (glyph->size.x <= m_textureSize)
    {
        glyph->origin.x = 0;
        glyph->origin.y = m_baseline;
        m_glyphVec.push_back(glyph);
        return true;
    }
    else
    {
        // very rare glyph is larger then the width of the texture
        return false;
    }
}

void Sheet::InsertGlyph(Glyph& glyph)
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

/// TODO

Cache::Cache(uint32_t textureSize, uint32_t numSheets, TextureFormat textureFormat)
{

}

GB_ERROR GB_CacheMake(uint32_t texture_size, uint32_t num_sheets, enum GB_TextureFormat texture_format,
                      struct GB_Cache **cache_out)
{
    struct GB_Cache *cache = (struct GB_Cache*)malloc(sizeof(struct GB_Cache));
    memset(cache, 0, sizeof(struct GB_Cache));

    cache->num_sheets = num_sheets;
    cache->texture_size = texture_size;
    cache->glyph_hash = NULL;

    uint32_t i;
    for (i = 0; i < num_sheets; i++)
        _GB_SheetInit(cache, texture_format, &cache->sheet[i]);

    *cache_out = cache;
    return GB_ERROR_NONE;
}

GB_ERROR GB_CacheDestroy(struct GB_Cache *cache)
{
    if (cache) {
        // release all glyphs
        struct GB_Glyph *glyph, *tmp;
        HASH_ITER(cache_hh, cache->glyph_hash, glyph, tmp) {
            HASH_DELETE(cache_hh, cache->glyph_hash, glyph);
            GB_GlyphRelease(glyph);
        }

        // destroy all textures
        int i;
        for (i = 0; i < cache->num_sheets; i++)
            GB_TextureDestroy(cache->sheet[i].gl_tex_obj);

        free(cache);
    }
    return GB_ERROR_NONE;
}

static int _GB_CacheInsertGlyph(struct GB_Cache *cache, struct GB_Glyph *glyph)
{
    int i;
    for (i = 0; i < cache->num_sheets; i++) {
        struct GB_Sheet *sheet = cache->sheet + i;
        if (_GB_SheetInsertGlyph(cache, sheet, glyph)) {
            return 1;
        }
    }
    return 0;
}

static int _GB_CacheAddSheet(struct GB_Cache *cache)
{
    // TODO:
    return 0;
}

// sort glyph ptrs in decreasing height
static int glyph_cmp(const void *a, const void *b)
{
    return (*(struct GB_Glyph**)b)->size[1] - (*(struct GB_Glyph**)a)->size[1];
}

GB_ERROR GB_CacheCompact(struct GB_Context *gb, struct GB_Cache *cache)
{
    uint32_t num_glyph_ptrs;
    struct GB_Glyph **glyph_ptrs = GB_ContextHashValues(gb, &num_glyph_ptrs);

    // The goal of all this retaining & releasing is to make sure we dispose of all
    // glyphs that are in the cache but aren't in the context.
    // i.e. clear out all the unused glyphs.

    // retain all glyphs in the context
    int i;
    for (i = 0; i < num_glyph_ptrs; i++) {
        GB_GlyphRetain(glyph_ptrs[i]);
    }

    // release all glyphs in the cache
    struct GB_Glyph *glyph;
    for (glyph = cache->glyph_hash; glyph != NULL; glyph = glyph->cache_hh.next) {
        GB_GlyphRelease(glyph);
    }
    HASH_CLEAR(cache_hh, cache->glyph_hash);

    // Clear all sheets
    for (i = 0; i < cache->num_sheets; i++) {
        struct GB_Sheet *sheet = &cache->sheet[i];
        sheet->num_levels = 0;
    }

    // sort glyphs in decreasing height
    qsort(glyph_ptrs, num_glyph_ptrs, sizeof(struct GB_Glyph*), glyph_cmp);

    // decreasing height find-first heuristic.
    for (i = 0; i < num_glyph_ptrs; i++) {
        struct GB_Glyph *glyph = glyph_ptrs[i];
        if (!_GB_CacheInsertGlyph(cache, glyph)) {
            return GB_ERROR_NOMEM;
        }
        GB_CacheHashAdd(cache, glyph);
    }

    // release all glyphs in the context, restoring their proper retain counts.
    for (i = 0; i < num_glyph_ptrs; i++) {
        GB_GlyphRelease(glyph_ptrs[i]);
    }

    free(glyph_ptrs);

    return GB_ERROR_NONE;
}

GB_ERROR GB_CacheInsert(struct GB_Context *gb, struct GB_Cache *cache,
                        struct GB_Glyph **glyph_ptrs, int num_glyph_ptrs)
{
    int i;
    int cache_full = 0;

    // sort glyphs in decreasing height
    qsort(glyph_ptrs, num_glyph_ptrs, sizeof(struct GB_Glyph*), glyph_cmp);

    // decreasing height find-first heuristic.
    for (i = 0; i < num_glyph_ptrs; i++) {
        struct GB_Glyph *glyph = glyph_ptrs[i];
        // make sure duplicates don't end up in the hash
        if (!GB_CacheHashFind(cache, glyph->index, glyph->font_index)) {
            if (!_GB_CacheInsertGlyph(cache, glyph)) {

                if (!cache_full) {
                    // compact and try again.
                    GB_ERROR error = GB_CacheCompact(gb, cache);
                    if (error == GB_ERROR_NONE) {
                        if (!_GB_CacheInsertGlyph(cache, glyph)) {
                            // Add another sheet and try again.
                            if (_GB_CacheAddSheet(cache)) {
                                if (!_GB_CacheInsertGlyph(cache, glyph)) {
                                    // glyph must be too big to fit in a single sheet.
                                    assert(0);
                                    return GB_ERROR_NOMEM;
                                }
                            } else {
                                // failed to add sheet, no room
                                cache_full = 1;
                            }
                        }
                    } else {
                        return error;
                    }
                }
            }
            // add new glyph to both hashes
            GB_CacheHashAdd(cache, glyph);
            GB_ContextHashAdd(gb, glyph);
            GB_GlyphRelease(glyph);
        }
    }

    return GB_ERROR_NONE;
}

void GB_CacheHashAdd(struct GB_Cache *cache, struct GB_Glyph *glyph)
{
#ifndef NDEBUG
    if (GB_CacheHashFind(cache, glyph->index, glyph->font_index)) {
        printf("GB_CacheHashAdd() WARNING glyph index = %d, font_index = %d is already in cache!\n", glyph->index, glyph->font_index);
    }
#endif
    HASH_ADD(cache_hh, cache->glyph_hash, key, sizeof(uint64_t), glyph);
    GB_GlyphRetain(glyph);
}

struct GB_Glyph *GB_CacheHashFind(struct GB_Cache *cache, uint32_t glyph_index, uint32_t font_index)
{
    struct GB_Glyph *glyph = NULL;
    uint64_t key = ((uint64_t)font_index << 32) | glyph_index;
    HASH_FIND(cache_hh, cache->glyph_hash, &key, sizeof(uint64_t), glyph);
    return glyph;
}
