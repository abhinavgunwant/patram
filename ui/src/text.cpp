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
Character* characters;
Shader textShader;
unsigned int textVAO, textVBO;
uint32_t resolutionUniform = 0;

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
        cerr << "\nError: Failed to load font texture: " << stbi_failure_reason();
    }

    if (data) {
        #ifdef DEBUG_TEXTURE
            cout << "\nTexture " << path << " loaded.";
        #endif

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    } else {
        cerr << "\nError: Failed to load texture (" << path << ").";
    }

    stbi_image_free(data);
}

void initTextRendering(uint16_t width, uint16_t height) {
    int32_t pid = shaders[SHADER_TEXT].programId;
    glUseProgram(pid);

    glUniform2f(
        glGetUniformLocation(pid, "resolution"),
        static_cast<float>(width),
        static_cast<float>(height)
    );

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
        glGetUniformLocation(shaders[SHADER_TEXT].programId, "textColor"),
        color.r,
        color.g,
        color.b
    );

    resolutionUniform = glGetUniformLocation(shaders[SHADER_TEXT].programId, "resolution");
    glUniform2f(resolutionUniform, resolution.x, resolution.y);
}

RopeRenderResult renderContentRope(
    Rope* rope,
    glm::vec2 pos,
    glm::vec2& res,
    uint32_t& cbTextVAO,
    uint32_t& cbTextVBO,
    uint32_t& bufferSize,
    glm::fvec3 color,
    uint8_t fontSize,
    UIElement* parent,
    bool redraw
) {
    RopeRenderResult result = RopeRenderResult { pos.x, pos.y, 0.0f, 0.0f, 0, true };

    float xpos = pos.x;
    float yNet = pos.y;

    vector<float> vertices;

    if (redraw) {
        vertices.reserve(48000);
    } else {
        vertices.reserve(0);
    }

    uint32_t numVertices = 0;

    RopeTraverser rt(rope);

    while (rt.hasNext()) {
        Rope* r = rt.next();

        if (!redraw) {
            numVertices += r->text.length() * 24;
        }

        for (const char c : r->text) {
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

            if (glyphWidth != glyphWidth) {
                if (ch->width != 0) {
                    glyphWidth = static_cast<float>(ch->width) / static_cast<float>(fontSize);
                } else {
                    glyphWidth = 0.0f;
                }
            }

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
                    parent->width < 800.0f
                    && xpos + glyphWidth > parent->width - CONTENT_HORIZONTAL_MARGIN
                    || xpos + glyphWidth > contentRight
                )
            ) {
                yNet += fontSize * (1 - glyphDescender);
                xpos = pos.x;
                result.rows++;

                if (yNet > windowHeight) {
                    result.renderNextContentBlock = false;
                }
            }

            float ypos = yNet + glyphHeight - bearingTop + fontSize;

            if (glyphDescender < 0) {
                result.height = ypos - pos.y - (glyphDescender * fontSize);
            } else {
                result.height = ypos - pos.y;
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

            if (redraw && c != ' ') {
                vertices.insert(vertices.end(), {
                    xpos,           ypos_plus_h,    ch->textureX1, ch->textureY1,
                    xpos,           ypos,           ch->textureX1, ch->textureY2,
                    xpos_plus_w,    ypos,           ch->textureX2, ch->textureY2,
                    xpos,           ypos_plus_h,    ch->textureX1, ch->textureY1,
                    xpos_plus_w,    ypos,           ch->textureX2, ch->textureY2,
                    xpos_plus_w,    ypos_plus_h,    ch->textureX2, ch->textureY1
                });
            }

            xpos += (static_cast<uint32_t>(ch->advance * fontHeightRatio) >> 6) - bearingLeft;
        }
    }

    glUseProgram(shaders[SHADER_TEXT].programId);

    glActiveTexture(GL_TEXTURE0);

    if (cbTextVAO == 0) {
        glGenVertexArrays(1, &cbTextVAO);
        glBindVertexArray(cbTextVAO);

        glGenBuffers(1, &cbTextVBO);
        glBindBuffer(GL_ARRAY_BUFFER, cbTextVBO);

        bufferSize = vertices.size() * sizeof(float);
        glBufferData(GL_ARRAY_BUFFER, bufferSize, NULL, GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
        glEnableVertexAttribArray(0);
    }

    glBindVertexArray(cbTextVAO);
    glBindTexture(GL_TEXTURE_2D, fontTextureID);
    glBindBuffer(GL_ARRAY_BUFFER, cbTextVBO);

    if (redraw) {
        glUniform2f(resolutionUniform, res.x, res.y);

        numVertices = vertices.size();
        uint32_t requiredBufferSize = numVertices * sizeof(float);

        if (bufferSize >= requiredBufferSize) {
            uint32_t zeros = bufferSize - requiredBufferSize;

            for (uint32_t i=0; i<zeros; ++i) { vertices.push_back(0.0f); }

            glBufferSubData(GL_ARRAY_BUFFER, 0, bufferSize, &vertices[0]);
        } else {
            while (bufferSize < requiredBufferSize) {
                bufferSize *= 1.6;
            }

            glDeleteBuffers(1, &cbTextVBO);
            glGenBuffers(1, &cbTextVBO);
            glBindBuffer(GL_ARRAY_BUFFER, cbTextVBO);
            glBufferData(GL_ARRAY_BUFFER, bufferSize, &vertices[0], GL_DYNAMIC_DRAW);
            glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
            glEnableVertexAttribArray(0);
        }
    }

    glDrawArrays(GL_TRIANGLES, 0, numVertices / 4);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    result.width = pos.x;

    if (result.rows) {
        ++result.rows;
    }

    return result;
}

// render line of text
glm::vec4 renderText(
    string text, glm::vec2 pos, glm::vec2& res, glm::fvec3 color,
    uint8_t fontSize, UIElement* parent
) {
    glm::vec4 dimensions = glm::vec4(pos.x, pos.y, 0, 0); // x, y, height, width

    int32_t pid = shaders[SHADER_TEXT].programId;

    glUseProgram(pid);

    glUniform3f(
        glGetUniformLocation(pid, "textColor"),
        color.r,
        color.g,
        color.b
    );
    glUniform2f(glGetUniformLocation(pid, "resolution"), res.x, res.y);
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
                parent->width < 800.0f
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

    dimensions.w = pos.x;

    return dimensions;
}

// render line of text
// TODO: update to be consistent with above function.
glm::vec4 renderText(
    wstring text, glm::vec2 pos, glm::vec2& res, glm::fvec3 color,
    uint8_t fontSize, UIElement* parent
) {
    glm::vec4 dimensions = glm::vec4(pos.x, pos.y, 0, 0);

    int32_t pid = shaders[SHADER_TEXT].programId;

    glUseProgram(pid);

    glUniform3f(glGetUniformLocation(pid, "textColor"), color.r, color.g, color.b);
    glUniform2f(glGetUniformLocation(pid, "resolution"), res.x, res.y);
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
        contentContainer->width < 800.0f
        && xpos + glyphWidth > contentContainer->width - CONTENT_HORIZONTAL_MARGIN
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

