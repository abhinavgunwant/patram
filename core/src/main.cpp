#include "include/argparser.hpp"
#include "include/window-utils.hpp"
#include "include/setup-ui.hpp"
#include "ui.hpp"
#include "utils.hpp"
#include "files.hpp"

#include "shader.hpp"

#define TRACY_ENABLE

using namespace std;

int main(int argc, char ** argv) {
    switch (parseArgs(argc, argv)) {
        case CMD_LINE_VERSION: displayVersion(); return 0;
        case CMD_LINE_ERROR: displayError(); return 1;
        case CMD_LINE_HELP: displayHelp(); return 0;

        case CMD_LINE_RUN_DARK: darkTheme = true; break;

        case CMD_LINE_FPS:
            showFps = true;
            darkTheme = isDarkMode();
            break;

        case CMD_LINE_RUN:
            darkTheme = false;
            break;

        case CMD_LINE_NONE:
            darkTheme = isDarkMode();
            break;
    }

    if (!cacheDirectoryExists()) {
        createCacheDirectory();
    }

    showWindow();
    initShaders();
    setupUI(darkTheme);
    enterRenderLoop();

    return 0;
}

