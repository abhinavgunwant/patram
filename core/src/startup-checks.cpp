#include <string>
#include <chrono>

using namespace std;

#include <ft2build.h>
#include FT_FREETYPE_H
#include "include/startup-checks.hpp"
#include "text.hpp"
#include "utils.hpp"
#include "files.hpp"
#include "logger.hpp"
#include "arena.hpp"

void initFonts(Arena& arena) {
    characters = new Character[MAX_CHARACTERS];

    #ifdef DEBUG_TEXT
        cout << "\nSTARTUP CHECK: Initializing font cache.";
    #endif

    FT_Library ft;

    if (FT_Init_FreeType(&ft)) {
        error("FREETYPE: Could not init FreeType Library");
        return;
    }

    // load font as face
    FT_Face face;
    if (FT_New_Face(ft, POPPINS_REGULAR, 0, &face)) {
        error("FREETYPE: Failed to load font");
        return;
    }

    FT_Select_Charmap(face, FT_ENCODING_UNICODE);
    FT_Set_Pixel_Sizes(face, 0, DEFAULT_FONT_SIZE);

    #ifdef DEBUG_TEXT
        cout << "\nDEBUG: Total characters: " << face->num_glyphs;
    #endif

    chvec.reserve(face->num_glyphs);

    uint32_t charcode;
    uint32_t id;

    charcode = FT_Get_First_Char(face, &id);
    glyphDescender = static_cast<float>(face->size->metrics.descender >> 6) / DEFAULT_FONT_SIZE;

    memset(characters, 0, MAX_CHARACTERS);

    while (id != 0) {
        if (FT_Load_Char(face, charcode, FT_LOAD_RENDER)) {
            error("FREETYPE: Failed to load glyph");
            return;
        }
        uint8_t width = face->glyph->bitmap.width;
        uint8_t height = face->glyph->bitmap.rows;

        characters[charcode] = {
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

        Char cccc = { static_cast<uint16_t>(charcode), width, height, nullptr };

        cccc.bitmap = (uint8_t*) arenaAllocate(arena, width * height); // new uint8_t[width * height];

        memcpy(cccc.bitmap, face->glyph->bitmap.buffer, width * height);

        chvec.push_back(cccc);

        charcode = FT_Get_Next_Char(face, charcode, &id);
    }

    FT_Done_Face(face);
    FT_Done_FreeType(ft);
}

uint8_t* createFontAtlas(Arena& arena) {
    sort(chvec.begin(), chvec.end(), [](Char& a, Char& b) {
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

    for (vector<Char>::iterator itr = chvec.begin(); itr != chvec.end(); ++itr) {
        if (itr->height == 0 || itr->width == 0) {
            continue;
        }

        if (itr->height > max_y) {
            max_y = (*itr).height;
        }

        if (x + itr->width > ATLAS_DIMENSION) {
            y += max_y + FONT_ATLAS_MARGIN;
            x = FONT_ATLAS_MARGIN;
        }

        characters[itr->charcode].textureX1 = static_cast<float>(x)/static_cast<float>(ATLAS_DIMENSION);
        characters[itr->charcode].textureY1 = static_cast<float>(y)/static_cast<float>(ATLAS_DIMENSION);
        characters[itr->charcode].textureX2 = static_cast<float>(x + itr->width)/static_cast<float>(ATLAS_DIMENSION);
        characters[itr->charcode].textureY2 = static_cast<float>(y + itr->height)/static_cast<float>(ATLAS_DIMENSION);

        for (int i=y; i<y + itr->height && i<ATLAS_DIMENSION; ++i) {
            memcpy(
                &pngData[i * ATLAS_DIMENSION + x],
                &((*itr).bitmap[(i-y) * itr->width]),
                itr->width
            );
        }

        x += itr->width + FONT_ATLAS_MARGIN;
    }

    return pngData;
}

uint8_t* runStartupChecks(Arena& arena) {
    initArena(arena, ATLAS_DIMENSION * ATLAS_DIMENSION * 2);

    auto t0 = std::chrono::high_resolution_clock::now();

    initFonts(arena);
    uint8_t* ptr = createFontAtlas(arena);

    #ifdef DEBUG_TEXT
    cout << "\nDEBUG: Took "
        << std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::high_resolution_clock::now() - t0
        )
        << " to initialize fonts.";
    #endif

    return ptr;
}

