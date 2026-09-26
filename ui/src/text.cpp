#include "include/text.hpp"

#include <algorithm>
#include <vector>
#include <cctype>
#include <chrono>
#include <iostream>
#include <cstdlib>
#include <string>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "logger.hpp"
#include "include/shader.hpp"
#include "include/ui.hpp"
#include "../lib/stb_image_write.h"
#include "../lib/stb_image.h"

using namespace std;

#ifdef DEBUG_TEXT
bool debugTextFirstRenderPrint = true;
#endif

float glyphDescender = 0;
uint32_t fontTextureID = 0;
uint32_t iconTextureID;
Character* characters;
Character icons[MAX_ICONS];
Shader textShader;
unsigned int textVAO, textVBO;

void initFontTexture(string path) {
    #ifdef DEBUG_TEXT
        cout << "\nDEBUG: STARTUP CHECK: loading font texture";
    #endif

    glGenTextures(1, &fontTextureID);
    glBindTexture(GL_TEXTURE_2D, fontTextureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    int width, height, nrChannels;

    stbi_set_flip_vertically_on_load(true);
    uint8_t* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);

    if (stbi_failure_reason()) {
        errorv("Failed to load font texture: %s", stbi_failure_reason());
    }

    if (data) {
        #ifdef DEBUG_TEXTURE
            cout << "\nTexture " << path << " loaded.";
        #endif

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    } else {
        errorv("Failed to load texture (%s).", path);
    }

    stbi_image_free(data);
}

