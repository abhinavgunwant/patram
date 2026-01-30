/**
 * Inspired by: https://learnopengl.com/In-Practice/Text-Rendering
 **/

#ifndef PATRAM_TEXT
#define PATRAM_TEXT

#include <string>

#include <glm/glm.hpp>
#include <ft2build.h>
#include FT_FREETYPE_H

#include "ui.hpp"

#define MAX_CHARACTERS 65536
#define DEFAULT_FONT_SIZE 48
#define ATLAS_DIMENSION 1024
// #define DEBUG_TEXT

#ifdef DEBUG_TEXT
extern bool debugTextFirstRenderPrint;
#endif

using namespace std;

/// Holds all state information relevant to a character as loaded using FreeType
struct Character {
    uint8_t width;
    uint8_t height;
    int8_t bearingLeft;
    uint8_t bearingTop;
	uint16_t textureID; // ID handle of the glyph texture
	uint16_t advance;   // Horizontal offset to advance to next glyph
};

struct Char {
    uint16_t charcode;
    uint8_t width;
    uint8_t height;
    uint8_t* bitmap;
};

struct GlyphDimensions {
    /** x dimension of the left of glyph */
    float leftX; // xpos

    /** x dimension of the right of glyph */
    float rightX; // xpos_plus_w

    /** x dimension of the bottom of glyph */
    float bottomY; // ypos

    /** x dimension of the top of glyph */
    float topY; // ypos_plus_h

    float scale; // Font Height Ratio

	uint16_t advance; // Same as in Character
	uint16_t textureID; // Same as in Character
};

extern Character characters[MAX_CHARACTERS];
extern unsigned int textVAO, textVBO;

void initCharGlyph(FT_Face& face, uint16_t c);
void initFonts();
void initTextRendering(uint16_t width, uint16_t height);

/**
 * Renders text
 *
 * Returns glm::vec4(top, left, bottom, right)
 */
glm::vec4 renderText(
    string text,
    glm::vec2 pos,
    glm::vec2& res,
    glm::fvec3 color = glm::fvec3(1.0, 1.0, 1.0),
    uint8_t fontSize = 16,
    UIElement* parent = nullptr
);

glm::vec4 renderText(
    wstring text,
    glm::vec2 pos,
    glm::vec2& res,
    glm::fvec3 color = glm::fvec3(1.0, 1.0, 1.0),
    uint8_t fontSize = 16,
    UIElement* parent = nullptr
);

GlyphDimensions getGlyphDimensions(char c, float x, float y, glm::vec2& res, uint8_t fontSize);
uint32_t renderGlyph(char c, float x, float y, glm::vec2& res, uint8_t fontSize);

#endif // PATRAM_TEXT

