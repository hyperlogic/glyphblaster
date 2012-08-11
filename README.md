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
* rename GB_GLYPH_CACHE to just cache:
* rename GB_GLYPH_SHEET to just GB_CACHE_SHEET & GB_GLYPH_SHEET_LEVEL to GB_CACHE_SHEET_LEVEL.
* consolidate all glyph refs into two hash tables:
  * gb->text_hash - which contains all glyphs in use by GB_TEXT structs
  * gb->cache->sheet_hash - contains all glyphs in use by texture sheets.
  * the KEY should be font_index & glyph_index. instead of just glyph_index.
* add glyph bitmap-padding option, necessary for scaled or non-screen aligned text.
* quad rendering with post hinted-advances.
* Finish SDL test prog.
* Text Alignment
* Text Word Wrapping
* currently two fonts with different pt sizes will have two copies of the same FreeType font.
  resources should be shared.


