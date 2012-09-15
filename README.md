Glyph Blaster
---------------

Things:
  * C API
  * Pluggable render function.  Have pluggable texture creation & sub-loading later.
  * Limited Wrapping support, for now
  * Use HarfBuzz for glyph shaping.
  * Manages OpenGL resources
  * Glyph packer
  * FreeType is used for rasterization.
  * utf8

Dependencies
-----------------
  * icu4c
  * FreeType2
  * HarfBuzz-0.9.0

TODO:
-----------------
* test empty string & whitespace strings.
* assertion and more graceful failure when static buffers overflow.
* Test support for LCD subpixel rendering/ what should the alpha be? what should the blend mode be?
* Enable sRGB aware blending, during rendering. (if available)
* Add support for GB-RENDER-MONO, unpack bits
* Justify-Horizontal: left, center, right
* Justify-Vertical: top, center, bottom
* Justify: Scale to fit
* Right-to-Left text
* Replace uthash
* Bundle harf-buzz, to make building easier
* Minimize/control use of icu4c, might have to implement some stuff for harf-buzz as well.
* currently two fonts with different pt sizes will have two copies of the same FreeType font.
  resources should be shared.
* add glyph bitmap-padding option, necessary for scaled or non-screen aligned text.
* integrate bidi
* Finish SDL test prog.

NOTES:
----------------
* When cache is full, glyphs will use a fallback texture, which is 1/2 alpha.
* Bidi makes word-wrapping a pain.  do this after word wrapping/justification is functional.
* I still don't know how slow a full repack is. Benchmark it.
* I'm not sure if the interface is very good.
  Text's are not mutable, they must be destoryed and re-created.
* Should I add a direction parameter? to Text or laungage? script?
* Should I replace ut headers? with hand coded stuff?

First steps toward word wrapping..
-------------------------------------

Do i need to do two passes?  i.e. one to generate a new set of runs.
and another to do the justification. of the runs. by offsetting everything in x or y.

1) keeping track of word_start
2) keeping running total of line length. (length from text.size[0])
3) checking when gone past length.
4) when this occurs reset the pen. as-if there was a new line.
5) do justification as a post process?!? i guess I have to build a new run array.