void initIconTexture() {
    icons[ICON_CHEVRON_DOWN] = {
        32, 15, 0, 0, 27,
        0.0058,  0.34,
        0.0867, 0.66
    };

    icons[ICON_CHEVRON_RIGHT] = {
        15, 32, 0, 0, 32,
        0.0925, 0.22,
        0.1387, 0.78
    };

    icons[ICON_CHEVRON_LEFT] = {
        15, 32, 0, 0, 32,
        0.1445, 0.22,
        0.1908, 0.78
    };

    icons[ICON_FILE] = {
        35, 42, 0, 0, 35,
        0.1965, 0.06,
        0.3006, 0.94
    };

    icons[ICON_SETTINGS] = {
        46, 46, 0, 0, 46,
        0.3064, 0.02,
        0.4451, 0.98
    };

    icons[ICON_SIDEBAR] = {
        38, 38, 0, 0, 38,
        0.4509, 0.1,
        0.5665, 0.9
    };

    icons[ICON_CLOSE] = {
        27, 27, 0, 0, 27,
        0.5780, 0.22,
        0.659,  0.78
    };

    icons[ICON_MOVE] = {
        42, 42, 0, 0, 27,
        0.6647, 0.06,
        0.7919, 0.94
    };

    icons[ICON_MORE_VERTICAL] = {
        7, 34, 0, 0, 27,
        0.7977, 0.14,
        0.8208, 0.86
    };

    icons[ICON_BULLET_CIRCLE] = {
        15, 15, 0, 0, 15,
        0.8266, 0.34,
        0.8728, 0.66
    };

    icons[ICON_IMAGE] = {
        38, 38, 0, 0, 38,
        0.8786, 0.1,
        0.9942, 0.9
    };

    glGenTextures(1, &iconTextureID);
    glBindTexture(GL_TEXTURE_2D, iconTextureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    int width, height, nrChannels;

    const char* path = ICONS_ATLAS_FILE;

    stbi_set_flip_vertically_on_load(false);
    uint8_t* data = stbi_load(path, &width, &height, &nrChannels, 0);

    uint32_t dataSize = width * height;

    if (width < 0 || height < 0) {
        dataSize = 0;
    }

    if (stbi_failure_reason()) {
        errorv("\nFailed to load icon texture: %s", stbi_failure_reason());
    }

    if (data) {
        #ifdef DEBUG_TEXTURE
            cout << "\nTexture " << path << " loaded.";
        #endif

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        Character moveIcon = icons[ICON_MOVE];
        uint32_t moveCursorWidth = moveIcon.width/2;
        uint32_t moveCursorHeight = moveIcon.height/2;
        uint32_t moveCursorSize = moveCursorWidth * moveCursorHeight * 4;
        uint32_t moveCursorDataXOffset = static_cast<uint32_t>(static_cast<float>(width) * moveIcon.textureX1);
        uint32_t moveCursorDataYOffset = static_cast<uint32_t>(static_cast<float>(height) * moveIcon.textureY1 * width);
        uint32_t moveIconActualWidth = static_cast<uint32_t>(static_cast<float>(width) * moveIcon.textureX2) - static_cast<uint32_t>(static_cast<float>(width) * moveIcon.textureX1) + 1;
        uint32_t moveIconActualHeight = static_cast<uint32_t>(static_cast<float>(height) * moveIcon.textureY2) - static_cast<uint32_t>(static_cast<float>(height) * moveIcon.textureY1) + 1;

        moveCursorBitmap = new uint8_t[moveCursorSize];
        memset(moveCursorBitmap, 0xff, moveCursorSize);

        for (uint32_t i=0; i<moveCursorHeight&& i<dataSize; ++i) {
            for (uint32_t j=0; j<moveCursorWidth; ++j) {
                uint32_t sourceX = static_cast<float>(j) * moveIconActualWidth / moveCursorWidth;
                uint32_t sourceY = static_cast<float>(i) * moveIconActualHeight / moveCursorHeight;

                uint32_t dataIndex = moveCursorDataYOffset + width * sourceY + moveCursorDataXOffset + sourceX;

                if (dataIndex >= dataSize) { continue; }

                moveCursorBitmap[moveCursorWidth * i * 4 + j*4 + 3] = data[dataIndex];
            }
        }
    } else {
        errorv("Failed to load texture (%s).", path);
    }

    stbi_image_free(data);
}

void initTextRendering(uint16_t width, uint16_t height) {
    glUseProgram(shaders[SHADER_TEXT].programId);

    glGenVertexArrays(1, &textVAO);
    glGenBuffers(1, &textVBO);
    glBindVertexArray(textVAO);
    glBindBuffer(GL_ARRAY_BUFFER, textVBO);
    glBufferData(GL_ARRAY_BUFFER, 1048576, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    glUseProgram(shaders[SHADER_TEXT].programId);

    glm::vec3 color = darkTheme ? WHITE : BLACK;

    glUniform3f(
        uniforms[U_TEXT_COLOR],
        color.r,
        color.g,
        color.b
    );
}

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
    bool breakLine
) {
    float aspectRatio = static_cast<float>(c.width) / static_cast<float>(c.height);
    float fontHeightRatio = static_cast<float>(fontSize) / DEFAULT_FONT_SIZE;
    glyphHeight = static_cast<float>(c.height) * fontHeightRatio;
    glyphWidth = glyphHeight * aspectRatio;

    if (glyphWidth != glyphWidth) {
        if (c.width != 0) {
            glyphWidth = static_cast<float>(c.width) / static_cast<float>(fontSize);
        } else {
            glyphWidth = 0.0f;
        }
    }

    float widthRatio = glyphWidth / c.width;
    float heightRatio = glyphHeight / c.height;
    bearingTop = c.bearingTop * heightRatio;
    bearingLeft = c.bearingLeft * widthRatio;

    if (bearingLeft != bearingLeft) {
        bearingLeft = 0.0f;
    }

    if (bearingTop != bearingTop) {
        bearingTop = 0.0f;
    }

    xpos += bearingLeft;

    if (xpos + glyphWidth > rightBound) {
        if (!breakLine) {
            return false;
        }

        yNet += static_cast<float>(fontSize) * (1.0f - glyphDescender);
        xpos = pos.x;
        rows++;

        if (yNet > windowHeight) {
            renderNextContentBlock = false;
        }
    }

    ypos = yNet + glyphHeight - bearingTop + fontSize;

    if (glyphDescender < 0) {
        height = ypos - pos.y - (glyphDescender * fontSize);
    } else {
        height = ypos - pos.y;
    }

    xpos_plus_w = xpos + glyphWidth;
    ypos_plus_h = ypos - glyphHeight;

    advance = (static_cast<uint32_t>(c.advance * fontHeightRatio) >> 6) - bearingLeft;

    texX1 = c.textureX1;
    texX2 = c.textureX2;
    texY1 = c.textureY1;
    texY2 = c.textureY2;

    return true;
}

inline void drawTextBuffers(
    uint32_t& vao,
    uint32_t& vbo,
    uint32_t& bufferSize,
    uint32_t& numVertices,
    glm::vec2& res,
    vector<float>& vertices,
    glm::vec4 color,
    bool redraw,
    uint32_t texture
) {
    glUseProgram(shaders[SHADER_TEXT].programId);

    glUniform4f(uniforms[U_TEXT_COLOR], color.r, color.g, color.b, color.a);

    glActiveTexture(GL_TEXTURE0);

    if (vao == 0 && !vertices.empty()) {
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);

        bufferSize = vertices.size() * sizeof(float);
        glBufferData(GL_ARRAY_BUFFER, bufferSize, NULL, GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
        glEnableVertexAttribArray(0);
    }

    glBindVertexArray(vao);
    glBindTexture(GL_TEXTURE_2D, texture);

    if (redraw && bufferSize > 0) {
        glBindBuffer(GL_ARRAY_BUFFER, vbo);

        numVertices = vertices.size();
        uint32_t requiredBufferSize = numVertices * sizeof(float);

        if (bufferSize >= requiredBufferSize) {
            uint32_t zeros = (bufferSize - requiredBufferSize) / sizeof(float);

            for (uint32_t i=0; i<zeros; ++i) { vertices.push_back(0.0f); }

            glBufferSubData(GL_ARRAY_BUFFER, 0, bufferSize, &vertices[0]);
        } else {
            bufferSize = requiredBufferSize;
            glBufferData(GL_ARRAY_BUFFER, bufferSize, &vertices[0], GL_DYNAMIC_DRAW);
        }
    }

    glDrawArrays(GL_TRIANGLES, 0, numVertices / 4);
}

RopeRenderResult renderContentRope(
    Rope* rope,
    uint32_t startIndex,
    glm::vec2 pos,
    glm::vec2& res,
    uint32_t& cbTextVAO,
    uint32_t& cbTextVBO,
    uint32_t& bufferSize,
    float parentWidth,
    glm::fvec3 color,
    uint8_t fontSize,
    bool redraw
) {
    RopeRenderResult result = RopeRenderResult { pos.x, pos.y, 0.0f, 0.0f, 0, true };

    float xpos = pos.x;
    float ypos = 0.0f;
    float yNet = pos.y;
    float texX1 = 0.0f;
    float texX2 = 0.0f;
    float texY1 = 0.0f;
    float texY2 = 0.0f;
    float xpos_plus_w = 0.0f;
    float ypos_plus_h = 0.0f;
    float advance = 0.0f;

    vector<float> vertices;

    if (redraw) {
        vertices.reserve(48000);
    } else {
        vertices.reserve(0);
    }

    uint32_t numVertices = 0;

    RopeTraverser rt(rope);

    uint32_t i = 0;

    while (rt.hasNext()) {
        Rope* r = rt.next();

        if (!redraw) {
            numVertices += r->text.length() * 24;
        }

        float glyphHeight, glyphWidth, bearingTop, bearingLeft;

        for (const char c : r->text) {
            if (i < startIndex) {
                ++i;
                continue;
            }
            #ifdef DEBUG_TEXT
            if (debugTextFirstRenderPrint) {
                cout << "\n--> Rendering character: '" << (char) c << "'";
            }
            #endif

            if (!getCharParams(
                characters[(int)c],
                pos,
                xpos,
                ypos,
                yNet,
                xpos_plus_w,
                ypos_plus_h,
                result.height,
                texX1,
                texX2,
                texY1,
                texY2,
                glyphHeight,
                glyphWidth,
                bearingTop,
                bearingLeft,
                advance,
                contentRight,
                result.rows,
                fontSize,
                result.renderNextContentBlock
            )) {
                break;
            }

            #ifdef DEBUG_TEXT
            if (debugTextFirstRenderPrint) {
                cout << "; x: " << xpos << " y: " << ypos << " x + w: "
                    << xpos_plus_w << " y + h: " << ypos_plus_h
                    << " bearingLeft: " << bearingLeft
                    << " bearingTop: " << bearingTop
                    << " widthRatio: " << widthRatio
                    << " heightRatio: " << heightRatio;
            }
            #endif

            if (redraw && c != ' ') {
                vertices.insert(vertices.end(), {
                    xpos,           ypos_plus_h,    texX1, texY1,
                    xpos,           ypos,           texX1, texY2,
                    xpos_plus_w,    ypos,           texX2, texY2,
                    xpos,           ypos_plus_h,    texX1, texY1,
                    xpos_plus_w,    ypos,           texX2, texY2,
                    xpos_plus_w,    ypos_plus_h,    texX2, texY1
                });
            }

            xpos += advance;
        }
    }

    drawTextBuffers(cbTextVAO, cbTextVBO, bufferSize, numVertices, res, vertices, WHITE, redraw);

    result.width = pos.x;

    ++result.rows;

    return result;
}

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
    glm::vec4 color,
    uint8_t fontSize,
    bool redraw,
    bool lineBreak
) {
    RopeRenderResult result = RopeRenderResult { pos.x, pos.y, 0.0f, 0.0f, 0, true };

    float xpos = pos.x;
    float ypos = 0.0f;
    float yNet = pos.y;
    float texX1 = 0.0f;
    float texX2 = 0.0f;
    float texY1 = 0.0f;
    float texY2 = 0.0f;
    float xpos_plus_w = 0.0f;
    float ypos_plus_h = 0.0f;
    float advance = 0.0f;

    vector<float> vertices;

    if (redraw) {
        vertices.reserve(800);
    } else {
        vertices.reserve(0);
    }

    uint32_t _numVertices = 0;

    if (!redraw) {
        _numVertices += str.length() * 24;
    }

    float glyphHeight, glyphWidth, bearingTop, bearingLeft;

    if (redraw) {
        for (char c: str) {
            if (!getCharParams(
                characters[(int)c],
                pos,
                xpos,
                ypos,
                yNet,
                xpos_plus_w,
                ypos_plus_h,
                result.height,
                texX1,
                texX2,
                texY1,
                texY2,
                glyphHeight,
                glyphWidth,
                bearingTop,
                bearingLeft,
                advance,
                parentLeft + parentWidth,
                result.rows,
                fontSize,
                result.renderNextContentBlock,
                lineBreak
            )) {
                break;
            }

            if (redraw && c != ' ') {
                vertices.insert(vertices.end(), {
                    xpos,           ypos_plus_h,    texX1, texY1,
                    xpos,           ypos,           texX1, texY2,
                    xpos_plus_w,    ypos,           texX2, texY2,
                    xpos,           ypos_plus_h,    texX1, texY1,
                    xpos_plus_w,    ypos,           texX2, texY2,
                    xpos_plus_w,    ypos_plus_h,    texX2, texY1
                });
            }

            xpos += advance;
        }
    }

    drawTextBuffers(vao, vbo, bufferSize, _numVertices, res, vertices, color, redraw);

    numVertices = _numVertices;

    result.width = xpos - pos.x;

    if (result.rows) {
        ++result.rows;
    }

    return result;
}

