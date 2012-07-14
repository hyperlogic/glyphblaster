#include "glyphblaster.h"
#include <stdlib.h>

GB_ERROR_CODE GB_Init(GB_GB** gb_out)
{
    *gb_out = (GB_GB*)malloc(sizeof(GB_GB));
    if (*gb_out) {
        return GB_ERROR_NONE;
    } else {
        return GB_ERROR_NOMEM;
    }
}

GB_ERROR_CODE GB_Shutdown(GB_GB* gb)
{
    if (gb) {
        free(gb);
        return GB_ERROR_NONE;
    } else {
        return GB_ERROR_NOMEM;
    }
}

GB_ERROR_CODE GB_MakeFont(GB_GB* gb, const char* filename, uint32_t point_size,
                          GB_FONT** font_out)
{
    if (gb && filename && font_out) {
        return GB_ERROR_NOIMP;
    } else {
        return GB_ERROR_INVAL;
    }
}

GB_ERROR_CODE GB_ReleaseFont(GB_GB* gb, GB_FONT* font)
{
    if (gb && font) {
        return GB_ERROR_NOIMP;
    } else {
        return GB_ERROR_INVAL;
    }
}

GB_ERROR_CODE GB_MakeText(GB_GB* gb, const char* utf8_string, GB_FONT* font,
                          uint32_t color, uint32_t min[2], uint32_t max[2],
                          GB_HORIZONTAL_ALIGN horizontal_align,
                          GB_VERTICAL_ALIGN vertical_align,
                          GB_TEXT** text_out)
{
    if (gb && utf8_string && font && text_out) {
        return GB_ERROR_NOIMP;
    } else {
        return GB_ERROR_INVAL;
    }
}

GB_ERROR_CODE GB_ReleaseText(GB_GB* gb, GB_TEXT* text)
{
    if (gb && text) {
        return GB_ERROR_NOIMP;
    } else {
        return GB_ERROR_INVAL;
    }
}

GB_ERROR_CODE GB_GetTextMetrics(GB_GB* gb, const char* utf8_string,
                                GB_FONT* font, uint32_t min[2], uint32_t max[2],
                                GB_HORIZONTAL_ALIGN horizontal_align,
                                GB_VERTICAL_ALIGN vertical_align,
                                GB_TEXT_METRICS* text_metrics_out)
{
    if (gb && utf8_string && font && text_metrics_out) {
        return GB_ERROR_NOIMP;
    } else {
        return GB_ERROR_INVAL;
    }
}

// Creates textures, packs and subloads glyphs into texture cache.
// Should be called once a frame, before GB_DrawText
// NOTE: issues OpenGL texture bind commands.
GB_ERROR_CODE GB_Update(GB_GB* gb)
{
    if (gb) {
        return GB_ERROR_NOIMP;
    } else {
        return GB_ERROR_INVAL;
    }
}

// Renders given text using renderer func.
GB_ERROR_CODE GB_DrawText(GB_GB* gb, GB_TEXT* text)
{
    if (gb && text) {
        return GB_ERROR_NOIMP;
    } else {
        return GB_ERROR_INVAL;
    }
}
