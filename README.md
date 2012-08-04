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
* glyphcache & glyph packing
* quad rendering with post hinted-advances.
* Finish SDL test prog.
* Alignment
* Word Wrapping
* currently two fonts with different pt sizes will have two copies of the same FreeType font.
  resources should be shared.
* Do i need to store post hinted-advances?
