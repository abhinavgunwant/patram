/**
 * Inspired by: https://learnopengl.com/In-Practice/Text-Rendering
 **/

#ifndef PATRAM_UI_TEXT
#define PATRAM_UI_TEXT

#include <string>

#include <glm/glm.hpp>
#include <ft2build.h>
#include FT_FREETYPE_H

#include "ui.hpp"
#include "files.hpp"
#include "arena.hpp"

#define ICONS_ATLAS_FILE            "assets/images/icons.png"

#define MAX_CHARACTERS              65535
#define MAX_ICONS                   11
#define DEFAULT_FONT_SIZE           48
#define ATLAS_DIMENSION             1400

#define CHARACTER_REGULAR           0
#define CHARACTER_BOLD              1
#define CHARACTER_ITALIC            2

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

#define ICON_CHEVRON_DOWN           0
#define ICON_CHEVRON_RIGHT          1
#define ICON_CHEVRON_LEFT           2
#define ICON_FILE                   3
#define ICON_SETTINGS               4
#define ICON_SIDEBAR                5
#define ICON_CLOSE                  6
#define ICON_MOVE                   7
#define ICON_MORE_VERTICAL          8
#define ICON_BULLET_CIRCLE          9
#define ICON_IMAGE                  10

#define DRAW_ICON(vao, color, numVertices) do {\
        glBindVertexArray(vao);\
        glActiveTexture(GL_TEXTURE0);\
        glBindTexture(GL_TEXTURE_2D, iconTextureID);\
        glUseProgram(shaders[SHADER_TEXT]);\
        glDrawArrays(GL_TRIANGLES, 0, numVertices);\
    } while(false)

using namespace std;

struct Glyph {
    uint8_t width;
    uint8_t height;
    int8_t bearingLeft;
    uint8_t bearingTop;
    uint16_t advance;   // Horizontal offset to advance to next glyph

    float textureX1;
    float textureY1;
    float textureX2;
    float textureY2;
};

struct Char {
    uint8_t width;
    uint8_t height;
    Glyph* glyph;
    uint8_t* bitmap;
};

const Glyph CHAR_EMPTY = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };

struct Character {
    Glyph glyphs[3][256];
};

extern float glyphDescender;
extern uint32_t fontTextureID;
extern uint32_t iconTextureID;
extern Character* characters[256];
extern const Glyph icons[MAX_ICONS];

uint8_t* createFontAtlas(Arena& arena);
void initFonts(Arena& arena);
void initTextRendering(uint16_t width, uint16_t height);

struct RopeRenderResult {
    float x;
    float y;
    float height;
    float width;
    uint32_t rows;
    bool renderNextContentBlock;
};

struct TextRenderResult {
    float x;
    float y;
    float selStartX;
    float selStartY;
    float selWidth;
    float selHeight;
    float cursorX;
    float cursorY;
    float height;
    float width;
    uint32_t rows;
    uint32_t columns;
    bool renderNextContentBlock;
};

inline bool getCharParams(
    Glyph c,
    float x,
    float y,
    float& xpos,
    float& ypos,
    float& yNet,
    float& xpos_plus_w,
    float& ypos_plus_h,
    float& height,
    float& texX1,
    float& texX2,
    float& texY1,
    float& texY2,
    float& glyphHeight,
    float& glyphWidth,
    float& bearingTop,
    float& bearingLeft,
    float& advance,
    float rightBound,
    uint32_t& rows,
    uint8_t fontSize,
    bool& renderNextContentBlock,
    bool breakLine = true
);

inline void drawTextBuffers(
    uint32_t& vao,
    uint32_t& vbo,
    uint32_t& bufferSize,
    uint32_t& numVertices,
    vector<TextVertex>& vertices,
    float time = 1.0f,
    bool redraw = false,
    uint32_t texture = fontTextureID
);

RopeRenderResult renderTextCached(
    string str,
    glm::vec2 pos,
    uint32_t& vao,
    uint32_t& vbo,
    uint32_t& bufferSize,
    uint32_t& numVertices,
    float parentLeft,
    float parentWidth,
    float time = 1.0f,
    uint8_t fontSize = 16,
    uint32_t colorIndex = WHITE_INDEX,
    bool redraw = true,
    bool lineBreak = true
);

RopeRenderResult getTextDimensions(
    string text, float x, float y, uint8_t variant, uint8_t fontSize,
    float parentLeft, float parentWidth, bool breakLine = true
);

TextRenderResult getInputTextDimensions(
    string text, TextInputData* tid, float x, float y, uint8_t variant,
    uint8_t fontSize, float parentLeft, float parentWidth, bool breakLine = true
);

float appendCharVertices(
    float x,
    float y,
    Glyph c,
    uint32_t colorIndex,
    vector<TextVertex>& vertices
);

RopeRenderResult appendTextVertices(
    float x,
    float y,
    string text,
    uint8_t variant,
    uint32_t colorIndex,
    uint8_t fontSize,
    vector<TextVertex>& vertices
);

RopeRenderResult appendRopeVertices(
    float x,
    float y,
    Rope *rope,
    uint8_t variant,
    uint32_t startIndex,
    uint8_t fontSize,
    uint32_t colorIndex,
    vector<TextVertex>& vertices
);

void initFontTexture(uint8_t* pngData);
void initIconTexture();

uint16_t getCharIndex(uint32_t charcode);

#endif // PATRAM_UI_TEXT

