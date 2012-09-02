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
* Adjust style to be more Linuxy. (mostly adjust decls to look like this: int *foo)
* Text Alignment
* Text Word Wrapping
* currently two fonts with different pt sizes will have two copies of the same FreeType font.
  resources should be shared.
* add glyph bitmap-padding option, necessary for scaled or non-screen aligned text.
* bidi
* make texture sheet size configurable.
* make max num sheets configurable.

NOTES:
----------------
* When cache is full, glyphs will use a fallback texture, which is 1/2 alpha.
* Bidi makes word-wrapping a pain.  maybe drop it?
* I still don't know how slow a full repack is.
* I'm not sure if the interface is very good.
  Text's are not mutable, they must be destoryed and re-created.
* Should I add a direction parameter? to Text or laungage? script?
* I'm not sure if multi-sheet caching works.
* Should I replace ut headers? with hand coded stuff?
