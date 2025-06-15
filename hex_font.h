#ifndef HEX_FONT_H
#define HEX_FONT_H
#include <cstdint>

#define HEX_FONT_GLYPHS 16
#define HEX_FONT_WIDTH  32
#define HEX_FONT_HEIGHT 32

// Build a 16-glyph atlas from the Consolas TrueType font. The atlas array
// is laid out row-major with `HEX_FONT_GLYPHS` glyphs each of size
// `HEX_FONT_WIDTH` by `HEX_FONT_HEIGHT`. Each pixel is an 8-bit alpha value.
// Returns false if the font cannot be loaded.
bool BuildConsolasHexAtlas(unsigned char atlas[HEX_FONT_HEIGHT][HEX_FONT_WIDTH * HEX_FONT_GLYPHS]);

#endif
