
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
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, GB_TEXTURE_SIZE, GB_TEXTURE_SIZE, 0, GL_ALPHA, GL_UNSIGNED_INT, 0);
#ifndef NDEBUG
    GLErrorCheck("_GB_InitOpenGLTexture");
#endif
    return GB_ERROR_NONE;
}

static GB_ERROR _GB_GlyphSheet_Init(GB_GLYPH_SHEET* sheet)
{
    _GB_InitOpenGLTexture(&sheet->gl_tex);
    sheet->num_levels = 0;
    return GB_ERROR_NONE;
}

/*
static int _GB_GlyphSheet_TryToAddNewLevel(GB_GLYPH_SHEET* sheet, uint32_t height)
{
    // TODO:
    return 0;
}

static void _GB_GlyphSheet_SubloadGlyph(GB_GLYPH_SHEET* sheet, GB_GLYPH* glyph)
{
    return;
}

static void _GB_GlyphCache_InsertIntoHash(GB_GLYPH_CACHE* cache, GB_GLYPH* glyph)
{

}
*/

static int _GB_GlyphSheet_TryToInsertGlyph(GB_GLYPH_CACHE* cache, GB_GLYPH_SHEET* sheet, GB_GLYPH* glyph)
{
    /*
    int i = 0;
    for (i = 0; i < sheet->num_levels; i++) {
        if (glyph->size_y <= sheet->level[i].height) {
            int last_glyph_index = sheet->level[i].num_glyphs - 1;
            if (last_glyph_index < GB_MAX_GLYPHS_PER_LEVEL) {
                GB_GLYPH* last_glyph = sheet->level[i].glyph[last_glyph_index];
                if (last_glyph->origin_x + last_glyph->size_x + glyph->size_x <= GB_TEXTURE_SIZE) {

                    // update glyph origin
                    glyph->origin_x = last_glyph->origin_x + last_glyph->size_x;
                    glyph->origin_y = last_glyph->origin_y;

                    // add glyph to sheet
                    sheet->level[i].glyph[sheet->level[i].num_glyphs++] = glyph;

                    // subload into opengl texture
                    _GB_GlyphSheet_SubloadGlyph(sheet, glyph);

                    // add to glyph to hash table
                    _GB_GlyphCache_InsertIntoHash(cache, glyph);

                    return 1;
                }
            }
        }
    }

    // TODO: add a new level.
    if (_GB_GlyphSheet_TryToAddNewLevel(sheet, glyph->size_y)) {

        // update glyph origin
        glyph->origin_x = 0;
        glyph->origin_y = sheet->level[i].baseline;

        // add glyph to new sheet
        sheet->level[i].glyph[sheet->level[i].num_glyphs++] = glyph;

        // subload into opengl texture
        _GB_GlyphSheet_SubloadGlyph(sheet, glyph);

        // add to glyph to hash table
        _GB_GlyphCache_InsertIntoHash(cache, glyph);

        return 1;

    } else {
        // out of room
        return 0;
    }
    */
    return 0;
}

GB_ERROR GB_GlyphCache_Make(GB_GLYPH_CACHE** cache_out)
{
    *cache_out = (GB_GLYPH_CACHE*)malloc(sizeof(GB_GLYPH_CACHE));
    memset(*cache_out, 0, sizeof(GB_GLYPH_CACHE));

    _GB_GlyphSheet_Init(&(*cache_out)->sheet[0]);
    (*cache_out)->num_sheets = 1;

    return GB_ERROR_NONE;
}

GB_ERROR GB_GlyphCache_Free(GB_GLYPH_CACHE* glyph_cache)
{
    if (glyph_cache) {
        // TODO:
        free(glyph_cache);
    }
    return GB_ERROR_NONE;
}

GB_GLYPH* GB_GlyphCache_Find(GB_GLYPH_CACHE* glyph_cache, uint32_t index, uint32_t font_index)
{
    // TODO:
    return NULL;
}

static int _GB_GlyphCache_TryToInsertGlyph(GB_GLYPH_CACHE* cache, GB_GLYPH* glyph)
{
    int i;
    for (i = 0; i < cache->num_sheets; i++) {
        GB_GLYPH_SHEET* sheet = cache->sheet + i;
        if (_GB_GlyphSheet_TryToInsertGlyph(cache, sheet, glyph)) {
            return 1;
        }
    }
    return 0;
}

static int _GB_GlyphCache_AddSheet(GB_GLYPH_CACHE* cache)
{
    return 0;
}

static void _GB_GlyphCache_Compact(GB_GLYPH_CACHE* cache)
{
    // build array of all glyphs
    // sort them in decreasing height
    // clear cache
    // rebuild!
    return;
}

// sort glyph ptrs in decreasing height
static int glyph_cmp(const void* a, const void* b)
{
    return ((GB_GLYPH*)b)->size[1] - ((GB_GLYPH*)a)->size[1];
}

GB_ERROR GB_GlyphCache_Insert(GB_GLYPH_CACHE* cache, GB_GLYPH** glyph_ptrs, int num_glyph_ptrs)
{
    int i;

    // sort them in decreasing height
    qsort(glyph_ptrs, num_glyph_ptrs, sizeof(GB_GLYPH*), glyph_cmp);

    // find first decreasing height heuristic.
    for (i = 0; i < num_glyph_ptrs; i++) {
        GB_GLYPH* glyph = glyph_ptrs[i];
        if (!_GB_GlyphCache_TryToInsertGlyph(cache, glyph)) {
            // compact and try again.
            _GB_GlyphCache_Compact(cache);
            if (!_GB_GlyphCache_TryToInsertGlyph(cache, glyph)) {
                // Add another sheet and try again.
                if (_GB_GlyphCache_AddSheet(cache)) {
                    if (!_GB_GlyphCache_TryToInsertGlyph(cache, glyph)) {
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
