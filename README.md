# Glyph Blaster

## WIP C++11 Port

Justifications.

* Standard smart pointers that will make the memory management of resources less error prone.
* can drop dependency on uthash and utlist, and use standard collections
* lamdas provide a better mechanism for rendering hooks then c function ptrs.
* all of my personal projects are in C++, C linkage is not important to me.
* shared lib version of library is also not important to me.
* Interop with other languages is not important to me.

### Design questions

Should Context be a singleton? YES. simplifies things, i can sort this out later if necessary.

### Proposed Architecture Diagram

![Image](docs/glyphblaster.png)

The Context holds a weak collection of all gyphs ever created.
Each Text object holds a collection of glyphs it is activly using.

## Features

* Pluggable render function, to integrate into existing engines.
* Uses HarfBuzz for glyph shaping for ligatures & arabic languages.
* FreeType is used for rasterization, after shaping.
* Manages glyph bitmaps in a tightly packed set of OpenGL textures.
* utf8 support
* rtl language support (arabic & hebrew)

## Dependencies

* FreeType2
* HarfBuzz-0.9

## Implementation Notes

* When cache is full, glyphs will use a fallback texture, which is 1/2 alpha.
* When glyph is not present in the font, the replacement character is used. �
* Bidi text is not supported, it makes word-wrapping a pain.
* I still don't know how slow a full repack is. Benchmark it.
* I'm not sure if the interface is very good.
  * Text's are not mutable, they must be destroyed and re-created.
  * No metrics available.
  * The metrics should be good enough to perform custom word-wrapping, bidi, underline & html styles
    at a higher level.

## TODO:

### Dependency Build Tasks

* compile option to make dependency on HarfBuzz optional
* Bundle harf-buzz, to make building easier

### Implementation Tasks

* Split MakeGlyphQuadRuns into two parts.
  * Build metrics/do word wrapping.
  * Build quads from metrics.
* Group quads that share the same texture into a single mesh.
* Add metrics function to text.
* Add ability to set pen position.
* Test support of LCD subpixel decimated RGB using shader and GL_COLOR_MASK
* Enable sRGB aware blending, during rendering. (if available) provide a sample renderer
* Justify-Vertical: top, center, bottom
* Justify: Scale to fit
* currently two fonts with different pt sizes will have two copies of the same FreeType font.
  resources should be shared.
* add glyph bitmap-padding option, necessary for scaled or non-screen aligned text.
* Currently mipmapping on glyph texture is disabled.
* bidi
* Better SDL test prog.
* Add pluggable texture creation & subload functions, for Direct3D renderers.

