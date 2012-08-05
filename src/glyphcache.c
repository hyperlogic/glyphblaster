
#ifdef __APPLE__
#  include "TargetConditionals.h"
#else
#  define TARGET_OS_IPHONE 0
#  define TARGET_IPHONE_SIMULATOR 0
#endif

#if defined DARWIN
#  include <OpenGL/gl.h>
#  include <OpenGL/glu.h>
#elif TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
#  include <OpenGLES/ES1/gl.h>
#  include <OpenGLES/ES1/glext.h>
#else
#  include <GL/gl.h>
#  include <GL/glext.h>
#  include <GL/glu.h>
#endif

#include "glyphcache.h"
#include <assert.h>

#ifndef NDEBUG
// If there is a glError this outputs it along with a message to stderr.
// otherwise there is no output.
void GLErrorCheck(const char* message)
{
    GLenum val = glGetError();
    switch (val)
    {
    case GL_INVALID_ENUM:
        fprintf(stderr, "GL_INVALID_ENUM : %s\n", message);
        break;
    case GL_INVALID_VALUE:
        fprintf(stderr, "GL_INVALID_VALUE : %s\n", message);
        break;
    case GL_INVALID_OPERATION:
        fprintf(stderr, "GL_INVALID_OPERATION : %s\n", message);
        break;
#ifndef GL_ES_VERSION_2_0
    case GL_STACK_OVERFLOW:
        fprintf(stderr, "GL_STACK_OVERFLOW : %s\n", message);
        break;
    case GL_STACK_UNDERFLOW:
        fprintf(stderr, "GL_STACK_UNDERFLOW : %s\n", message);
        break;
#endif
    case GL_OUT_OF_MEMORY:
        fprintf(stderr, "GL_OUT_OF_MEMORY : %s\n", message);
        break;
    case GL_NO_ERROR:
        break;
    }
}
#endif

static GB_ERROR _GB_InitOpenGLTexture(uint32_t* gl_tex_out)
{
    glGenTextures(1, gl_tex_out);
    glBindTexture(GL_TEXTURE_2D, *gl_tex_out);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    uint8_t* image = NULL;

#ifndef NDEBUG
    // in debug fill texture with 255.
    image = (uint8_t*)malloc(GB_TEXTURE_SIZE * GB_TEXTURE_SIZE * sizeof(uint8_t));
    memset(image, 255, GB_TEXTURE_SIZE * GB_TEXTURE_SIZE * sizeof(uint8_t));
#endif

    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, GB_TEXTURE_SIZE, GB_TEXTURE_SIZE, 0, GL_ALPHA, GL_UNSIGNED_BYTE, image);

#ifndef NDEBUG
    GLErrorCheck("_GB_InitOpenGLTexture");
    free(image);
#endif
    return GB_ERROR_NONE;
}

static GB_ERROR _GB_GlyphSheetInit(GB_GLYPH_SHEET* sheet)
{
    _GB_InitOpenGLTexture(&sheet->gl_tex);
    sheet->num_levels = 0;
    return GB_ERROR_NONE;
}

static int _GB_GlyphSheetAddNewLevel(GB_GLYPH_SHEET* sheet, uint32_t height)
{
    GB_GLYPH_SHEET_LEVEL* prev_level = (sheet->num_levels == 0) ? NULL : &sheet->level[sheet->num_levels - 1];
    if (sheet->num_levels < GB_MAX_LEVELS_PER_SHEET) {
        GB_GLYPH_SHEET_LEVEL* level = &sheet->level[sheet->num_levels++];
        memset(level, 0, sizeof(GB_GLYPH_SHEET_LEVEL));
        level->baseline = prev_level ? prev_level->baseline + prev_level->height : 0;
        level->height = height;
        level->num_glyphs = 0;
        return 1;
    } else {
        return 0;
    }
}

static void _GB_GlyphSheetSubloadGlyph(GB_GLYPH_SHEET* sheet, GB_GLYPH* glyph)
{
    assert(sheet);
    glBindTexture(GL_TEXTURE_2D, sheet->gl_tex);
    glTexSubImage2D(GL_TEXTURE_2D, 0, glyph->origin[0], glyph->origin[1],
                    glyph->size[0], glyph->size[1], GL_ALPHA, GL_UNSIGNED_BYTE, glyph->image);
#ifndef NDEBUG
    GLErrorCheck("_GB_GlyphSheetSubloadGlyph");
#endif
}