RopeRenderResult getTextDimensions(
    string str, glm::vec2 pos, glm::vec2& res, uint8_t fontSize,
    float parentLeft, float parentWidth, bool breakLine
) {
    RopeRenderResult result = RopeRenderResult { pos.x, pos.y, 0.0f, 0.0f, 0, true };

    float xpos = pos.x;
    float ypos = 0.0f;
    float yNet = pos.y;
    float texX1 = 0.0f;
    float texX2 = 0.0f;
    float texY1 = 0.0f;
    float texY2 = 0.0f;
    float xpos_plus_w = 0.0f;
    float ypos_plus_h = 0.0f;
    float advance = 0.0f;

    float glyphHeight, glyphWidth, bearingTop, bearingLeft;

    for (char c: str) {
        if (!getCharParams(
            characters[(int)c],
            pos,
            xpos,
            ypos,
            yNet,
            xpos_plus_w,
            ypos_plus_h,
            result.height,
            texX1,
            texX2,
            texY1,
            texY2,
            glyphHeight,
            glyphWidth,
            bearingTop,
            bearingLeft,
            advance,
            parentLeft + parentWidth,
            result.rows,
            fontSize,
            result.renderNextContentBlock,
            breakLine
        )) {
            break;
        }

        xpos += advance;
    }

    result.width = xpos - pos.x;

    if (result.rows) {
        ++result.rows;
    }

    return result;
}

