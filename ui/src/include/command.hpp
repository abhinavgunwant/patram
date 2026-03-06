#ifndef PATRAM_CORE_UI
#define PATRAM_CORE_UI

#include <string>
#include <iostream>

#define MAX_COMMANDS 16
// #define DEBUG_COMMANDS

using namespace std;

enum class CommandType : uint8_t {
    None,
    Type,
    NewLine,
    Delete,
    Backspace,
    Undo,
    Redo,
    Replace,
};

struct ContentCommand {
    string str;
    string prevString; // old string. Useful for undo.

    CommandType type;

    uint32_t selectRowStart;
    uint32_t selectRowEnd;
    uint32_t selectColStart;
    uint32_t selectColEnd;
    uint32_t cursorPos;
    uint32_t cbIndex;
};

/**
 * Used when user types something after selecting text on text input.
 * Contains selection details witht the string that will be replaced.
 */
struct ReplaceData {
    uint32_t selectRowStart;
    uint32_t selectRowEnd;
    uint32_t selectColStart;
    uint32_t selectColEnd;
    /** Contains the old string. Useful for undo. */
    string prevString;

    /** Contains the new string. */
    string newString;
};

struct Command {
    CommandType type;

    union {
        string str;
        ReplaceData replace;
    };

    /**
     * Cursor position BEFORE the change occured.
     */
    uint32_t cursorPos;

    Command* prev;
    Command* next;

    Command():
        type(CommandType::None), str(""), cursorPos(0), prev(nullptr), next(nullptr) {}

    Command(CommandType type, string strData, uint32_t cursorPos):
        type(type), cursorPos(cursorPos), str(strData), prev(nullptr), next(nullptr) {}

    Command(
        uint32_t rowStart, uint32_t rowEnd,
        uint32_t colStart, uint32_t colEnd,
        string newString, string prevString, uint32_t cursorPos
    ):
        type(CommandType::Replace), cursorPos(cursorPos), prev(nullptr), next(nullptr),
        replace({ rowStart, rowEnd, colStart, colEnd, prevString, newString }) {}

    ~Command() {
        if (type == CommandType::Replace) {
            replace.newString.~string();
            replace.prevString.~string();
        } else {
            str.~string();
        }
    }
};

void deleteCommand(Command* c);
string commandStr(Command* c);

/**
 * Link List of commands
 */
struct CommandList {
    uint8_t length;
    Command* currentCommand;

    CommandList(): length(1), currentCommand(new Command()) {}

    ~CommandList() {
        cout << "\nDeleting comand list!";
        Command * cmd = currentCommand;
        while (cmd->next != nullptr) {
            cmd = cmd->next;
        }

        while (cmd != nullptr) {
            deleteCommand(cmd);
            cmd = cmd->prev;
        }
    }
};

/**
 * Adds a command right after the current command. Deletes any commands after
 * the current command before appending.
 * So, not quite "append" in the usual way, but whatever.
 */
void appendCommand(CommandList &list, Command* newCommand);
void moveToPrevCommand(CommandList &list);
void moveToNextCommand(CommandList &list);
void printCommands(CommandList& list);

#endif // PATRAM_CORE_UI