static int _GB_GlyphSheetLevelInsertGlyph(GB_GLYPH_SHEET_LEVEL* level, GB_GLYPH* glyph)
{
    GB_GLYPH* prev_glyph = level->num_glyphs == 0 ? NULL : level->glyph[level->num_glyphs - 1];

    if (level->num_glyphs < GB_MAX_GLYPHS_PER_LEVEL) {
        if (prev_glyph) {
            if (prev_glyph->origin[0] + prev_glyph->size[0] + glyph->size[0] <= GB_TEXTURE_SIZE) {

                // update glyph origin
                glyph->origin[0] = prev_glyph->origin[0] + prev_glyph->size[0];
                glyph->origin[1] = level->baseline;

                // add glyph to sheet
                level->glyph[level->num_glyphs++] = glyph;

                GB_GlyphReference(glyph);
                return 1;
            }
        } else {
            if (glyph->size[0] <= GB_TEXTURE_SIZE) {
                glyph->origin[0] = 0;
                glyph->origin[1] = level->baseline;

                // add glyph to sheet
                level->glyph[level->num_glyphs++] = glyph;
                return 1;
            }
        }
    }

    // no free space
    return 0;
}

static int _GB_GlyphSheetInsertGlyph(GB_GLYPH_CACHE* cache, GB_GLYPH_SHEET* sheet, GB_GLYPH* glyph)
{
    int i = 0;
    for (i = 0; i < sheet->num_levels; i++) {
        if (glyph->size[1] <= sheet->level[i].height) {
            if (_GB_GlyphSheetLevelInsertGlyph(&sheet->level[i], glyph)) {
                _GB_GlyphSheetSubloadGlyph(sheet, glyph);
                printf("AJT: Inserted glyph %d into existing level %d\n", glyph->index, i);
                return 1;
            }
        }
    }

    // add a new level.
    if (_GB_GlyphSheetAddNewLevel(sheet, glyph->size[1])) {
        if (_GB_GlyphSheetLevelInsertGlyph(&sheet->level[i], glyph)) {
            _GB_GlyphSheetSubloadGlyph(sheet, glyph);
            printf("AJT: Inserted glyph %d into new level %d\n", glyph->index, i);
            return 1;
        } else {
            printf("AJT: glyph %d is too wide\n", glyph->index);
            // glyph is wider then the texture?!?
            return 0;
        }
    } else {
        printf("AJT: glyph %d out of room\n", glyph->index);
        // out of room
        return 0;
    }
}

GB_ERROR GB_GlyphCacheMake(GB_GLYPH_CACHE** cache_out)
{
    *cache_out = (GB_GLYPH_CACHE*)malloc(sizeof(GB_GLYPH_CACHE));
    memset(*cache_out, 0, sizeof(GB_GLYPH_CACHE));

    _GB_GlyphSheetInit(&(*cache_out)->sheet[0]);
    (*cache_out)->num_sheets = 1;

    return GB_ERROR_NONE;
}

GB_ERROR GB_GlyphCacheFree(GB_GLYPH_CACHE* glyph_cache)
{
    if (glyph_cache) {
        // TODO:
        free(glyph_cache);
    }
    return GB_ERROR_NONE;
}

static int _GB_GlyphCacheInsertGlyph(GB_GLYPH_CACHE* cache, GB_GLYPH* glyph)
{
    int i;
    for (i = 0; i < cache->num_sheets; i++) {
        GB_GLYPH_SHEET* sheet = cache->sheet + i;
        if (_GB_GlyphSheetInsertGlyph(cache, sheet, glyph)) {
            return 1;
        }
    }
    return 0;
}

static int _GB_GlyphCacheAddSheet(GB_GLYPH_CACHE* cache)
{
    return 0;
}

static void _GB_GlyphCacheCompact(GB_GLYPH_CACHE* cache)
{
    // build array of all glyphs
    // sort them in decreasing height
    // rebuild!

    // make sure to clear out all glyphs that are no longer used by any text nodes.
    // using ref count or some such.
    return;
}

// sort glyph ptrs in decreasing height
static int glyph_cmp(const void* a, const void* b)
{
    return (*(GB_GLYPH**)b)->size[1] - (*(GB_GLYPH**)a)->size[1];
}

GB_ERROR GB_GlyphCacheInsert(GB_GLYPH_CACHE* cache, GB_GLYPH** glyph_ptrs, int num_glyph_ptrs)
{
    int i;

    // sort glyphs in decreasing height
    qsort(glyph_ptrs, num_glyph_ptrs, sizeof(GB_GLYPH*), glyph_cmp);

    // decreasing height find-first heuristic.
    for (i = 0; i < num_glyph_ptrs; i++) {
        printf("AJT: i = %d\n", i);
        GB_GLYPH* glyph = glyph_ptrs[i];
        if (!_GB_GlyphCacheInsertGlyph(cache, glyph)) {
            // compact and try again.
            _GB_GlyphCacheCompact(cache);
            if (!_GB_GlyphCacheInsertGlyph(cache, glyph)) {
                // Add another sheet and try again.
                if (_GB_GlyphCacheAddSheet(cache)) {
                    if (!_GB_GlyphCacheInsertGlyph(cache, glyph)) {
                        // glyph must be too big to fit in a single sheet.
                        assert(0);
                        return GB_ERROR_NOMEM;
                    }
                } else {
                    // failed to add sheet, no room
                    return GB_ERROR_NOMEM;
                }
            }
        }
    }
    return GB_ERROR_NONE;
}
