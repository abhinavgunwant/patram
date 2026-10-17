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

