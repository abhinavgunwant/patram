#include "include/argparser.hpp"
#include "include/window-utils.hpp"
#include "include/setup-ui.hpp"
#include "include/startup-checks.hpp"

#include "shader.hpp"

using namespace std;

int main(int argc, char ** argv) {
    processCmdLineArgs(argc, argv);

    runStartupChecks();

    showWindow();
    initShaders();
    setupUI(darkTheme);
    enterRenderLoop();

    return 0;
}

