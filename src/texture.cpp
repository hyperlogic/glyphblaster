
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

namespace gb {

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

Texture::Texture(TextureFormat format, uint32_t textureSize, uint8_t* image)
{
    glGenTextures(1, &m_texObj);
    glBindTexture(GL_TEXTURE_2D, m_texObj);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    if (format == TextureFormat_Alpha)
        glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, textureSize, textureSize, 0, GL_ALPHA, GL_UNSIGNED_BYTE, image);
    else
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, textureSize, textureSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);

#ifndef NDEBUG
    GLErrorCheck("Texture::Texture");
#endif
}

Texture::~Texture()
{
    glDeleteTextures(1, &m_texObj);

#ifndef NDEBUG
    GLErrorCheck("Texture::~Texture");
#endif
}

void Texture::Subload(IntPoint origin, IntPoint size, uint8_t* image)
{
    glBindTexture(GL_TEXTURE_2D, m_texObj);
    if (format == TextureFormat_Alpha)
        glTexSubImage2D(GL_TEXTURE_2D, 0, origin.x, origin.y, size.x, size.y, GL_ALPHA, GL_UNSIGNED_BYTE, image);
    else
        glTexSubImage2D(GL_TEXTURE_2D, 0, origin.x, origin.y, size.x, size.y, GL_RGBA, GL_UNSIGNED_BYTE, image);

#ifndef NDEBUG
    GLErrorCheck("Texture::Subload");
#endif
    return GB_ERROR_NONE;
}

} // namespace gb
