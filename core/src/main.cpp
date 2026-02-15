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
    processCmdLineArgs(argc, argv);

    if (!cacheDirectoryExists()) {
        createCacheDirectory();
    }

    showWindow();
    initShaders();
    setupUI(darkTheme);
    enterRenderLoop();

    return 0;
}

