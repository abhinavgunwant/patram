#ifndef PATRAM_SYS_CHARACTER
#define PATRAM_SYS_CHARACTER

#include <cinttypes>

/// Holds all state information relevant to a character as loaded using FreeType
struct Character {
    uint8_t width;
    uint8_t height;
    int8_t bearingLeft;
    uint8_t bearingTop;
    uint16_t textureID; // ID handle of the glyph texture
    uint16_t advance;   // Horizontal offset to advance to next glyph

    float textureX1;
    float textureY1;
    float textureX2;
    float textureY2;
};

struct Char {
    uint16_t charcode;
    uint8_t width;
    uint8_t height;
    uint8_t* bitmap;
};

#endif // PATRAM_SYS_CHARACTER

