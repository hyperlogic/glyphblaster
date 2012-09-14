
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

#include "gb_texture.h"
#include "gb_context.h"
#include <stdio.h>

#ifndef NDEBUG
// If there is a glError this outputs it along with a message to stderr.
// otherwise there is no output.
void GLErrorCheck(const char *message)
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

GB_ERROR GB_TextureInit(enum GB_TextureFormat texture_format, uint32_t texture_size, uint8_t *image,
                        uint32_t *tex_out)
{
    glGenTextures(1, tex_out);
    glBindTexture(GL_TEXTURE_2D, *tex_out);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    if (texture_format == GB_TEXTURE_FORMAT_ALPHA)
        glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, texture_size, texture_size, 0, GL_ALPHA, GL_UNSIGNED_BYTE, image);
    else
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture_size, texture_size, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);

#ifndef NDEBUG
    GLErrorCheck("_GB_InitOpenGLTexture");
#endif

    return GB_ERROR_NONE;
}

GB_ERROR GB_TextureDestroy(uint32_t tex)
{
    glDeleteTextures(1, &tex);
    return GB_ERROR_NONE;
}


GB_ERROR GB_TextureSubLoad(uint32_t tex, uint32_t origin[2], uint32_t size[2], uint8_t *image)
{
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexSubImage2D(GL_TEXTURE_2D, 0, origin[0], origin[1], size[0], size[1], GL_ALPHA, GL_UNSIGNED_BYTE, image);
#ifndef NDEBUG
    GLErrorCheck("_GB_SheetSubloadGlyph");
#endif
    return GB_ERROR_NONE;
}
