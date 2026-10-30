#include "include/text.hpp"

#include <algorithm>
#include <vector>
#include <cctype>
#include <chrono>
#include <iostream>
#include <cstdlib>
#include <string>
#include <thread>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "logger.hpp"
#include "include/shader.hpp"
#include "include/ui.hpp"
#include "../lib/stb_image_write.h"
#include "../lib/stb_image.h"

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#endif

using namespace std;

#ifdef DEBUG_TEXT
bool debugTextFirstRenderPrint = true;
#endif

float glyphDescender = 0;
uint32_t fontTextureID = 0;
uint32_t iconTextureID;
Character* characters[256];
unsigned int textVAO, textVBO;

mutex textInitMutex;
Char* charList = nullptr;

atomic<uint32_t> charListIndex(0);

const Glyph icons[MAX_ICONS] = {
    // ICON_CHEVRON_DOWN
    {
        32, 15, 0, 0, 27,
        0.0058,  0.34,
        0.0867, 0.66
    },

    // ICON_CHEVRON_RIGHT
    {
        15, 32, 0, 0, 32,
        0.0925, 0.22,
        0.1387, 0.78
    },

    // ICON_CHEVRON_LEFT
    {
        15, 32, 0, 0, 32,
        0.1445, 0.22,
        0.1908, 0.78
    },

    // ICON_FILE
    {
        35, 42, 0, 0, 35,
        0.1965, 0.06,
        0.3006, 0.94
    },

    // ICON_SETTINGS
    {
        46, 46, 0, 0, 46,
        0.3064, 0.02,
        0.4451, 0.98
    },

    // ICON_SIDEBAR
    {
        38, 38, 0, 0, 38,
        0.4509, 0.1,
        0.5665, 0.9
    },

    // ICON_CLOSE
    {
        27, 27, 0, 0, 27,
        0.5780, 0.22,
        0.659,  0.78
    },

    // ICON_MOVE
    {
        42, 42, 0, 0, 27,
        0.6647, 0.06,
        0.7919, 0.94
    },

    // ICON_MORE_VERTICAL
    {
        7, 34, 0, 0, 27,
        0.7977, 0.14,
        0.8208, 0.86
    },

    // ICON_BULLET_CIRCLE
    {
        15, 15, 0, 0, 15,
        0.8266, 0.34,
        0.8728, 0.66
    },

    // ICON_IMAGE
    {
        38, 38, 0, 0, 38,
        0.8786, 0.1,
        0.9942, 0.9
    },
};

uint8_t* createFontAtlas(Arena& arena) {
    sort(&charList[0], &charList[charListIndex], [](Char& a, Char& b) {
        if (a.height == b.height) {
            return a.width < b.width;
        }

        return a.height < b.height;
    });

    uint8_t* pngData = (uint8_t*) arenaAllocate(arena, ATLAS_DIMENSION * ATLAS_DIMENSION);

    memset(pngData, 0, ATLAS_DIMENSION * ATLAS_DIMENSION);

    uint16_t max_y = 0;
    uint16_t x = FONT_ATLAS_MARGIN;
    uint16_t y = FONT_ATLAS_MARGIN;

    for (uint32_t i=0; i<charListIndex; ++i) {
        Char c = charList[i];

        if (c.glyph == nullptr || c.height == 0 || c.width == 0) { continue; }

        if (c.height > max_y) {
            max_y = c.height;
        }

        if (x + c.width > ATLAS_DIMENSION) {
            y += max_y + FONT_ATLAS_MARGIN;
            x = FONT_ATLAS_MARGIN;
        }

        c.glyph->textureX1 = static_cast<float>(x)/static_cast<float>(ATLAS_DIMENSION);
        c.glyph->textureY1 = static_cast<float>(y)/static_cast<float>(ATLAS_DIMENSION);
        c.glyph->textureX2 = static_cast<float>(x + c.width)/static_cast<float>(ATLAS_DIMENSION);
        c.glyph->textureY2 = static_cast<float>(y + c.height)/static_cast<float>(ATLAS_DIMENSION);

        for (int i=y; i<y + c.height && i<ATLAS_DIMENSION; ++i) {
            memcpy(
                &pngData[i * ATLAS_DIMENSION + x],
                &(c.bitmap[(i-y) * c.width]),
                c.width
            );
        }

        x += c.width + FONT_ATLAS_MARGIN;
    }

    return pngData;
}

void initCharList(Arena& arena, size_t len) {
    lock_guard<mutex> lock(textInitMutex);

    if (charList == nullptr) {
        cout << "\nDEBUG: reserving capacity for charList.";
        charList = (Char*) arenaAllocate(arena, sizeof(Char) * len * CHARACTER_VARIANTS);
    }
}

