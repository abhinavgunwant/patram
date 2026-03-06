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
        #ifdef DEBUG_COMMANDS
            cout << "\nMoved to previous command!";
    } else {
            if (list.currentCommand != nullptr) {
                cout << "\nCurrent command is null!";
            } else {
                cout << "\nPrevious command is null!";
            }
        #endif
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
        case CommandType::None:
            return "CommandType::None";
        case CommandType::Type:
            return "CommandType::Type \"" + c->str + "\", cursorPos: " + to_string(c->cursorPos);
        case CommandType::NewLine:
            return "CommandType::NewLine \"" + c->str + "\", cursorPos: " + to_string(c->cursorPos);
        case CommandType::Undo:
            return "CommandType::Undo, cursorPos: " + to_string(c->cursorPos);
        case CommandType::Redo:
            return "CommandType::Redo, cursorPos: " + to_string(c->cursorPos);
        case CommandType::Delete:
            return "CommandType::Delete \"" + c->str + "\", cursorPos: " + to_string(c->cursorPos);
        case CommandType::Backspace:
            return "CommandType::Backspace \"" + c->str + "\", cursorPos: " + to_string(c->cursorPos);
        case CommandType::Replace:
            return "CommandType::Replace \"" + c->replace.prevString + "\" -> \"" + c->replace.newString + "\"";
    }

    return "ERR: NO_COMMAND";
}

void printCommands(CommandList& list) {
    #ifdef DEBUG_COMMANDS
        cout << "\nCommands (" << (int)list.length << "):";
    #endif

    if (list.currentCommand == nullptr) {
        #ifdef DEBUG_COMMANDS
            cout << " empty!";
        #endif
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
        #ifdef DEBUG_COMMANDS
            if (head == list.currentCommand) {
                cout << "\n -> ";
            } else {
                cout << "\n    ";
            }

            cout << (int)i << " " << commandStr(head);
        #endif

        head = head->next;
        ++i;
    }
}

