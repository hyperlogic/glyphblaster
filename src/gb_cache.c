
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

#include "utlist.h"
#include <assert.h>
#include "gb_context.h"
#include "gb_glyph.h"
#include "gb_font.h"
#include "gb_cache.h"

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

static GB_ERROR _GB_InitOpenGLTexture(uint32_t texture_size, uint32_t* gl_tex_out)
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
    image = (uint8_t*)malloc(texture_size * texture_size * sizeof(uint8_t));
    memset(image, 255, texture_size * texture_size * sizeof(uint8_t));
#endif

    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, texture_size, texture_size, 0,
                 GL_ALPHA, GL_UNSIGNED_BYTE, image);

#ifndef NDEBUG
    GLErrorCheck("_GB_InitOpenGLTexture");
    free(image);
#endif
    return GB_ERROR_NONE;
}

static GB_ERROR _GB_SheetInit(struct GB_Cache* cache, struct GB_Sheet* sheet)
{
    _GB_InitOpenGLTexture(cache->texture_size, &sheet->gl_tex_obj);
    sheet->num_levels = 0;
    return GB_ERROR_NONE;
}

static int _GB_SheetAddNewLevel(struct GB_Cache* cache, struct GB_Sheet* sheet, uint32_t height)
{
    struct GB_SheetLevel* prev_level = (sheet->num_levels == 0) ? NULL : &sheet->level[sheet->num_levels - 1];
    uint32_t baseline = prev_level ? prev_level->baseline + prev_level->height : 0;
    if (sheet->num_levels < GB_MAX_LEVELS_PER_SHEET && (baseline + height) <= cache->texture_size) {
        struct GB_SheetLevel* level = &sheet->level[sheet->num_levels++];
        memset(level, 0, sizeof(struct GB_SheetLevel));
        level->baseline = baseline;
        level->height = height;
        level->num_glyphs = 0;
        return 1;
    } else {
        return 0;
    }
}

static void _GB_SheetSubloadGlyph(struct GB_Sheet* sheet, struct GB_Glyph* glyph)
{
    assert(sheet);
    glBindTexture(GL_TEXTURE_2D, sheet->gl_tex_obj);
    glTexSubImage2D(GL_TEXTURE_2D, 0, glyph->origin[0], glyph->origin[1],
                    glyph->size[0], glyph->size[1], GL_ALPHA, GL_UNSIGNED_BYTE, glyph->image);
#ifndef NDEBUG
    GLErrorCheck("_GB_SheetSubloadGlyph");
#endif
}

static int _GB_SheetLevelInsertGlyph(struct GB_Cache* cache, struct GB_SheetLevel* level,
                                     struct GB_Glyph* glyph)
{
    struct GB_Glyph* prev_glyph = level->num_glyphs == 0 ? NULL : level->glyph[level->num_glyphs - 1];

