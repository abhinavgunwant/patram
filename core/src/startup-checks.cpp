#include <vector>
#include <string>
#include <chrono>

using namespace std;

#include <ft2build.h>
#include FT_FREETYPE_H
#include "include/startup-checks.hpp"
#include "text.hpp"
#include "utils.hpp"
#include "files.hpp"

bool initFonts(string fontPath, bool cache) {
    characters = new Character[MAX_CHARACTERS];

    if (cache) {
        #ifdef DEBUG_TEXT
            cout << "\nSTARTUP CHECK: Initializing fonts from cache.";
        #endif

        if (processFontInfoFile(characters)) {
            return true;
        }
    }

    #ifdef DEBUG_TEXT
        cout << "\nSTARTUP CHECK: Initializing font cache.";
    #endif

    FT_Library ft;

    if (FT_Init_FreeType(&ft)) {
        std::cerr << "Error: FREETYPE: Could not init FreeType Library" << std::endl;
        return false;
    }

    // load font as face
    FT_Face face;
    if (FT_New_Face(ft, fontPath.c_str(), 0, &face)) {
        std::cout << "Error: FREETYPE: Failed to load font" << std::endl;
        return false;
    }

    FT_Select_Charmap(face, FT_ENCODING_UNICODE);
    FT_Set_Pixel_Sizes(face, 0, DEFAULT_FONT_SIZE);

    // disable byte-alignment restriction
    // glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    #ifdef DEBUG_TEXT
        cout << "\nDEBUG: Total characters: " << face->num_glyphs;
    #endif

    chvec.reserve(face->num_glyphs);

    uint32_t charcode;
    uint32_t id;

    charcode = FT_Get_First_Char(face, &id);

    while (id != 0) {
        if (FT_Load_Char(face, charcode, FT_LOAD_RENDER)) {
            std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
            return false;
        }
        uint8_t width = face->glyph->bitmap.width;
        uint8_t height = face->glyph->bitmap.rows;

        characters[charcode] = {
            static_cast<uint8_t>(width),
            static_cast<uint8_t>(height),
            static_cast<int8_t>(face->glyph->bitmap_left),
            static_cast<uint8_t>(face->glyph->bitmap_top),
            0,
            static_cast<uint16_t>(face->glyph->advance.x),
            0.0f,
            0.0f,
            0.0f,
            0.0f,
        };

        Char cccc = { charcode, width, height, nullptr };

        cccc.bitmap = new uint8_t[width * height];

        for (int i=0; i<width * height; ++i) {
            cccc.bitmap[i] = face->glyph->bitmap.buffer[i];
        }

        chvec.push_back(cccc);

        charcode = FT_Get_Next_Char(face, charcode, &id);
    }

    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    return true;
}

void runStartupChecks() {
    if (
        DIR_EXISTS(getCacheDirectoryPath())
        && FILE_EXISTS(getCacheIndexPath())
        && FILE_EXISTS(getFontInfoPath(0))
        && FILE_EXISTS(getFontAtlasPath(0))
    ) {
        auto t0 = std::chrono::high_resolution_clock::now();

        bool success = initFonts(POPPINS_REGULAR, true);

        auto t1 = std::chrono::high_resolution_clock::now();

        if (success) {
            auto duration0 = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0);
            #ifdef DEBUG_TEXT
                cout << "\nSTARTUP CHECK: Font initialization from cache took " << duration0.count() << " microseconds.";
            #endif
            return;
        }
    } else {
        #ifdef DEBUG_TEXT
            cout << "\nSTARTUP CHECK: Creating cache";
        #endif

        if (!DIR_EXISTS(getCacheDirectoryPath())) {
            createCacheDirectory();
        }

        // TODO: As of now, just a single standard poppins font is being used, use
        // more and make the user configured default in future!
        vector<string> fontFilePaths;
        fontFilePaths.reserve(1);
        fontFilePaths.push_back(POPPINS_REGULAR);

        #ifdef DEBUG_TEXT
            auto t0 = std::chrono::high_resolution_clock::now();
        #endif

        initFonts(POPPINS_REGULAR, false);

        #ifdef DEBUG_TEXT
        auto t1 = std::chrono::high_resolution_clock::now();
        #endif

        createCacheIndex(fontFilePaths);

        #ifdef DEBUG_TEXT
        auto t2 = std::chrono::high_resolution_clock::now();
        #endif

        createFontAtlasFile(0, characters);

        #ifdef DEBUG_TEXT
            auto t3 = std::chrono::high_resolution_clock::now();
        #endif

        createFontInfoFile(0, characters, MAX_CHARACTERS);

        #ifdef DEBUG_TEXT
            auto t4 = std::chrono::high_resolution_clock::now();
        #endif

        #ifdef DEBUG_TEXT
            auto duration0 = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0);
            auto duration1 = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1);
            auto duration2 = std::chrono::duration_cast<std::chrono::microseconds>(t3 - t2);
            auto duration3 = std::chrono::duration_cast<std::chrono::microseconds>(t4 - t3);

            cout << "\nSTARTUP CHECK: Font initialization took " << duration0.count() << " microseconds.";
            cout << "\nSTARTUP CHECK: Cache index creation took " << duration1.count() << " microseconds.";
            cout << "\nSTARTUP CHECK: Font Atlas creation took " << duration2.count() << " microseconds.";
            cout << "\nSTARTUP CHECK: Font info file creation took " << duration3.count() << " microseconds.";
        #endif

        chvec.resize(0);
    }
}