TextRenderResult getInputTextDimensions(
    string str, TextInputData* tid, glm::vec2 pos, uint8_t fontSize,
    float parentLeft, float parentWidth, bool breakLine
) {
    TextRenderResult result = TextRenderResult {
        pos.x, pos.y, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0, true
    };

    float xpos = pos.x;
    float ypos = 0.0f;
    float yNet = pos.y;
    float texX1 = 0.0f;
    float texX2 = 0.0f;
    float texY1 = 0.0f;
    float texY2 = 0.0f;
    float xpos_plus_w = 0.0f;
    float ypos_plus_h = 0.0f;
    float advance = 0.0f;

    float glyphHeight, glyphWidth, bearingTop, bearingLeft;
    uint32_t i = 0;

    for (char c: str) {
        if (!getCharParams(
            characters[(int)c],
            pos,
            xpos,
            ypos,
            yNet,
            xpos_plus_w,
            ypos_plus_h,
            result.height,
            texX1,
            texX2,
            texY1,
            texY2,
            glyphHeight,
            glyphWidth,
            bearingTop,
            bearingLeft,
            advance,
            parentLeft + parentWidth,
            result.rows,
            fontSize,
            result.renderNextContentBlock,
            breakLine
        )) {
            break;
        }

        if (i == tid->textSelectStartCol && result.rows == tid->textSelectStartRow) {
            result.selStartX = xpos;
            result.selStartY = ypos;
        }

        if (i == tid->cursorPosition) {
            result.cursorX = xpos;
            result.cursorY = ypos;
        }

        xpos += advance;

        if (i == tid->textSelectEndCol - 1 && result.rows == tid->textSelectEndRow) {
            result.selWidth = xpos - result.selStartX;
            result.selHeight = ypos + static_cast<float>(fontSize) - result.selStartY;
        }

        ++i;
    }

    if (i == tid->cursorPosition) {
        result.cursorX = xpos;
        result.cursorY = ypos;
    }

    if (i == tid->textSelectEndCol - 1 && result.rows == tid->textSelectEndRow) {
        result.selWidth = xpos - result.selStartX;
        result.selHeight = ypos + static_cast<float>(fontSize) - result.selStartY;
    }

    if (tid->textSelectStartCol == tid->textSelectEndCol) {
        result.selWidth = 0.0f;
    }

    result.width = xpos - pos.x;

    if (result.rows) {
        ++result.rows;
    }

    return result;
}

