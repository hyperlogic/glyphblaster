#ifndef GB_TEXTURE
#define GB_TEXTURE

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "gb_error.h"
#include "gb_context.h"

// TODO: make into class?

GB_ERROR GB_TextureInit(enum GB_TextureFormat format, uint32_t texture_size, uint8_t *image,uint32_t *tex_out);
GB_ERROR GB_TextureDestroy(uint32_t tex);
GB_ERROR GB_TextureSubLoad(uint32_t tex, enum GB_TextureFormat format, uint32_t origin[2], uint32_t size[2],
                           uint8_t *image);

#ifdef __cplusplus
}
#endif

#endif // GB_TEXTURE_H
