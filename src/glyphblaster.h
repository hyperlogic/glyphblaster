#ifndef GB_GLYPHBLASTER_H
#define GB_GLYPHBLASTER_H

#define GB_NO_COPY(TypeName)                        \
	TypeName(const TypeName&);						\
	TypeName& operator=(const TypeName&);

namespace gb {

template <typename T>
struct Point
{
    Point(T _x, T _y) : x(_x), y(_y) {}
    T x, y;
};

typedef Point<int> IntPoint;
typedef Point<float> FloatPoint;

// y axis points down
// origin is upper-left corner of glyph
struct Quad
{
    IntPoint pen;
    IntPoint origin;
    IntPoint size;
    FloatPoint uvOrigin;
    FloatPoint uvSize;
    void* userData;
    uint32_t glTexObj;
};

enum TextureFormat { TextureFormat_Alpha = 0, TextureFormat_RGBA };

enum FontRenderOption {
    FontRenderOption_Normal = 0,  // normal anti-aliased font rendering
    FontRenderOption_Light,  // lighter anti-aliased outline hinting, this will force auto hinting.
    FontRenderOption_Mono,  // no anti-aliasing
    FontRenderOption_LCD_RGB,  // subpixel anti-aliasing, designed for LCD RGB displays
    FontRenderOption_LCD_BGR,  // subpixel anti-aliasing, designed for LCD BGR displays
    FontRenderOption_LCD_RGB_V,  // vertical subpixel anti-aliasing, designed for LCD RGB displays
    FontRenderOption_LCD_BGR_V   // vertical subpixel anti-aliasing, designed for LCD BGR displays
};

enum FontHintOption {
    FontHintOption_Default = 0,  // default hinting algorithm is chosen.
    FontHintOption_ForceAuto,  // always use the FreeType2 auto hinting algorithm
    FontHintOption_NoAuto,  // always use the fonts hinting algorithm
    FontHintOption_None  // use no hinting algorithm at all.
};

} // namespace gb

#endif // GB_GLYPHBLASTER_H