float appendCharVertices(
    float x,
    float y,
    Character c,
    vector<float>& vertices
) {
    float xpos = x, ypos = x, yNet = y, xpos_plus_w, ypos_plus_h, texX1, texX2, texY1, texY2,
        glyphHeight, glyphWidth, bearingTop, bearingLeft, advance = 0, totalHeight;
    float infinity = numeric_limits<float>::infinity();
    uint32_t rows;
    glm::vec2 pos = glm::vec2(x, y);
    bool renderNextContentBlock;

    getCharParams(
        c,
        pos,
        xpos,
        ypos,
        yNet,
        xpos_plus_w,
        ypos_plus_h,
        totalHeight,
        texX1,
        texX2,
        texY1,
        texY2,
        glyphHeight,
        glyphWidth,
        bearingTop,
        bearingLeft,
        advance,
        infinity,
        rows,
        16,
        renderNextContentBlock
    );

    vertices.insert(vertices.end(), {
        xpos,           ypos_plus_h,    texX1, texY1,
        xpos,           ypos,           texX1, texY2,
        xpos_plus_w,    ypos,           texX2, texY2,
        xpos,           ypos_plus_h,    texX1, texY1,
        xpos_plus_w,    ypos,           texX2, texY2,
        xpos_plus_w,    ypos_plus_h,    texX2, texY1
    });

    return glyphWidth;
}