void initCharBlock(uint8_t blockIndex) {
    lock_guard<mutex> lock(textInitMutex);

    if (characters[blockIndex] == nullptr) {
        characters[blockIndex] = new Character;

        cout << "\nDEUBG: initializing block: " << (int) blockIndex;

        memset(
            characters[blockIndex]->glyphs,
            0,
            CHARACTER_VARIANTS * CHARACTER_VARIANT_GLYPHS * sizeof(Glyph)
        );
    }
}

void initGlyph(
    string path,
    uint8_t variantIndex,
    Arena& arena
) {
    FT_Library ft;

    if (FT_Init_FreeType(&ft)) { error("Could not init FreeType Library"); exit(1); }

    FT_Face face;
    if (FT_New_Face(ft, path.c_str(), 0, &face)) { error("Failed to load font"); exit(1); }

    FT_Select_Charmap(face, FT_ENCODING_UNICODE);
    FT_Set_Pixel_Sizes(face, 0, 48);

    cout << "\nDEBUG: Total characters in font " << path << ": " << face->num_glyphs;

    if (charList == nullptr) {
        initCharList(arena, face->num_glyphs);
    }

    uint32_t charcode;
    uint32_t id;

    charcode = FT_Get_First_Char(face, &id);

    if (glyphDescender == 0.0f && variantIndex == 0) {
        glyphDescender = static_cast<float>(face->size->metrics.descender >> 6) / DEFAULT_FONT_SIZE;
    }

    while (id != 0) {
        uint8_t mapIndex = charcode >> 8;
        uint8_t charIndex = static_cast<uint8_t>(charcode);

        if (characters[mapIndex] == nullptr) {
            initCharBlock(mapIndex);
        }

        if (charcode == 32) {
            float advance = 13.0f;

            switch (variantIndex) {
                case CHARACTER_REGULAR: advance = 13.0f; break;
                case CHARACTER_BOLD: advance = 10.0f; break;
                case CHARACTER_ITALIC: advance = 12.0f; break;
                case CHARACTER_MONO: advance = 29.0f; break;
            }

            characters[mapIndex]->glyphs[variantIndex][charIndex].advance = advance;
            charcode = FT_Get_Next_Char(face, charcode, &id);
            continue;
        }

        if (FT_Load_Char(face, charcode, FT_LOAD_RENDER)) { errorv("Failed to load glyph"); }

        uint8_t width = face->glyph->bitmap.width;
        uint8_t height = face->glyph->bitmap.rows;

        characters[mapIndex]->glyphs[variantIndex][charIndex] = {
            static_cast<uint8_t>(width),
            static_cast<uint8_t>(height),
            static_cast<int8_t>(face->glyph->bitmap_left),
            static_cast<uint8_t>(face->glyph->bitmap_top),
            static_cast<float>(face->glyph->advance.x * 0.015625), // divide by 64
            0.0f,
            0.0f,
            0.0f,
            0.0f,
        };

        if (width != 0 && height != 0) {
            uint32_t index = charListIndex.fetch_add(1, std::memory_order_relaxed);

            charList[index] = {
                width, height,
                &characters[mapIndex]->glyphs[variantIndex][charIndex],
                (uint8_t*) arenaAllocate(arena, width * height)
            };

            memcpy(charList[index].bitmap, face->glyph->bitmap.buffer, width * height);
        }

        charcode = FT_Get_Next_Char(face, charcode, &id);
    }

    FT_Done_Face(face);
    FT_Done_FreeType(ft);
}

void initFonts(Arena& arena) {
    memset(characters, 0, sizeof(characters));

    thread regular(
        initGlyph,
        "assets/fonts/poppins/Poppins-Regular.ttf",
        CHARACTER_REGULAR,
        std::ref(arena)
    );

    thread bold(
        initGlyph,
        "assets/fonts/poppins/Poppins-Bold.ttf",
        CHARACTER_BOLD,
        std::ref(arena)
    );

    thread italic(
        initGlyph,
        "assets/fonts/poppins/Poppins-Italic.ttf",
        CHARACTER_ITALIC,
        std::ref(arena)
    );

    thread mono(
        initGlyph,
        "assets/fonts/firacode/FiraCode-Regular.ttf",
        CHARACTER_MONO,
        std::ref(arena)
    );

    #ifdef _WIN32
        SetThreadDescription(regular.native_handle(), L"initGlyph:regular");
        SetThreadDescription(bold.native_handle(), L"initGlyph:bold");
        SetThreadDescription(italic.native_handle(), L"initGlyph:italic");
        SetThreadDescription(mono.native_handle(), L"initGlyph:mono");
    #endif

    regular.join();
    bold.join();
    italic.join();
    mono.join();
}

