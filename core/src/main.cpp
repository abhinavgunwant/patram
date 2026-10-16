#include "include/argparser.hpp"
#include "include/startup-checks.hpp"

#include "ui.hpp"
#include "logger.hpp"
#include "arena.hpp"

int main(int argc, char ** argv) {
    initLogging();

    processCmdLineArgs(argc, argv);

    Arena startupArena;
    uint8_t* pngData = runStartupChecks(startupArena);

    showWindow();
    setupUI(darkTheme, startupArena, pngData);

    destroyArena(startupArena);

    enterRenderLoop();

    destroyLogging();

    return 0;
}