float appendTextVertices(
    float x,
    float y,
    string text,
    vector<float>& vertices
) {
    float xpos = x, ypos = x, yNet = y, xpos_plus_w, ypos_plus_h, texX1, texX2, texY1, texY2,
        glyphHeight, glyphWidth, bearingTop, bearingLeft, advance = 0, totalHeight;
    float infinity = numeric_limits<float>::infinity();

    uint32_t rows;

    bool renderNextContentBlock;

    for (char c: text) {
        // appendCharVertices(x, y, characters[(int)c], vertices);
        glm::vec2 pos = glm::vec2(x, y);

        getCharParams(
            characters[(int)c],
            pos,
            xpos,
            ypos,
            yNet,
            xpos_plus_w,
            ypos_plus_h,
            totalHeight,
            texX1,
            texX2,
            texY1,
            texY2,
            glyphHeight,
            glyphWidth,
            bearingTop,
            bearingLeft,
            advance,
            infinity,
            rows,
            16,
            renderNextContentBlock
        );

        vertices.insert(vertices.end(), {
            xpos,           ypos_plus_h,    texX1, texY1,
            xpos,           ypos,           texX1, texY2,
            xpos_plus_w,    ypos,           texX2, texY2,
            xpos,           ypos_plus_h,    texX1, texY1,
            xpos_plus_w,    ypos,           texX2, texY2,
            xpos_plus_w,    ypos_plus_h,    texX2, texY1
        });

        xpos += advance;
    }

    return xpos - x;
}

RopeRenderResult appendRopeVertices(
    float x,
    float y,
    Rope* rope,
    uint32_t startIndex,
    uint8_t fontSize,
    vector<float>& vertices
) {
    RopeRenderResult result = RopeRenderResult { x, y, 0.0f, 0.0f, 0, true };

    float xpos = x, ypos = x, yNet = y, xpos_plus_w, ypos_plus_h, texX1, texX2, texY1, texY2,
        glyphHeight, glyphWidth, bearingTop, bearingLeft, advance = 0;

    RopeTraverser rt(rope);

    uint32_t i = 0;

    while (rt.hasNext()) {
        Rope* node = rt.next();

        for (char c: node->text) {
            if (i < startIndex) {
                ++i;
                continue;
            }

            glm::vec2 pos = glm::vec2(x, y);

            getCharParams(
                characters[(int)c],
                pos,
                xpos,
                ypos,
                yNet,
                xpos_plus_w,
                ypos_plus_h,
                result.height,
                texX1,
                texX2,
                texY1,
                texY2,
                glyphHeight,
                glyphWidth,
                bearingTop,
                bearingLeft,
                advance,
                contentRight,
                result.rows,
                fontSize,
                result.renderNextContentBlock
            );

            if (c != ' ') {
                vertices.insert(vertices.end(), {
                    xpos,           ypos_plus_h,    texX1, texY1,
                    xpos,           ypos,           texX1, texY2,
                    xpos_plus_w,    ypos,           texX2, texY2,
                    xpos,           ypos_plus_h,    texX1, texY1,
                    xpos_plus_w,    ypos,           texX2, texY2,
                    xpos_plus_w,    ypos_plus_h,    texX2, texY1
                });
            }

            xpos += advance;
        }
    }

    result.width = x;
    ++result.rows;

    return result;
}

