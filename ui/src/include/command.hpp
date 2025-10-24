#ifndef PATRAM_CORE_UI
#define PATRAM_CORE_UI

#include <string>

#define MAX_COMMANDS 16

using namespace std;

enum CommandType {
    COMMAND_NONE,
    COMMAND_TYPE,
    COMMAND_DELETE,
    COMMAND_BACKSPACE,
    COMMAND_UNDO,
    COMMAND_REDO,
};

struct Command {
    CommandType type;
    string data;

    /**
     * Cursor position BEFORE the change occured.
     */
    uint32_t cursorPos;

    Command* prev;
    Command* next;
};

/**
 * Link List of commands
 */
struct CommandList {
    uint8_t length;
    Command* currentCommand;
};

CommandList initCommandList();
Command* initCommand(CommandType type, string data, uint32_t cursorPos);

/**
 * Adds a command right after the current command. Deletes any commands after
 * the current command before appending.
 * So, not quite "append" in the usual way, but whatever.
 */
void appendCommand(CommandList &list, Command* newCommand);
void moveToPrevCommand(CommandList &list);
void moveToNextCommand(CommandList &list);
void deleteCommand(Command* c);
string commandStr(Command* c);
void printCommands(CommandList& list);

#endif // PATRAM_CORE_UI

