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
        case CommandType::DeleteSelected:
            return "CommandType::DeleteSelected \"" + c->str + "\", cursorPos: " + to_string(c->cursorPos);
        case CommandType::Backspace:
            return "CommandType::Backspace \"" + c->str + "\", cursorPos: " + to_string(c->cursorPos);
        case CommandType::Replace:
            return "CommandType::Replace \"" + c->replace.prevString + "\" -> \"" + c->replace.newString + "\"";
    }

    return "ERR: NO_COMMAND";
}

string commandTypeStr(CommandType type) {
    switch (type) {
        case CommandType::None:             return "CommandType::None";
        case CommandType::Type:             return "CommandType::Type";
        case CommandType::NewLine:          return "CommandType::NewLine";
        case CommandType::Undo:             return "CommandType::Undo";
        case CommandType::Redo:             return "CommandType::Redo";
        case CommandType::Delete:           return "CommandType::Delete";
        case CommandType::DeleteSelected:   return "CommandType::DeleteSelected";
        case CommandType::Backspace:        return "CommandType::Backspace";
        case CommandType::Replace:          return "CommandType::Replace";
        case CommandType::Paste:            return "CommandType::Paste";
    }

    return "";
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

void printContentCommand(ContentCommand* cmd, ContentCommandExt* cmdExt) {
    cout << "type: " << commandTypeStr(cmd->type);

    if (cmd->ext && cmdExt != nullptr) {
        cout << ", startCBIndex: " << cmd->startCBIndex << ", startCursorPos: "
            << cmd->startCursorPos << ", endCBIndex: " << cmdExt->endCBIndex
            << ", endCursorPos: " << cmdExt->endCursorPos;

        if (!cmdExt->deletedBlocks.empty()) {
            cout << "\n     |-> Deleted Content Blocks:";
        }
    } else {
        cout << ", CBIndex: " << cmd->startCBIndex << ", cursorPos: "
            << cmd->startCursorPos << ", str: \"" << cmd->str << "\"";
    }
}

string to_string(CommandType cmdType) {
    switch (cmdType) {
        case CommandType::None: return "None";
        case CommandType::Type: return "Type";
        case CommandType::NewLine: return "NewLine";
        case CommandType::Delete: return "Delete";
        case CommandType::DeleteSelected: return "DeleteSelected";
        case CommandType::DeleteMergeCB: return "DeleteMergeCB";
        case CommandType::Backspace: return "Backspace";
        case CommandType::Undo: return "Undo";
        case CommandType::Redo: return "Redo";
        case CommandType::Replace: return "Replace";
    }

    return "";
}

