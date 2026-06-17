#include "include/argparser.hpp"
#include "include/startup-checks.hpp"

#include "ui.hpp"
#include "logger.hpp"

using namespace std;

int main(int argc, char ** argv) {
    initLogging();

    info("Startup");

    processCmdLineArgs(argc, argv);

    runStartupChecks();

    showWindow();
    setupUI(darkTheme);
    enterRenderLoop();

    destroyLogging();

    return 0;
}