void initFontTexture(uint8_t* pngData) {
    #ifdef DEBUG_TEXT
        cout << "\nDEBUG: STARTUP CHECK: loading font texture";
    #endif

    if (pngData == nullptr) { error("\nERROR: no font texture provided!"); exit(1); }

    glGenTextures(1, &fontTextureID);
    glBindTexture(GL_TEXTURE_2D, fontTextureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, ATLAS_DIMENSION, ATLAS_DIMENSION, 0, GL_RED, GL_UNSIGNED_BYTE, pngData);
    glGenerateMipmap(GL_TEXTURE_2D);
}

void initIconTexture() {
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

        Glyph moveIcon = icons[ICON_MOVE];
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
    glUseProgram(shaders[SHADER_TEXT]);

    glGenVertexArrays(1, &textVAO);
    glGenBuffers(1, &textVBO);
    glBindVertexArray(textVAO);
    glBindBuffer(GL_ARRAY_BUFFER, textVBO);
    glBufferData(GL_ARRAY_BUFFER, 1200, NULL, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(TextVertex), 0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(TextVertex), (void*) (2 * sizeof(float)));
    glVertexAttribIPointer(2, 1, GL_UNSIGNED_INT, sizeof(TextVertex), (void*) (4 * sizeof(float)));\
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    glUseProgram(shaders[SHADER_TEXT]);
}

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
    float leftBound,
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
        xpos = leftBound;
        rows++;

        if (yNet > windowHeight) {
            renderNextContentBlock = false;
        }
    }

    ypos = yNet + glyphHeight - bearingTop + fontSize;

    height = ypos - y - (glyphDescender * fontSize);

    xpos_plus_w = xpos + glyphWidth;
    ypos_plus_h = ypos - glyphHeight;

    advance = c.advance * fontHeightRatio - bearingLeft;

    texX1 = c.textureX1;
    texX2 = c.textureX2;
    texY1 = c.textureY1;
    texY2 = c.textureY2;

    return true;
}

void drawTextBuffers(
    uint32_t& vao,
    uint32_t& vbo,
    uint32_t& bufferSize,
    uint32_t& numVertices,
    vector<TextVertex>& vertices,
    float time,
    bool redraw,
    uint32_t texture
) {
    glUseProgram(shaders[SHADER_TEXT]);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(vao);
    glBindTexture(GL_TEXTURE_2D, texture);

    if (redraw && !vertices.empty()) {
        glBindBuffer(GL_ARRAY_BUFFER, vbo);

        numVertices = vertices.size();
        uint32_t requiredBufferSize = numVertices * sizeof(TextVertex);

        if (bufferSize >= requiredBufferSize) {
            uint32_t zeros = (bufferSize - requiredBufferSize) / sizeof(TextVertex);

            for (uint32_t i=0; i<zeros; ++i) { vertices.push_back(TextVertex { 0 }); }

            glBufferSubData(GL_ARRAY_BUFFER, 0, bufferSize, &vertices[0]);
        } else {
            bufferSize = requiredBufferSize;
            glBufferData(GL_ARRAY_BUFFER, bufferSize, &vertices[0], GL_DYNAMIC_DRAW);
        }
    }

    glUniform1f(uniforms[U_TEXT_TIME], time);

    glDrawArrays(GL_TRIANGLES, 0, numVertices);
}

RopeRenderResult getTextDimensions(
    string str, float x, float y, uint8_t variant, uint8_t fontSize,
    float parentLeft, float parentWidth, bool breakLine
) {
    RopeRenderResult result = RopeRenderResult { x, y, 0.0f, 0.0f, 0.0f, 0.0f, 0, true };

    float xpos = x;
    float ypos = 0.0f;
    float yNet = y;
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
            characters[c >> 8]->glyphs[variant][static_cast<uint8_t>(c)],
            x,
            y,
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
            parentLeft,
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

    result.width = xpos - x;

    if (result.rows) {
        ++result.rows;
    }

    return result;
}

TextRenderResult getInputTextDimensions(
    string str, TextInputData* tid, float x, float y, uint8_t variant,
    uint8_t fontSize, float parentLeft, float parentWidth, bool breakLine
) {
    TextRenderResult result = TextRenderResult {
        x, y, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0, 0, true
    };

    float xpos = x;
    float ypos = 0.0f;
    float yNet = y;
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
            characters[c >> 8]->glyphs[variant][static_cast<uint8_t>(c)],
            x,
            y,
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
            parentLeft,
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

    result.width = xpos - x;

    if (result.rows) {
        ++result.rows;
    }

    result.columns = i;

    return result;
}

