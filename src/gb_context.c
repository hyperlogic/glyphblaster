#include <assert.h>
#include "gb_context.h"
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
        *gb_out = gb;
        return err;
    } else {
        return GB_ERROR_NOMEM;
    }
}

GB_ERROR GB_ContextReference(struct GB_Context* gb)
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
    GB_CacheFree(gb->cache);
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
