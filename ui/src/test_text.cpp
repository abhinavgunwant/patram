// #include "text.hpp"
#include <iostream>
#include <chrono>
#include <thread>
#include <algorithm>

using namespace std;

#include <ft2build.h>
#include FT_FREETYPE_H

#include "arena.hpp"
#include "../lib/stb_image_write.h"

#define MAX_CHARACTERS      65536
#define ATLAS_DIMENSION     1400
#define FONT_ATLAS_MARGIN   2
#define DEFAULT_FONT_SIZE   16

struct TextVertex {
    float x;
    float y;
    float s;
    float t;
    uint32_t colorIndex;
};

struct Character {
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
    uint8_t variant;
    uint16_t charcode;
    uint8_t width;
    uint8_t height;
    uint8_t* bitmap;
};

struct GlyphVariant {
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

#define CHARACTER_BLOCK_REGULAR 0
#define CHARACTER_BLOCK_BOLD    1
#define CHARACTER_BLOCK_ITALIC  2

struct CharacterBlock {
    GlyphVariant glyphVariants[3][256];
};

struct Characters {
    CharacterBlock* blocks[256];
};

float glyphDescender = -0.33f;
Character* oldChars = new Character[MAX_CHARACTERS];
Characters chars;

inline bool getCharParamsOld(
    Character& c,
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
    uint8_t fontSize
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
        yNet += static_cast<float>(fontSize) * (1.0f - glyphDescender);
        xpos = x;
        rows++;
    }

    ypos = yNet + glyphHeight - bearingTop + fontSize;

