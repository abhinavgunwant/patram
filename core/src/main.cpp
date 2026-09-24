#include "include/argparser.hpp"
#include "include/startup-checks.hpp"

#include "ui.hpp"
#include "logger.hpp"

int main(int argc, char ** argv) {
    initLogging();

    processCmdLineArgs(argc, argv);

    runStartupChecks();

    showWindow();
    setupUI(darkTheme);
    enterRenderLoop();

    destroyLogging();

    return 0;
}

