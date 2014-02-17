#ifndef GB_TEXTURE
#define GB_TEXTURE

#include <stdint.h>
#include "glyphblaster.h"

namespace gb {

enum TextureFormat { TextureFormat_Alpha = 0, TextureFormat_RGBA };

class Texture
{
public:
    Texture(TextureFormat format, uint32_t texture_size, uint8_t* image)
    ~Texture();
    uint32_t GetGLTexObj() const { return m_texObj; }
    Subload(IntPoint origin, IntPoint size, uint8_t* image);
protected:
    uint32_t m_texObj;
};

} // namespace gb

#endif // GB_TEXTURE_H
