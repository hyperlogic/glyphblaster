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
* Finish SDL test prog.
* Text Word Wrapping
* Text Alignment
* currently two fonts with different pt sizes will have two copies of the same FreeType font.
  resources should be shared.
* add glyph bitmap-padding option, necessary for scaled or non-screen aligned text.
* integrate bidi

NOTES:
----------------
* When cache is full, glyphs will use a fallback texture, which is 1/2 alpha.
* Bidi makes word-wrapping a pain.  do this after word wrapping/justification is functional.
* I still don't know how slow a full repack is. Benchmark it.
* I'm not sure if the interface is very good.
  Text's are not mutable, they must be destoryed and re-created.
* Should I add a direction parameter? to Text or laungage? script?
* Should I replace ut headers? with hand coded stuff?