float appendCharVertices(
    float x,
    float y,
    Glyph c,
    uint32_t colorIndex,
    vector<TextVertex>& vertices
) {
    float xpos = x, ypos = x, yNet = y, xpos_plus_w, ypos_plus_h, texX1, texX2, texY1, texY2,
        glyphHeight, glyphWidth, bearingTop, bearingLeft, advance = 0, totalHeight;
    float infinity = numeric_limits<float>::infinity();
    uint32_t rows;
    bool renderNextContentBlock;

    getCharParams(
        c,
        x,
        y,
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
        0,
        infinity,
        rows,
        16,
        renderNextContentBlock
    );

    vertices.insert(vertices.end(), {
        TextVertex { xpos,           ypos_plus_h, texX1, texY1, colorIndex },
        TextVertex { xpos,           ypos,        texX1, texY2, colorIndex },
        TextVertex { xpos_plus_w,    ypos,        texX2, texY2, colorIndex },
        TextVertex { xpos,           ypos_plus_h, texX1, texY1, colorIndex },
        TextVertex { xpos_plus_w,    ypos,        texX2, texY2, colorIndex },
        TextVertex { xpos_plus_w,    ypos_plus_h, texX2, texY1, colorIndex },
    });

    return glyphWidth;
}

RopeRenderResult appendTextVertices(
    float x,
    float y,
    string text,
    uint8_t variant,
    uint32_t colorIndex,
    uint8_t fontSize,
    vector<TextVertex>& vertices
) {
    float xpos = x, ypos = x, yNet = y, xpos_plus_w, ypos_plus_h, texX1, texX2, texY1, texY2,
        glyphHeight, glyphWidth, bearingTop, bearingLeft, advance = 0, totalHeight = 0;
    float infinity = numeric_limits<float>::infinity();

    uint32_t rows = 0;

    bool renderNextContentBlock = false;

    for (char c: text) {
        // appendCharVertices(x, y, characters[(int)c], vertices);
        getCharParams(
            characters[c >> 8]->glyphs[variant][static_cast<uint8_t>(c)],
            x,
            y,
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
            0,
            infinity,
            rows,
            fontSize,
            renderNextContentBlock
        );

        vertices.insert(vertices.end(), {
            TextVertex { xpos,           ypos_plus_h, texX1, texY1, colorIndex },
            TextVertex { xpos,           ypos,        texX1, texY2, colorIndex },
            TextVertex { xpos_plus_w,    ypos,        texX2, texY2, colorIndex },
            TextVertex { xpos,           ypos_plus_h, texX1, texY1, colorIndex },
            TextVertex { xpos_plus_w,    ypos,        texX2, texY2, colorIndex },
            TextVertex { xpos_plus_w,    ypos_plus_h, texX2, texY1, colorIndex },
        });

        xpos += advance;
    }

    return { x, y, totalHeight, xpos - x, xpos, ypos, rows, renderNextContentBlock };
}

RopeRenderResult appendRopeVertices(
    float x,
    float y,
    Rope* rope,
    uint8_t variant,
    uint32_t startIndex,
    uint32_t length,
    uint8_t fontSize,
    uint32_t colorIndex,
    vector<TextVertex>& vertices
) {
    RopeRenderResult result = RopeRenderResult { x, y, 0.0f, 0.0f, 0.0f, 0.0f, 0, true };

    float xpos = x, ypos = y, yNet = y, xpos_plus_w, ypos_plus_h, texX1, texX2, texY1, texY2,
        glyphHeight, glyphWidth, bearingTop, bearingLeft, advance = 0;

    RopeTraverser rt(rope);

    uint32_t i = 0;
    uint32_t appendedLength = 0;

    while (rt.hasNext()) {
        Rope* node = rt.next();

        for (char c: node->text) {
            if (i < startIndex) {
                ++i;
                continue;
            }

            getCharParams(
                characters[c >> 8]->glyphs[variant][static_cast<uint8_t>(c)],
                x,
                y,
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
                contentLeft,
                contentRight,
                result.rows,
                fontSize,
                result.renderNextContentBlock
            );

            ++appendedLength;

            if (c != ' ') {
                vertices.insert(vertices.end(), {
                    TextVertex { xpos,        ypos_plus_h, texX1, texY1, colorIndex },
                    TextVertex { xpos,        ypos,        texX1, texY2, colorIndex },
                    TextVertex { xpos_plus_w, ypos,        texX2, texY2, colorIndex },
                    TextVertex { xpos,        ypos_plus_h, texX1, texY1, colorIndex },
                    TextVertex { xpos_plus_w, ypos,        texX2, texY2, colorIndex },
                    TextVertex { xpos_plus_w, ypos_plus_h, texX2, texY1, colorIndex },
                });
            }

            xpos += advance;

            if (appendedLength == length) {
                break;
            }
        }
    }

    ++result.rows;

    if (result.rows == 1) {
        result.width = xpos - x;
    } else if (result.rows > 1) {
        result.width = contentRight - contentLeft;
    }

    result.endX = xpos;
    result.endY = yNet;

    return result;
}

