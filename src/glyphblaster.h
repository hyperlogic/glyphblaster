#ifndef GB_GLYPHBLASTER_H
#define GB_GLYPHBLASTER_H

#define GB_NO_COPY(TypeName)                        \
	TypeName(const TypeName&);						\
	TypeName& operator=(const TypeName&);

namespace gb {

struct Point<type T>
{
    T x, y;
};

typedef Point<int> IntPoint;
typedef Point<float> FloatPoint;

} // namespace gb

#endif // GB_GLYPHBLASTER_H
