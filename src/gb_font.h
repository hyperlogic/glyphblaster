#ifndef GB_FONT_H
#define GB_FONT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <harfbuzz/hb.h>
#include "gb_error.h"

struct GB_Context;

enum GB_FontRenderOptions { GB_RENDER_NORMAL = 0, GB_RENDER_LIGHT, GB_RENDER_MONO,
                            GB_RENDER_LCD_RGB, GB_RENDER_LCD_BGR, 
                            GB_RENDER_LCD_RGB_V, GB_RENDER_LCD_BGR_V };
enum GB_FontHintOptions { GB_HINT_DEFAULT = 0, GB_HINT_FORCE_AUTO, GB_HINT_NO_AUTO, GB_HINT_NONE };

struct GB_Font {
    int32_t rc;
    uint32_t index;
    FT_Face ft_face;
    hb_font_t *hb_font;
    struct GB_Font *prev;
    struct GB_Font *next;
    enum GB_FontRenderOptions render_options;
    enum GB_FontHintOptions hint_options;
    uint32_t flags;
};

GB_ERROR GB_FontMake(struct GB_Context *gb, const char *filename, uint32_t point_size,
                     enum GB_FontRenderOptions render_options, enum GB_FontHintOptions hint_options,
                     struct GB_Font **font_out);
GB_ERROR GB_FontRetain(struct GB_Context *gb, struct GB_Font *font);
GB_ERROR GB_FontRelease(struct GB_Context *gb, struct GB_Font *font);

#ifdef __cplusplus
}
#endif

#endif // GB_FONT_H