    if (level->num_glyphs < GB_MAX_GLYPHS_PER_LEVEL) {
        if (prev_glyph) {
            if (prev_glyph->origin[0] + prev_glyph->size[0] + glyph->size[0] <= cache->texture_size) {

                // update glyph origin
                glyph->origin[0] = prev_glyph->origin[0] + prev_glyph->size[0];
                glyph->origin[1] = level->baseline;

                // add glyph to sheet
                level->glyph[level->num_glyphs++] = glyph;
                return 1;
            }
        } else {
            if (glyph->size[0] <= cache->texture_size) {
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

static int _GB_SheetInsertGlyph(struct GB_Cache* cache, struct GB_Sheet* sheet, struct GB_Glyph* glyph)
{
    int i = 0;
    for (i = 0; i < sheet->num_levels; i++) {
        if (glyph->size[1] <= sheet->level[i].height) {
            if (_GB_SheetLevelInsertGlyph(cache, &sheet->level[i], glyph)) {
                glyph->gl_tex_obj = sheet->gl_tex_obj;
                _GB_SheetSubloadGlyph(sheet, glyph);
                printf("AJT: Inserted glyph %d into existing level %d\n", glyph->index, i);
                return 1;
            }
        }
    }

    // add a new level.
    if (_GB_SheetAddNewLevel(cache, sheet, glyph->size[1])) {
        if (_GB_SheetLevelInsertGlyph(cache, &sheet->level[i], glyph)) {
            glyph->gl_tex_obj = sheet->gl_tex_obj;
            _GB_SheetSubloadGlyph(sheet, glyph);
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

GB_ERROR GB_CacheMake(uint32_t texture_size, uint32_t num_sheets, struct GB_Cache** cache_out)
{
    struct GB_Cache* cache = (struct GB_Cache*)malloc(sizeof(struct GB_Cache));
    memset(cache, 0, sizeof(struct GB_Cache));

    cache->num_sheets = num_sheets;
    cache->texture_size = texture_size;
    cache->glyph_hash = NULL;

    uint32_t i;
    for (i = 0; i < num_sheets; i++)
        _GB_SheetInit(cache, &cache->sheet[i]);

    *cache_out = cache;
    return GB_ERROR_NONE;
}

GB_ERROR GB_CacheDestroy(struct GB_Cache* cache)
{
    if (cache) {
        // release all glyphs
        struct GB_Glyph *glyph, *tmp;
        HASH_ITER(cache_hh, cache->glyph_hash, glyph, tmp) {
            HASH_DELETE(cache_hh, cache->glyph_hash, glyph);
            GB_GlyphRelease(glyph);
        }
        free(cache);
    }
    return GB_ERROR_NONE;
}

static int _GB_CacheInsertGlyph(struct GB_Cache* cache, struct GB_Glyph* glyph)
{
    int i;
    for (i = 0; i < cache->num_sheets; i++) {
        struct GB_Sheet* sheet = cache->sheet + i;
        if (_GB_SheetInsertGlyph(cache, sheet, glyph)) {
            return 1;
        }
    }
    return 0;
}

static int _GB_CacheAddSheet(struct GB_Cache* cache)
{
    // TODO:
    return 0;
}

// sort glyph ptrs in decreasing height
static int glyph_cmp(const void* a, const void* b)
{
    return (*(struct GB_Glyph**)b)->size[1] - (*(struct GB_Glyph**)a)->size[1];
}

GB_ERROR GB_CacheCompact(struct GB_Context* gb, struct GB_Cache* cache)
{
    printf("AJT: COMPACT!!!!!\n");

    uint32_t num_glyph_ptrs;
    struct GB_Glyph** glyph_ptrs = GB_ContextHashValues(gb, &num_glyph_ptrs);

    // The goal of all this retaining & releasing is to make sure we dispose of all
    // glyphs that are in the cache but aren't in the context.
    // i.e. clear out all the unused glyphs.

    // retain all glyphs in the context
    int i;
    for (i = 0; i < num_glyph_ptrs; i++) {
        GB_GlyphRetain(glyph_ptrs[i]);
    }

    // release all glyphs in the cache
    struct GB_Glyph* glyph;
    for (glyph = cache->glyph_hash; glyph != NULL; glyph = glyph->cache_hh.next) {
        GB_GlyphRelease(glyph);
    }
    HASH_CLEAR(cache_hh, cache->glyph_hash);

    // Clear all sheets
    for (i = 0; i < cache->num_sheets; i++) {
        struct GB_Sheet* sheet = &cache->sheet[i];
        sheet->num_levels = 0;
    }

    // sort glyphs in decreasing height
    qsort(glyph_ptrs, num_glyph_ptrs, sizeof(struct GB_Glyph*), glyph_cmp);

    // decreasing height find-first heuristic.
    for (i = 0; i < num_glyph_ptrs; i++) {
        struct GB_Glyph* glyph = glyph_ptrs[i];
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

GB_ERROR GB_CacheInsert(struct GB_Context* gb, struct GB_Cache* cache,
                        struct GB_Glyph** glyph_ptrs, int num_glyph_ptrs)
{
    int i;

    // sort glyphs in decreasing height
    qsort(glyph_ptrs, num_glyph_ptrs, sizeof(struct GB_Glyph*), glyph_cmp);

    // decreasing height find-first heuristic.
    for (i = 0; i < num_glyph_ptrs; i++) {
        struct GB_Glyph* glyph = glyph_ptrs[i];
        // make sure duplicates don't end up in the hash
        if (!GB_CacheHashFind(cache, glyph->index, glyph->font_index)) {
            if (!_GB_CacheInsertGlyph(cache, glyph)) {
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
                            return GB_ERROR_NOMEM;
                        }
                    }
                } else {
                    return error;
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

void GB_CacheHashAdd(struct GB_Cache* cache, struct GB_Glyph* glyph)
{
#ifndef NDEBUG
    if (GB_CacheHashFind(cache, glyph->index, glyph->font_index)) {
        printf("GB_CacheHashAdd() WARNING glyph index = %d, font_index = %d is already in cache!\n", glyph->index, glyph->font_index);
    }
#endif

    printf("GB_CacheHashAdd() index = %d, font_index = %d\n", glyph->index, glyph->font_index);
    HASH_ADD(cache_hh, cache->glyph_hash, key, sizeof(uint64_t), glyph);
    GB_GlyphRetain(glyph);
}

struct GB_Glyph* GB_CacheHashFind(struct GB_Cache* cache, uint32_t glyph_index, uint32_t font_index)
{
    struct GB_Glyph* glyph = NULL;
    uint64_t key = ((uint64_t)font_index << 32) | glyph_index;
    HASH_FIND(cache_hh, cache->glyph_hash, &key, sizeof(uint64_t), glyph);
    return glyph;
}
