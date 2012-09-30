Glyph Blaster
===============

Features
---------------
  * C API
  * Pluggable render function, to integrate into existing engines.
  * Uses HarfBuzz for glyph shaping for liguatures & arabic languages.
  * FreeType is used for rasterization, after shaping.
  * Manages glyph bitmaps in a tightly packed set of OpenGL textures.
  * utf8 support
  * rtl language support (arabic & hebrew)

Dependencies
-----------------
  * icu4c
  * FreeType2
  * HarfBuzz-0.9.0

TODO:
-----------------
* assertion and more graceful failure when static buffers overflow.
* Test support of LCD subpixel decimated RGB using shader and GL_COLOR_MASK
* Enable sRGB aware blending, during rendering. (if available)
* Justify-Vertical: top, center, bottom
* Justify: Scale to fit
* Replace uthash
* Bundle harf-buzz, to make building easier
* Minimize/control use of icu4c, might have to implement some stuff for harf-buzz as well.
* currently two fonts with different pt sizes will have two copies of the same FreeType font.
  resources should be shared.
* add glyph bitmap-padding option, necessary for scaled or non-screen aligned text.
* bidi
* Better SDL test prog.
* Add pluggable texture creation & subload functions, for Direct3D renderers.

NOTES:
----------------
* When cache is full, glyphs will use a fallback texture, which is 1/2 alpha.
* When glyph is not present in the font, the replacement character is used. �
* Bidi makes word-wrapping a pain.  do this after word wrapping/justification is functional for rtl & ltr text.
* I still don't know how slow a full repack is. Benchmark it.
* I'm not sure if the interface is very good.
  * Text's are not mutable, they must be destroyed and re-created.
  * No metrics available.
  * The metrics should be good enough to perform custom word-wrapping, bidi & html styles
    at a higher level.

