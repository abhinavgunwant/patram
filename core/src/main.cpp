#include "include/argparser.hpp"
// #include "include/window-utils.hpp"
// #include "include/setup-ui.hpp"
#include "include/startup-checks.hpp"

#include "ui.hpp"

using namespace std;

int main(int argc, char ** argv) {
    processCmdLineArgs(argc, argv);

    runStartupChecks();

    showWindow();
    setupUI(darkTheme);
    enterRenderLoop();

    return 0;
}

