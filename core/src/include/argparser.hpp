#ifndef PATRAM_CORE_ARGPARSER
#define PATRAM_CORE_ARGPARSER

#include <string>

using namespace std;

enum CmdLineArgs {
    CMD_LINE_RUN,
    CMD_LINE_RUN_DARK, // dark theme
    CMD_LINE_VERSION,
    CMD_LINE_ERROR,
    CMD_LINE_HELP,
    CMD_LINE_NONE,
    CMD_LINE_FPS,
};

void processCmdLineArgs(int argc, char ** argv);
CmdLineArgs parseArgs(int argc, char ** argv);
void displayVersion();
void displayHelp();
void displayError();

#endif // PATRAM_CORE_ARGPARSER

