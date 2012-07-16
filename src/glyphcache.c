
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

static GB_ERROR _GB_InitGlyphSheet(GB_GLYPH_SHEET* glyph_sheet)
{
    _GB_InitOpenGLTexture(&glyph_sheet->gl_tex);
    glyph_sheet->num_glyphs = 0;
    return GB_ERROR_NONE;
}

static GB_ERROR _GB_TryToInsertIntoGlyphSheet(GB_GLYPH_SHEET* glyph_sheet, GB_GLYPH* glyph, uint8_t* image, int* success_out)
{
    /*
    *success_out = 0;
    return GB_ERROR_NONE;
    */
}

GB_ERROR GB_MakeGlyphCache(GB_GLYPH_CACHE** glyph_cache_out)
{
    *glyph_cache_out = (GB_GLYPH_CACHE*)malloc(sizeof(GB_GLYPH_CACHE));
    memset(*glyph_cache_out, 0, sizeof(GB_GLYPH_CACHE));

    _GB_InitGlyphSheet(&(*glyph_cache_out)->sheet[0]);
    (*glyph_cache_out)->num_sheets = 1;

    return GB_ERROR_NONE;
}

GB_ERROR GB_DestroyGlyphCache(GB_GLYPH_CACHE* glyph_cache)
{
    if (glyph_cache) {
    }
    return GB_ERROR_NONE;
}

GB_ERROR GB_FindInGlyphCache(GB_GLYPH_CACHE* glyph_cache, uint32_t index, uint32_t font_index, uint32_t* gl_tex_out, GB_GLYPH** glyph_out)
{
    // not found
    *gl_tex_out = 0;
    *glyph_out = NULL;
    return GB_ERROR_NONE;
}

GB_ERROR GB_InsertIntoGlyphCache(GB_GLYPH_CACHE* glyph_cache, GB_GLYPH* glyph, uint8_t* image)
{
#ifndef NDEBUG
    uint32_t temp_gl_tex;
    GB_GLYPH* temp_glyph;
    GB_FindInGlyphCache(glyph_cache, glyph->index, glyph->font_index, &temp_gl_tex, &temp_glyph);
    assert(temp_gl_tex == 0 && temp_glyph == NULL);
#endif
    uint32_t i;
    for (i = 0; i < glyph_cache->num_sheets; i++) {
        int success = 0;
        GB_ERROR err = _GB_TryToInsertIntoGlyphSheet(&glyph_cache->sheet[i], glyph, image, &success);
        if (err != GB_ERROR_NONE) {
            return err;
        }
        if (success) {
            // success
            return GB_ERROR_NONE;
        }
    }
    return GB_ERROR_NOMEM;
}
