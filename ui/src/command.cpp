#include <iostream>

#include "include/command.hpp"

using namespace std;

void appendCommand(CommandList &list, Command* newCommand) {
    if (list.currentCommand->next != nullptr) {
        Command * c = list.currentCommand;

        // Move to the end
        while (c->next != nullptr) {
            c = c->next;
        }

        // Delete all the commands till current command
        while (c != list.currentCommand) {
            Command * prev = c->prev;
            deleteCommand(c);
            list.length--;
            c = prev;
        }
    }

    list.currentCommand->next = newCommand;
    newCommand->prev = list.currentCommand;
    list.currentCommand = newCommand;
    list.length++;

    #ifdef DEBUG_COMMANDS
        cout << "\nAppended command " << commandStr(newCommand);
    #endif
}

void moveToPrevCommand(CommandList &list) {
    if (list.currentCommand != nullptr && list.currentCommand->prev != nullptr) {
        list.currentCommand = list.currentCommand->prev;
        cout << "\nMoved to previous command!";
    } else {
        if (list.currentCommand != nullptr) {
            cout << "\nCurrent command is null!";
        } else {
            cout << "\nPrevious command is null!";
        }
    }
}

void moveToNextCommand(CommandList &list) {
    if (list.currentCommand != nullptr && list.currentCommand->next != nullptr) {
        list.currentCommand = list.currentCommand->next;
    }
}

void deleteCommand(Command* c) {
    if (c->prev != nullptr) {
        c->prev->next = nullptr;
    }

    delete c;
}

string commandStr(Command* c) {
    switch (c->type) {
        case COMMAND_NONE:
            return "COMMAND_NONE";
        case COMMAND_TYPE:
            return "COMMAND_TYPE \"" + c->str + "\", cursorPos: " + to_string(c->cursorPos);
        case COMMAND_UNDO:
            return "COMMAND_UNDO, cursorPos: " + to_string(c->cursorPos);
        case COMMAND_REDO:
            return "COMMAND_REDO, cursorPos: " + to_string(c->cursorPos);
        case COMMAND_DELETE:
            return "COMMAND_DELETE \"" + c->str + "\", cursorPos: " + to_string(c->cursorPos);
        case COMMAND_BACKSPACE:
            return "COMMAND_BACKSPACE \"" + c->str + "\", cursorPos: " + to_string(c->cursorPos);
        case COMMAND_REPLACE:
            return "COMMAND_REPLACE \"" + c->replace.prevString + "\" -> \"" + c->replace.newString + "\"";
    }

    return "ERR: NO_COMMAND";
}

void printCommands(CommandList& list) {
    cout << "\nCommands (" << (int)list.length << "):";

    if (list.currentCommand == nullptr) {
        cout << " empty!";
        return;
    }

    Command* head = nullptr;

    if (list.currentCommand->prev != nullptr) {
        head = list.currentCommand->prev;

        while (head->prev != nullptr) {
            head = head->prev;
        }
    } else {
        head = list.currentCommand;
    }

    uint8_t i = 0;

    while (head != nullptr) {
        if (head == list.currentCommand) {
            cout << "\n -> ";
        } else {
            cout << "\n    ";
        }

        cout << (int)i << " " << commandStr(head);

        head = head->next;
        ++i;
    }
}

