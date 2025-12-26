#include "include/argparser.hpp"
#include "include/window-utils.hpp"
#include "include/setup-ui.hpp"
#include "include/dark-mode.hpp"
#include "ui.hpp"

#include "shader.hpp"

using namespace std;

int main(int argc, char ** argv) {
    switch (parseArgs(argc, argv)) {
        case CMD_LINE_VERSION: displayVersion(); return 0;
        case CMD_LINE_ERROR: displayError(); return 1;
        case CMD_LINE_HELP: displayHelp(); return 0;

        case CMD_LINE_RUN_DARK: darkTheme = true; break;

        case CMD_LINE_FPS:
            showFps = true;

        case CMD_LINE_NONE:
        case CMD_LINE_RUN:
            darkTheme = isDarkMode();
            break;
    }

    showWindow();
    initShaders();
    setupUI(darkTheme);
    enterRenderLoop();

    return 0;
}