// render line of text
glm::vec4 renderText(
    string text, glm::vec2 pos, glm::vec2& res, glm::fvec3 color,
    uint8_t fontSize, UIElement* parent
) {
    glm::vec4 dimensions = glm::vec4(pos.x, pos.y, 0, 0); // x, y, height, width

    glUseProgram(shaders[SHADER_TEXT].programId);

    glUniform3f(
        uniforms[U_TEXT_COLOR],
        color.r,
        color.g,
        color.b
    );
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(textVAO);

    #ifdef DEBUG_TEXT
    if (debugTextFirstRenderPrint) {
        cout << "\nRendering text: " << text;
    }
    #endif

    float xpos = pos.x;
    float yNet = pos.y;

    // iterate through all characters
    for (const char c : text) {
        Character* ch = &characters[(int)c];

        #ifdef DEBUG_TEXT
        if (debugTextFirstRenderPrint) {
            cout << "\n--> Rendering character: '" << (char) c << "'";
        }
        #endif

        float aspectRatio = static_cast<float>(ch->width) / static_cast<float>(ch->height);

        float fontHeightRatio = static_cast<float>(fontSize) / DEFAULT_FONT_SIZE;

        float glyphHeight = static_cast<float>(ch->height) * fontHeightRatio;
        float glyphWidth = glyphHeight * aspectRatio;

        float widthRatio = glyphWidth / ch->width;
        float heightRatio = glyphHeight / ch->height;

        float bearingTop = ch->bearingTop * heightRatio;
        float bearingLeft = ch->bearingLeft * widthRatio;

        if (bearingLeft != bearingLeft) {
            bearingLeft = 0.0f;
        }

        if (bearingTop != bearingTop) {
            bearingTop = 0.0f;
        }

        xpos += bearingLeft;

        if (
            parent != nullptr
            && (
                parent->width < CONTENT_BREAKPOINT
                && xpos + glyphWidth > parent->width - CONTENT_HORIZONTAL_MARGIN
                || xpos + glyphWidth > contentRight
            )
        ) {
            yNet += fontSize + 4.0f;
            xpos = pos.x;
        }

        float ypos = yNet + glyphHeight - bearingTop + fontSize;

        if (glyphDescender < 0) {
            dimensions.z = ypos - pos.y - (glyphDescender * fontSize);
        } else {
            dimensions.z = ypos - pos.y;
        }

        float xpos_plus_w = xpos + glyphWidth;
        float ypos_plus_h = ypos - glyphHeight; // ypos + h

        #ifdef DEBUG_TEXT
        if (debugTextFirstRenderPrint) {
            cout << "; x: " << xpos << " y: " << ypos << " x + w: "
                << xpos_plus_w << " y + h: " << ypos_plus_h
                << " bearingLeft: " << bearingLeft
                << " bearingTop: " << bearingTop
                << " widthRatio: " << widthRatio
                << " heightRatio: " << heightRatio;
        }
        #endif

        float vertices[6][4] = {
            { xpos,         ypos_plus_h,    ch->textureX1, ch->textureY1 },
            { xpos,         ypos,           ch->textureX1, ch->textureY2 },
            { xpos_plus_w,  ypos,           ch->textureX2, ch->textureY2 },
            { xpos,         ypos_plus_h,    ch->textureX1, ch->textureY1 },
            { xpos_plus_w,  ypos,           ch->textureX2, ch->textureY2 },
            { xpos_plus_w,  ypos_plus_h,    ch->textureX2, ch->textureY1 },
        };

        // render glyph texture over quad
        glBindTexture(GL_TEXTURE_2D, fontTextureID);
        // update content of textVBO memory
        glBindBuffer(GL_ARRAY_BUFFER, textVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); // be sure to use glBufferSubData and not glBufferData

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        // render quad
        glDrawArrays(GL_TRIANGLES, 0, 6);
        // now advance cursors for next glyph (note that advance is number of 1/64 pixels)
        xpos += (static_cast<uint32_t>(ch->advance * fontHeightRatio) >> 6) - bearingLeft; // bitshift by 6 to get value in pixels (2^6 = 64 (divide amount of 1/64th pixels by 64 to get amount of pixels))
    }

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    dimensions.w = xpos;

    return dimensions;
}

// render line of text
// TODO: update to be consistent with above function.
glm::vec4 renderText(
    wstring text, glm::vec2 pos, glm::vec2& res, glm::fvec3 color,
    uint8_t fontSize, UIElement* parent
) {
    glm::vec4 dimensions = glm::vec4(pos.x, pos.y, 0, 0);

    glUseProgram(shaders[SHADER_TEXT].programId);

    glUniform3f(uniforms[U_TEXT_COLOR], color.r, color.g, color.b);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(textVAO);

    // iterate through all characters
    for (const wchar_t c : text) {
        Character ch = characters[(int)c];

        float xpos = pos.x + ch.bearingLeft;
        float ypos = pos.y - (ch.height - ch.bearingTop);

        float xpos_plus_w = xpos - ch.width; // xpos + w
        float ypos_plus_h = ypos + ch.height; // ypos + h
        // update textVBO for each character
        float vertices[6][4] = {
            { xpos,         ypos_plus_h,    0.0f, 0.0f },
            { xpos,         ypos,           0.0f, 1.0f },
            { xpos_plus_w,  ypos,           1.0f, 1.0f },
            { xpos,         ypos_plus_h,    0.0f, 0.0f },
            { xpos_plus_w,  ypos,           1.0f, 1.0f },
            { xpos_plus_w,  ypos_plus_h,    1.0f, 0.0f },
        };
        // render glyph texture over quad
        // glBindTexture(GL_TEXTURE_2D, ch.textureID);
        // update content of textVBO memory
        glBindBuffer(GL_ARRAY_BUFFER, textVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); // be sure to use glBufferSubData and not glBufferData

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        // render quad
        glDrawArrays(GL_TRIANGLES, 0, 6);
        // now advance cursors for next glyph (note that advance is number of 1/64 pixels)
        pos.x += (ch.advance >> 6); // bitshift by 6 to get value in pixels (2^6 = 64 (divide amount of 1/64th pixels by 64 to get amount of pixels))
    }

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    return dimensions;
}

