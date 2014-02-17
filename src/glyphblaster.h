#ifndef GB_GLYPHBLASTER_H
#define GB_GLYPHBLASTER_H

#define GB_NO_COPY(TypeName)                        \
	TypeName(const TypeName&);						\
	TypeName& operator=(const TypeName&);

namespace gb {

enum TextureFormat { TextureFormat_Alpha = 0, TextureFormat_RGBA };

} // namespace gb

#endif // GB_GLYPHBLASTER_H
