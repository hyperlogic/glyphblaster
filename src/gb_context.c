#include <assert.h>
#include "gb_context.h"
#include "gb_glyph.h"
#include "gb_cache.h"

GB_ERROR GB_ContextMake(struct GB_Context** gb_out)
{
    struct GB_Context* gb = (struct GB_Context*)malloc(sizeof(struct GB_Context));
    if (gb) {
        memset(gb, 0, sizeof(struct GB_Context));
        gb->rc = 1;
        if (FT_Init_FreeType(&gb->ft_library)) {
            return GB_ERROR_FTERR;
        }
        struct GB_Cache* cache = NULL;
        GB_ERROR err = GB_CacheMake(&cache);
        if (err == GB_ERROR_NONE) {
            gb->cache = cache;
        }
        gb->font_list = NULL;
        gb->glyph_hash = NULL;
        gb->next_font_index = 0;
        *gb_out = gb;
        return err;
    } else {
        return GB_ERROR_NOMEM;
    }
}

GB_ERROR GB_ContextRetain(struct GB_Context* gb)
{
    if (gb) {
        assert(gb->rc > 0);
        gb->rc++;
        return GB_ERROR_NONE;
    } else {
        return GB_ERROR_INVAL;
    }
}

static void _GB_ContextDestroy(struct GB_Context* gb)
{
    assert(gb);
    if (gb->ft_library) {
        FT_Done_FreeType(gb->ft_library);
    }
    GB_CacheDestroy(gb->cache);
    free(gb);
}

GB_ERROR GB_ContextRelease(struct GB_Context* gb)
{
    if (gb) {
        gb->rc--;
        assert(gb->rc >= 0);
        if (gb->rc == 0) {
            _GB_ContextDestroy(gb);
        }
        return GB_ERROR_NONE;
    } else {
        return GB_ERROR_INVAL;
    }
}

GB_ERROR GB_ContextCompact(struct GB_Context* gb)
{
    return GB_CacheCompact(gb, gb->cache);
}

void GB_ContextHashAdd(struct GB_Context* gb, struct GB_Glyph* glyph)
{
#ifndef NDEBUG
    if (GB_ContextHashFind(gb, glyph->index, glyph->font_index)) {
        printf("GB_ContextHashAdd() WARNING glyph index = %d, font_index = %d is already in context!\n", glyph->index, glyph->font_index);
    }
#endif
    printf("GB_ContextHashAdd() index = %d, font_index = %d\n", glyph->index, glyph->font_index);
    HASH_ADD(context_hh, gb->glyph_hash, key, sizeof(uint64_t), glyph);
    GB_GlyphRetain(glyph);
}

struct GB_Glyph* GB_ContextHashFind(struct GB_Context* gb, uint32_t glyph_index, uint32_t font_index)
{
    struct GB_Glyph* glyph = NULL;
    uint64_t key = ((uint64_t)font_index << 32) | glyph_index;
    HASH_FIND(context_hh, gb->glyph_hash, &key, sizeof(uint64_t), glyph);
    return glyph;
}

void GB_ContextHashRemove(struct GB_Context* gb, uint32_t glyph_index, uint32_t font_index)
{
    printf("GB_ContextHashRemove() index = %d, font_index = %d\n", glyph_index, font_index);
    struct GB_Glyph* glyph = NULL;
    uint64_t key = ((uint64_t)font_index << 32) | glyph_index;
    HASH_FIND(context_hh, gb->glyph_hash, &key, sizeof(uint64_t), glyph);
    if (glyph) {
        HASH_DELETE(context_hh, gb->glyph_hash, glyph);
        GB_GlyphRelease(glyph);
    }
}

struct GB_Glyph** GB_ContextHashValues(struct GB_Context* gb, uint32_t* num_ptrs_out)
{
    int num_glyph_ptrs = HASH_CNT(context_hh, gb->glyph_hash);
    struct GB_Glyph** glyph_ptrs = (struct GB_Glyph**)malloc(sizeof(struct GB_Glyph*) * num_glyph_ptrs);
    struct GB_Glyph* glyph;
    int i = 0;
    for (glyph = gb->glyph_hash; glyph != NULL; glyph = glyph->context_hh.next) {
        glyph_ptrs[i++] = glyph;
    }
    *num_ptrs_out = num_glyph_ptrs;
    return glyph_ptrs;
}
