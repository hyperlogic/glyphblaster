#ifndef GB_TEXTURE
#define GB_TEXTURE

#include <stdint.h>
#include "glyphblaster.h"

namespace gb {

class Texture
{
public:
    Texture(TextureFormat format, uint32_t texture_size, uint8_t* image);
    ~Texture();
    uint32_t GetTexObj() const { return m_texObj; }
    void Subload(IntPoint origin, IntPoint size, uint8_t* image);
    void GenerateMipmap() const;
protected:
    uint32_t m_texObj;
    TextureFormat m_format;
    mutable bool m_mipDirty;
};

} // namespace gb

#endif // GB_TEXTURE_H
