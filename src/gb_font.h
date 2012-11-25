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

// argument to GB_FontMake
enum GB_FontRenderOptions {
    GB_RENDER_NORMAL = 0,  // normal anti-aliased font rendering
    GB_RENDER_LIGHT,  // lighter anti-aliased outline hinting, this will force auto hinting.
    GB_RENDER_MONO,  // no anti-aliasing
    GB_RENDER_LCD_RGB,  // subpixel anti-aliasing, designed for LCD RGB displays
    GB_RENDER_LCD_BGR,  // subpixel anti-aliasing, designed for LCD BGR displays
    GB_RENDER_LCD_RGB_V,  // vertical subpixel anti-aliasing, designed for LCD RGB displays
    GB_RENDER_LCD_BGR_V   // vertical subpixel anti-aliasing, designed for LCD BGR displays
};

// argument to GB_FontMake
enum GB_FontHintOptions {
    GB_HINT_DEFAULT = 0,  // default hinting algorithm is chosen.
    GB_HINT_FORCE_AUTO,  // always use the FreeType2 auto hinting algorithm
    GB_HINT_NO_AUTO,  // always use the fonts hinting algorithm
    GB_HINT_NONE  // use no hinting algorithm at all.
};

// font object
// reference counted
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

// filename - ttf or otf font
// point_size - pixels per em
// render_options - controls how anti-aliasing is preformed during glyph rendering.
// hint_pitons - controls which hinting algorithm is chosen during glyph rendering.
// reference count starts at 1, must release font objects to destroy them.
GB_ERROR GB_FontMake(struct GB_Context *gb, const char *filename, uint32_t point_size,
                     enum GB_FontRenderOptions render_options, enum GB_FontHintOptions hint_options,
                     struct GB_Font **font_out);

// reference count
GB_ERROR GB_FontRetain(struct GB_Context *gb, struct GB_Font *font);
GB_ERROR GB_FontRelease(struct GB_Context *gb, struct GB_Font *font);

// fills max_advance_out returns advance width in pixels
GB_ERROR GB_FontGetMaxAdvance(struct GB_Context *gb, struct GB_Font *font, uint32_t *max_advance_out);

// fills line_height_out with line height in pixels
GB_ERROR GB_FontGetLineHeight(struct GB_Context *gb, struct GB_Font *font, uint32_t *line_height_out);

#ifdef __cplusplus
}
#endif

#endif // GB_FONT_H