GlyphDimensions getGlyphDimensions(char c, float x, float y, glm::vec2& res, uint8_t fontSize) {
    Character ch = characters[(int)c];

    float aspectRatio = static_cast<float>(ch.width) / static_cast<float>(ch.height);

    float fontHeightRatio = static_cast<float>(fontSize) / DEFAULT_FONT_SIZE;

    float glyphHeight = static_cast<float>(ch.height) * fontHeightRatio;
    float glyphWidth = glyphHeight * aspectRatio;

    float widthRatio = glyphWidth / ch.width;
    float heightRatio = glyphHeight / ch.height;

    float bearingTop = ch.bearingTop * heightRatio;
    float bearingLeft = ch.bearingLeft * widthRatio;

    if (bearingLeft != bearingLeft) {
        bearingLeft = 0.0f;
    }

    if (bearingTop != bearingTop) {
        bearingTop = 0.0f;
    }

    float xpos = x + bearingLeft;
    float yNet = y;

    if (
        CONTENT_CONTAINER.width < CONTENT_BREAKPOINT
        && xpos + glyphWidth > CONTENT_CONTAINER.width - CONTENT_HORIZONTAL_MARGIN
        || xpos + glyphWidth > contentRight
    ) {
        yNet += fontSize + 4.0f;
        xpos = x;
    }

    float ypos = yNet + glyphHeight - bearingTop + fontSize;

    // float xpos_plus_w = xpos + glyphWidth;
    // float ypos_plus_h = ypos - glyphHeight; // ypos + h

    #ifdef DEBUG_TEXT
    if (debugTextFirstRenderPrint) {
     cout << "\nRendering glyph: " << c
        << "\n--> Rendering character: '" << (char) c
        << "'; x: " << xpos << " y: " << ypos << " x + w: "
        << xpos + glyphWidth << " y + h: " << ypos - glyphHeight
        << " bearingLeft: " << bearingLeft << "(" << ch.bearingLeft << ", " <<  widthRatio << ")"
        << " bearingTop: " << bearingTop
        << " widthRatio: " << widthRatio
        << " heightRatio: " << heightRatio;
    }
    #endif


    return GlyphDimensions {
        .leftX = xpos,
        .rightX = xpos + glyphWidth,
        .bottomY = ypos,
        .topY = ypos - glyphHeight,
        .scale = fontHeightRatio,
        .advance = ch.advance,
    };
}

uint32_t renderGlyph(char c, float x, float y, glm::vec2& res, uint8_t fontSize) {
    GlyphDimensions gd = getGlyphDimensions(c, x, y, res, fontSize);

    // update textVBO for each character
    // float vertices[4][4] = {
    //     { xpos,         ypos,           0.0f, 1.0f },
    //     { xpos_plus_w,  ypos,           1.0f, 1.0f },
    //     { xpos,         ypos_plus_h,    0.0f, 0.0f },
    //     { xpos_plus_w,  ypos_plus_h,    1.0f, 0.0f },
    // };
    float vertices[4][4] = {
        { gd.leftX,   gd.bottomY, 0.0f, 1.0f },
        { gd.rightX,  gd.bottomY, 1.0f, 1.0f },
        { gd.leftX,   gd.topY,    0.0f, 0.0f },
        { gd.rightX,  gd.topY,    1.0f, 0.0f },
    };

    // render glyph texture over quad
    // glBindTexture(GL_TEXTURE_2D, gd.textureID);
    // update content of textVBO memory
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); // be sure to use glBufferSubData and not glBufferData

    // render quad
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    // now advance cursors for next glyph (note that advance is number of 1/64 pixels)
    return (static_cast<uint32_t>(gd.advance * gd.scale) >> 6); // bitshift by 6 to get value in pixels (2^6 = 64 (divide amount of 1/64th pixels by 64 to get amount of pixels))
}

