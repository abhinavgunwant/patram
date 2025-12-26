#include "include/argparser.hpp"

#include <iostream>
#include <string>

using namespace std;

CmdLineArgs parseArgs(int argc, char ** argv) {
    if (argc == 2) {
        if (strcmp(argv[1], "-v") == 0 || strcmp(argv[1], "--version") == 0) {
            return CMD_LINE_VERSION;
        }

        if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
            return CMD_LINE_HELP;
        }

        if (strcmp(argv[1], "-d") == 0 || strcmp(argv[1], "--dark-theme") == 0) {
            return CMD_LINE_RUN_DARK;
        }

        if (strcmp(argv[1], "-l") == 0 || strcmp(argv[1], "--light-theme") == 0) {
            return CMD_LINE_RUN;
        }

        if (strcmp(argv[1], "--fps") == 0) {
            return CMD_LINE_FPS;
        }

        return CMD_LINE_ERROR;
    }

    return CMD_LINE_NONE;
}

void displayCommandHelp() {
    cout << "\n  patram.exe"
        << "\n  patram.exe [options]"
        << "\n\nOptions:"
        << "\n  -v, --version: Shows the current version"
        << "\n  -h, --help:    Shows this menu";
}

void displayVersion() {
    cout << "\npatram\nVersion: 0.1.0\n";
}

void displayError() {
    displayVersion();

    cout << "\nError: invalid command-line arguments.\n\nAvailable command usage:";
    displayCommandHelp();
}

void displayHelp() {
    displayVersion();
    cout << "\nUsage:";
    displayCommandHelp();
}