    if (glyphDescender < 0) {
        height = ypos - y - (glyphDescender * fontSize);
    } else {
        height = ypos - y;
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

void appendVerticesOld(float x, float y, string text, uint8_t fontSize) {
    float xpos = x, ypos = x, yNet = y, xpos_plus_w, ypos_plus_h, texX1, texX2, texY1, texY2,
        glyphHeight, glyphWidth, bearingTop, bearingLeft, advance = 0, totalHeight = 0;
    float infinity = numeric_limits<float>::infinity();

    uint32_t rows = 0;

    vector<TextVertex> tv;
    tv.reserve(100);

    for (char c: text) {
        getCharParamsOld(
            oldChars[(int)c],
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
            infinity,
            rows,
            fontSize
        );

        tv.insert(tv.end(), {
            TextVertex { xpos,           ypos_plus_h, texX1, texY1, 0 },
            TextVertex { xpos,           ypos,        texX1, texY2, 0 },
            TextVertex { xpos_plus_w,    ypos,        texX2, texY2, 0 },
            TextVertex { xpos,           ypos_plus_h, texX1, texY1, 0 },
            TextVertex { xpos_plus_w,    ypos,        texX2, texY2, 0 },
            TextVertex { xpos_plus_w,    ypos_plus_h, texX2, texY1, 0 },
        });

        xpos += advance;
    }
}

void oldTest() {
    Arena arena;
    initArena(arena, ATLAS_DIMENSION * ATLAS_DIMENSION * 2);

    vector<Char> chvec;

    FT_Library ft;

    if (FT_Init_FreeType(&ft)) {
        cout << "FREETYPE: Could not init FreeType Library";
    }

    // load font as face
    FT_Face face;
    if (FT_New_Face(ft, "assets/fonts/poppins/Poppins-Regular.ttf", 0, &face)) {
        cout << "FREETYPE: Failed to load font";
    }

    FT_Select_Charmap(face, FT_ENCODING_UNICODE);
    FT_Set_Pixel_Sizes(face, 0, 48);

    cout << "\nDEBUG: Total characters: " << face->num_glyphs;

    chvec.reserve(face->num_glyphs);

    uint32_t charcode;
    uint32_t id;

    charcode = FT_Get_First_Char(face, &id);
    float glyphDescender = static_cast<float>(face->size->metrics.descender >> 6) / 48;

    memset(oldChars, 0, MAX_CHARACTERS);

    while (id != 0) {
        if (FT_Load_Char(face, charcode, FT_LOAD_RENDER)) {
            cout << "FREETYPE: Failed to load glyph";
        }
        uint8_t width = face->glyph->bitmap.width;
        uint8_t height = face->glyph->bitmap.rows;

        oldChars[charcode] = {
            static_cast<uint8_t>(width),
            static_cast<uint8_t>(height),
            static_cast<int8_t>(face->glyph->bitmap_left),
            static_cast<uint8_t>(face->glyph->bitmap_top),
            static_cast<uint16_t>(face->glyph->advance.x),
            0.0f,
            0.0f,
            0.0f,
            0.0f,
        };

        Char cccc = { 0, static_cast<uint16_t>(charcode), width, height, nullptr };

        cccc.bitmap = (uint8_t*) arenaAllocate(arena, width * height);

        memcpy(cccc.bitmap, face->glyph->bitmap.buffer, width * height);

        chvec.push_back(cccc);

        charcode = FT_Get_Next_Char(face, charcode, &id);
    }

    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    sort(chvec.begin(), chvec.end(), [](Char& a, Char& b) {
        if (a.height == b.height) {
            return a.width < b.width;
        }

        return a.height < b.height;
    });

    uint8_t* png_data = (uint8_t*) arenaAllocate(arena, ATLAS_DIMENSION * ATLAS_DIMENSION); // new uint8_t[ATLAS_DIMENSION * ATLAS_DIMENSION];

    memset(png_data, 0, ATLAS_DIMENSION * ATLAS_DIMENSION);

    uint16_t max_y = 0;
    uint16_t x = FONT_ATLAS_MARGIN;
    uint16_t y = FONT_ATLAS_MARGIN;

    for (vector<Char>::iterator itr = chvec.begin(); itr != chvec.end(); ++itr) {
        if (itr->height == 0 || itr->width == 0) {
            continue;
        }

        if (itr->height > max_y) {
            max_y = itr->height;
        }

        if (x + itr->width > ATLAS_DIMENSION) {
            y += max_y + FONT_ATLAS_MARGIN;
            x = FONT_ATLAS_MARGIN;
        }

        oldChars[itr->charcode].textureX1 = static_cast<float>(x)/static_cast<float>(ATLAS_DIMENSION);
        oldChars[itr->charcode].textureY1 = 1.0f - static_cast<float>(y)/static_cast<float>(ATLAS_DIMENSION);
        oldChars[itr->charcode].textureX2 = static_cast<float>(x + itr->width)/static_cast<float>(ATLAS_DIMENSION);
        oldChars[itr->charcode].textureY2 = 1.0f - static_cast<float>(y + itr->height)/static_cast<float>(ATLAS_DIMENSION);

        for (int i=y; i<y + itr->height && i<ATLAS_DIMENSION; ++i) {
            memcpy(
                &png_data[i * ATLAS_DIMENSION + x],
                &(itr->bitmap[(i-y) * itr->width]),
                itr->width
            );
        }

        x += itr->width + FONT_ATLAS_MARGIN;
    }

    // stbi_write_png("oldTest.png", ATLAS_DIMENSION, ATLAS_DIMENSION, 1, png_data, ATLAS_DIMENSION);

    destroyArena(arena);
}

atomic<bool> blocked = false;

void initGlyphVariants(
    string path,
    uint32_t variantIndex,
    vector<Char>& chvec,
    Arena& arena
) {
    FT_Library ft;

    if (FT_Init_FreeType(&ft)) {
        cout << "FREETYPE: Could not init FreeType Library";
    }

    // load font as face
    FT_Face face;
    if (FT_New_Face(ft, path.c_str(), 0, &face)) {
        cout << "FREETYPE: Failed to load font";
    }

    FT_Select_Charmap(face, FT_ENCODING_UNICODE);
    FT_Set_Pixel_Sizes(face, 0, 48);

    cout << "\nDEBUG: Total characters in font " << path << ": " << face->num_glyphs;

    if (chvec.capacity() == 0) {
        cout << "\nDEBUG: reserving capacity for chvec.";
        while (blocked) {}
        blocked = true;

        chvec.reserve(face->num_glyphs * 3);

        blocked = false;
    }

    uint32_t charcode;
    uint32_t id;

    charcode = FT_Get_First_Char(face, &id);
    float glyphDescender = static_cast<float>(face->size->metrics.descender >> 6) / 48;

    while (id != 0) {
        while (blocked) {}

        uint8_t mapIndex = charcode >> 8;
        uint8_t charIndex = static_cast<uint8_t>(charcode);
        // cout << "\n! " << (uint32_t)variantIndex << " " << charcode;

        if (chars.blocks[mapIndex] == nullptr) {
            blocked = true;
            chars.blocks[mapIndex] = new CharacterBlock;

            memset(chars.blocks[mapIndex]->glyphVariants, 0, 256 * 3);
            blocked = false;
        }

        if (FT_Load_Char(face, charcode, FT_LOAD_RENDER)) { cout << "FREETYPE: Failed to load glyph"; }

        uint8_t width = face->glyph->bitmap.width;
        uint8_t height = face->glyph->bitmap.rows;

        chars.blocks[mapIndex]->glyphVariants[variantIndex][charIndex] = {
            static_cast<uint8_t>(width),
            static_cast<uint8_t>(height),
            static_cast<int8_t>(face->glyph->bitmap_left),
            static_cast<uint8_t>(face->glyph->bitmap_top),
            static_cast<uint16_t>(face->glyph->advance.x),
            0.0f,
            0.0f,
            0.0f,
            0.0f,
        };

        Char cccc = {
            static_cast<uint8_t>(variantIndex),
            static_cast<uint16_t>(charcode), width, height, nullptr
        };

        cccc.bitmap = (uint8_t*)arenaAllocate(arena, width * height);

        memcpy(cccc.bitmap, face->glyph->bitmap.buffer, width * height);

        chvec.push_back(cccc);

        charcode = FT_Get_Next_Char(face, charcode, &id);
    }

    FT_Done_Face(face);
    FT_Done_FreeType(ft);
}

inline bool getCharParamsNew(
    uint16_t charcode,
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
    uint8_t fontSize
) {
    GlyphVariant glyph = chars.blocks[charcode >> 8]->glyphVariants[0][static_cast<uint8_t>(charcode)];
    float aspectRatio = static_cast<float>(glyph.width) / static_cast<float>(glyph.height);
    float fontHeightRatio = static_cast<float>(fontSize) / DEFAULT_FONT_SIZE;
    glyphHeight = static_cast<float>(glyph.height) * fontHeightRatio;
    glyphWidth = glyphHeight * aspectRatio;

    if (glyphWidth != glyphWidth) {
        if (glyph.width != 0) {
            glyphWidth = static_cast<float>(glyph.width) / static_cast<float>(fontSize);
        } else {
            glyphWidth = 0.0f;
        }
    }

    float widthRatio = glyphWidth / glyph.width;
    float heightRatio = glyphHeight / glyph.height;
    bearingTop = glyph.bearingTop * heightRatio;
    bearingLeft = glyph.bearingLeft * widthRatio;

    if (bearingLeft != bearingLeft) {
        bearingLeft = 0.0f;
    }

    if (bearingTop != bearingTop) {
        bearingTop = 0.0f;
    }

    xpos += bearingLeft;

    if (xpos + glyphWidth > rightBound) {
        yNet += static_cast<float>(fontSize) * (1.0f - glyphDescender);
        xpos = x;
        rows++;
    }

    ypos = yNet + glyphHeight - bearingTop + fontSize;

    if (glyphDescender < 0) {
        height = ypos - y - (glyphDescender * fontSize);
    } else {
        height = ypos - y;
    }

    xpos_plus_w = xpos + glyphWidth;
    ypos_plus_h = ypos - glyphHeight;

    advance = (static_cast<uint32_t>(glyph.advance * fontHeightRatio) >> 6) - bearingLeft;

    texX1 = glyph.textureX1;
    texX2 = glyph.textureX2;
    texY1 = glyph.textureY1;
    texY2 = glyph.textureY2;

    return true;
}

void appendVerticesNew(float x, float y, string text, uint8_t fontSize) {
    float xpos = x, ypos = x, yNet = y, xpos_plus_w, ypos_plus_h, texX1, texX2, texY1, texY2,
        glyphHeight, glyphWidth, bearingTop, bearingLeft, advance = 0, totalHeight = 0;
    float infinity = numeric_limits<float>::infinity();

    uint32_t rows = 0;

    vector<TextVertex> tv;
    tv.reserve(100);

    for (char c: text) {
        getCharParamsNew(
            static_cast<uint16_t>(c),
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
            infinity,
            rows,
            fontSize
        );

        tv.insert(tv.end(), {
            TextVertex { xpos,           ypos_plus_h, texX1, texY1, 0 },
            TextVertex { xpos,           ypos,        texX1, texY2, 0 },
            TextVertex { xpos_plus_w,    ypos,        texX2, texY2, 0 },
            TextVertex { xpos,           ypos_plus_h, texX1, texY1, 0 },
            TextVertex { xpos_plus_w,    ypos,        texX2, texY2, 0 },
            TextVertex { xpos_plus_w,    ypos_plus_h, texX2, texY1, 0 },
        });

        xpos += advance;
    }
}

void newTest() {
    for (uint32_t i=0; i<256; ++i) {
        chars.blocks[i] = nullptr;
    }

    vector<Char> chvec;

    Arena arena;
    initArena(arena, ATLAS_DIMENSION * ATLAS_DIMENSION * 2);

    thread regular(
        initGlyphVariants,
        "assets/fonts/poppins/Poppins-Regular.ttf",
        0,
        std::ref(chvec),
        std::ref(arena)
    );

    thread bold(
        initGlyphVariants,
        "assets/fonts/poppins/Poppins-Bold.ttf",
        1,
        std::ref(chvec),
        std::ref(arena)
    );

    thread italic(
        initGlyphVariants,
        "assets/fonts/poppins/Poppins-Italic.ttf",
        2,
        std::ref(chvec),
        std::ref(arena)
    );

    regular.join();
    bold.join();
    italic.join();

    sort(chvec.begin(), chvec.end(), [](Char& a, Char& b) {
        if (a.height == b.height) {
            return a.width < b.width;
        }

        return a.height < b.height;
    });

    uint8_t* png_data = (uint8_t*)arenaAllocate(arena, ATLAS_DIMENSION * ATLAS_DIMENSION);

    memset(png_data, 0, ATLAS_DIMENSION * ATLAS_DIMENSION);

    uint16_t max_y = 0;
    uint16_t x = FONT_ATLAS_MARGIN;
    uint16_t y = FONT_ATLAS_MARGIN;

    for (vector<Char>::iterator itr = chvec.begin(); itr != chvec.end(); ++itr) {
        if (itr->height == 0 || itr->width == 0) {
            continue;
        }

        if (itr->height > max_y) {
            max_y = itr->height;
        }

        if (x + itr->width > ATLAS_DIMENSION) {
            y += max_y + FONT_ATLAS_MARGIN;
            x = FONT_ATLAS_MARGIN;
        }

        uint8_t blockIndex = itr->charcode >> 8;
        uint8_t charIndex = static_cast<uint8_t>(itr->charcode);

        if (chars.blocks[blockIndex] != nullptr) {
            GlyphVariant* gv = &chars.blocks[blockIndex]->glyphVariants[itr->variant][charIndex];

            gv->textureX1 = static_cast<float>(x)/static_cast<float>(ATLAS_DIMENSION);
            gv->textureY1 = 1.0f - static_cast<float>(y)/static_cast<float>(ATLAS_DIMENSION);
            gv->textureX2 = static_cast<float>(x + itr->width)/static_cast<float>(ATLAS_DIMENSION);
            gv->textureY2 = 1.0f - static_cast<float>(y + itr->height)/static_cast<float>(ATLAS_DIMENSION);

            for (int i=y; i<y + itr->height && i<ATLAS_DIMENSION; ++i) {
                memcpy(
                    &png_data[i * ATLAS_DIMENSION + x],
                    &((*itr).bitmap[(i-y) * itr->width]),
                    itr->width
                );
            }
        }

        x += itr->width + FONT_ATLAS_MARGIN;
    }

    // stbi_write_png("newTest.png", ATLAS_DIMENSION, ATLAS_DIMENSION, 1, png_data, ATLAS_DIMENSION);

    destroyArena(arena);
}

int main() {
    cout << "Testing " << __FILE__ << "\n";

    cout << "\n--- FONT INITIALIZATION ---";

    auto t0 = std::chrono::high_resolution_clock::now();

    oldTest();

    cout << "\nOld test took: "
         << std ::chrono ::duration_cast<std::chrono::microseconds>(
                std::chrono::high_resolution_clock::now() - t0
        );

    t0 = std::chrono::high_resolution_clock::now();

    newTest();

    cout << "\nNew test took: "
         << std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::high_resolution_clock::now() - t0
        );

    cout << "\n--- TEXT RENDERING ---";

    t0 = std::chrono::high_resolution_clock::now();

    appendVerticesOld(100.0f, 100.0f, "This is a very simple test!", 16);

    cout << "\nOld function took: "
         << std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::high_resolution_clock::now() - t0
        );

    t0 = std::chrono::high_resolution_clock::now();

    appendVerticesNew(100.0f, 100.0f, "This is a very simple test!", 16);

    cout << "\nNew function took: "
         << std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::high_resolution_clock::now() - t0
        );

    return 0;
}

