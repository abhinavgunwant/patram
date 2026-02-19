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
#include "files.hpp"

#define MAX_CHARACTERS              65535
#define DEFAULT_FONT_SIZE           48
#define ATLAS_DIMENSION             1024
// #define DEBUG_TEXT

#ifdef DEBUG_TEXT
extern bool debugTextFirstRenderPrint;
#endif

#define ASCII_START_CHAR            32
#define ASCII_END_CHAR              126
#define ASCII_LEN                   95

#define DEVANAGARI_START_CHAR       2304
#define DEVANAGARI_END_CHAR         2431
#define DEVANAGARI_LEN              128
#define DEVANAGARI_EXT_START_CHAR   43232
#define DEVANAGARI_EXT_END_CHAR     43263
#define DEVANAGARI_EXT_LEN          32

#define MAX_CHAR_INDEX              0xffff

using namespace std;

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
};

extern float glyphDescender;
extern uint32_t fontTextureID;
extern vector<Char> chvec;
extern Character* characters;
extern unsigned int textVAO, textVBO;

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
void initFontTexture(string path);

uint16_t getCharIndex(uint32_t charcode);

#endif // PATRAM_TEXT

