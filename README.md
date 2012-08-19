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
  * FreeType2
  * HarfBuzz-0.9.0

TODO:
-----------------
* make sure cache deletes opengl texture on destroy.
* test empty string & whitespace strings.
* Finish SDL test prog.
* Adjust style to be more Linuxy. (mostly adjust decls to look like this: int *foo)
* Text Alignment
* Text Word Wrapping
* currently two fonts with different pt sizes will have two copies of the same FreeType font.
  resources should be shared.
* add glyph bitmap-padding option, necessary for scaled or non-screen aligned text.
* bidi



