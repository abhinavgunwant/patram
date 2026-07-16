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

#define ICONS_ATLAS_FILE            "assets/images/icons.png"

#define MAX_CHARACTERS              65535
#define MAX_ICONS                   11
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
        glUseProgram(shaders[SHADER_TEXT].programId);\
        glUniform4fv(uniforms[U_TEXT_COLOR], 4, color);\
        glUniform2f(uniforms[U_TEXT_RESOLUTION], resolution.x, resolution.y);\
        glDrawArrays(GL_TRIANGLES, 0, numVertices / 4);\
    } while(false)

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
extern uint32_t iconTextureID;
extern vector<Char> chvec;
extern Character* characters;
extern Character icons[MAX_ICONS];
extern unsigned int textVAO, textVBO;

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
    bool renderNextContentBlock;
};

inline bool getCharParams(
    Character& c,
    glm::vec2& pos,
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
    glm::vec2& res,
    vector<float>& vertices,
    glm::vec4 color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
    bool redraw = false,
    uint32_t texture = fontTextureID
);

RopeRenderResult renderContentRope(
    Rope* rope,
    uint32_t startIndex,
    glm::vec2 pos,
    glm::vec2& res,
    uint32_t& textVAO,
    uint32_t& textVBO,
    uint32_t& bufferSize,
    float parentWidth,
    glm::fvec3 = glm::fvec3(1.0f, 1.0f, 1.0f),
    uint8_t fontSize = 16,
    bool redraw = true
);

RopeRenderResult renderTextCached(
    string str,
    glm::vec2 pos,
    glm::vec2& res,
    uint32_t& vao,
    uint32_t& vbo,
    uint32_t& bufferSize,
    uint32_t& numVertices,
    float parentLeft,
    float parentWidth,
    glm::fvec4 color = glm::fvec4(1.0f, 1.0f, 1.0f, 1.0f),
    uint8_t fontSize = 16,
    bool redraw = true,
    bool lineBreak = true
);

RopeRenderResult getTextDimensions(
    string text, glm::vec2 pos, glm::vec2& res, uint8_t fontSize,
    float parentLeft, float parentWidth, bool breakLine = true
);

TextRenderResult getInputTextDimensions(
    string text, TextInputData* tid, glm::vec2 pos, uint8_t fontSize,
    float parentLeft, float parentWidth, bool breakLine = true
);

float appendCharVertices(
    float x,
    float y,
    Character c,
    vector<float>& vertices
);

float appendTextVertices(
    float x,
    float y,
    string text,
    vector<float>& vertices
);

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
void initIconTexture();

uint16_t getCharIndex(uint32_t charcode);

#endif // PATRAM_UI_TEXT

