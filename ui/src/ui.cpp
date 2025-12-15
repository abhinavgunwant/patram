#include <algorithm>
#include <iostream>
#include <stack>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "include/ui.hpp"
#include "include/command.hpp"
#include "include/shader.hpp"
#include "include/text.hpp"
#include "../lib/stb_image.h"

#define TIME() static_cast<float>(glfwGetTime())

using namespace std;

float iconButtonVertices[16] = {
    -1.0, -1.0, -1.0, -1.0,
    -1.0, -1.0, -1.0, -1.0,
    -1.0, -1.0, -1.0, -1.0,
    -1.0, -1.0, -1.0, -1.0,
};

bool darkTheme = false;

uint32_t iconButtonVAO;
uint32_t iconButtonVBO;

void defaultCallback(UIElement* _elem) {
    cout << "\nDefault callback executed!";
}

void defaultKeyCallback(UIElement* elem, int key, int action, int mod) {
    cout << "\nDefault callback executed!";
}

void defaultOnMouseUpDownCallback(UIElement*, float, float, glm::vec2) {
    cout << "\nDefault mouse up/down callback executed!";
}

void textInputKeyCallback(UIElement* elem, int key, int action, int mod) {
    bool alt = mod & GLFW_MOD_ALT;
    bool shift = mod & GLFW_MOD_SHIFT;
    bool ctrl = mod & GLFW_MOD_CONTROL;
    bool capsLck = mod & GLFW_MOD_CAPS_LOCK;
    bool numLck = mod & GLFW_MOD_NUM_LOCK;

    // cout << "\n Key: ";

    // if (alt) {
    //     cout << "Alt + ";
    // }

    // if (shift) {
    //     cout << "Shift + ";
    // }

    // if (ctrl) {
    //     cout << "Control + ";
    // }

    // if (capsLck) {
    //     cout << "Caps Lock + ";
    // }

    // if (numLck) {
    //     cout << "Num Lock + ";
    // }

    // cout << key;

    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        #define TYPE(c) processCommand(elem, new Command(COMMAND_TYPE, c, elem->extraData->textInputData.cursorPosition)); elem->extraData->textInputData.lastActiveTime = static_cast<float>(glfwGetTime())
        #define INCR_CURSOR() elem->extraData->textInputData.cursorPosition++
        #define DECR_CURSOR() elem->extraData->textInputData.cursorPosition--
        #define CUR_POS elem->extraData->textInputData.cursorPosition
        #define UPDATE_CURSOR_TIME() elem->extraData->textInputData.lastActiveTime = static_cast<float>(glfwGetTime())

        switch (key) {
            case GLFW_KEY_ESCAPE: {
                TextInputData* tid = &elem->extraData->textInputData;

                if (tid->textSelected) {
                    textInputUnselect(tid);
                } else {
                    elem->active = false;
                }
                break;
            }

            case GLFW_KEY_LEFT:
                if (elem->extraData != nullptr && CUR_POS != 0) {
                    TextInputData* tid = &elem->extraData->textInputData;

                    if (shift) {
                        if (tid->textSelected) {
                            if (tid->cursorPosition - 1 == tid->initialCursorPosition) {
                                textInputUnselect(tid);
                            } else if (tid->cursorPosition > tid->initialCursorPosition) {
                                tid->textSelectEndCol -= 1;
                            } else {
                                tid->textSelectStartCol -= 1;
                            }
                        } else {
                            tid->textSelected = true;
                            tid->initialCursorPosition = tid->cursorPosition;
                            tid->textSelectStartCol = tid->cursorPosition - 1;
                            tid->textSelectEndCol = tid->cursorPosition;
                        }

                        tid->textSelectUpdated = true;
                    } else {
                        textInputUnselect(tid);
                    }

                    DECR_CURSOR();
                    UPDATE_CURSOR_TIME();
                }
                break;

            case GLFW_KEY_RIGHT:
                if (elem->extraData != nullptr && CUR_POS != elem->text.length()) {
                    TextInputData* tid = &elem->extraData->textInputData;

                    if (shift) {
                        if (tid->textSelected) {
                            if (tid->cursorPosition + 1 == tid->initialCursorPosition) {
                                textInputUnselect(tid);
                            } else if (tid->cursorPosition > tid->initialCursorPosition) {
                                tid->textSelectEndCol += 1;
                            } else {
                                tid->textSelectStartCol += 1;
                            }
                        } else {
                            tid->textSelected = true;
                            tid->initialCursorPosition = tid->cursorPosition;
                            tid->textSelectEndCol = tid->cursorPosition + 1;
                            tid->textSelectStartCol = tid->cursorPosition;
                        }

                        tid->textSelectUpdated = true;
                    } else {
                        textInputUnselect(tid);
                    }

                    INCR_CURSOR();
                    UPDATE_CURSOR_TIME();
                }
                break;

            case GLFW_KEY_UP:
                if (elem->type == UI_TEXT_AREA) {
                }
                break;

            case GLFW_KEY_DOWN:
                break;

            case GLFW_KEY_ENTER:
                cout << "\nEnter pressed";
                if (shift && elem->type == UI_TEXT_AREA) {
                    TYPE("\n");
                    INCR_CURSOR();
                }
                break;

            case GLFW_KEY_END:
                CUR_POS = static_cast<uint32_t>(elem->text.length());
                UPDATE_CURSOR_TIME();
                break;

            case GLFW_KEY_HOME:
                CUR_POS = 0;
                UPDATE_CURSOR_TIME();
                break;

            case GLFW_KEY_BACKSPACE: {
                TextInputData * tid = &elem->extraData->textInputData;

                if (
                    CUR_POS <= elem->text.length() && CUR_POS > 0
                    || tid->textSelected
                ) {
                    string d;

                    if (tid->textSelected){
                        d = "";
                    } else {
                        d = elem->text[CUR_POS-1];
                    }

                    bool shouldDecrementCursor = !tid->textSelected;

                    processCommand(elem, new Command(COMMAND_BACKSPACE, d, CUR_POS));

                    if (shouldDecrementCursor && CUR_POS != 0) {
                        DECR_CURSOR();
                    }

                    UPDATE_CURSOR_TIME();
                }

                break;
            }

            case GLFW_KEY_DELETE: {
                TextInputData * tid = &elem->extraData->textInputData;

                if (
                    CUR_POS < elem->text.length() && CUR_POS >= 0
                    || tid->textSelected
                ) {
                    string d;

                    if (tid->textSelected) {
                        d = "";
                    } else {
                        d = elem->text[CUR_POS];
                    }

                    processCommand(elem, new Command(COMMAND_DELETE, d, CUR_POS));
                    UPDATE_CURSOR_TIME();
                }

                break;
            }

            case GLFW_KEY_A:
                if (ctrl) {
                    TextInputData* tid = &elem->extraData->textInputData;
                    const uint32_t len = static_cast<uint32_t>(elem->text.length());
                    tid->textSelected = true;
                    tid->initialCursorPosition = tid->cursorPosition;
                    tid->textSelectStartCol = 0;
                    tid->textSelectEndCol = len;
                    tid->textSelectUpdated = true;
                    tid->cursorPosition = len;
                } else if (capsLck || shift) { TYPE("A"); } else { TYPE("a"); }
                INCR_CURSOR();
                break;

            case GLFW_KEY_B:
                if (capsLck || shift) { TYPE("B"); } else { TYPE("b"); }
                INCR_CURSOR();
                break;

            case GLFW_KEY_C:
                if (capsLck || shift) { TYPE("C"); } else { TYPE("c"); }
                INCR_CURSOR();
                break;

            case GLFW_KEY_D:
                if (capsLck || shift) { TYPE("D"); } else { TYPE("d"); }
                INCR_CURSOR();
                break;

            case GLFW_KEY_E:
                if (capsLck || shift) { TYPE("E"); } else { TYPE("e"); }
                INCR_CURSOR();
                break;

            case GLFW_KEY_F:
                if (capsLck || shift) { TYPE("F"); } else { TYPE("f"); }
                INCR_CURSOR();
                break;

            case GLFW_KEY_G:
                if (capsLck || shift) { TYPE("G"); } else { TYPE("g"); }
                INCR_CURSOR();
                break;

            case GLFW_KEY_H:
                if (capsLck || shift) { TYPE("H"); } else { TYPE("h"); }
                INCR_CURSOR();
                break;

            case GLFW_KEY_I:
                if (capsLck || shift) { TYPE("I"); } else { TYPE("i"); }
                INCR_CURSOR();
                break;

            case GLFW_KEY_J:
                if (capsLck || shift) { TYPE("J"); } else { TYPE("j"); }
                INCR_CURSOR();
                break;

            case GLFW_KEY_K:
                if (capsLck || shift) { TYPE("K"); } else { TYPE("k"); }
                INCR_CURSOR();
                break;

            case GLFW_KEY_L:
                if (capsLck || shift) { TYPE("L"); } else { TYPE("l"); }
                INCR_CURSOR();
                break;

            case GLFW_KEY_M:
                if (capsLck || shift) { TYPE("M"); } else { TYPE("m"); }
                INCR_CURSOR();
                break;

            case GLFW_KEY_N:
                if (capsLck || shift) { TYPE("N"); } else { TYPE("n"); }
                INCR_CURSOR();
                break;

            case GLFW_KEY_O:
                if (capsLck || shift) { TYPE("O"); } else { TYPE("o"); }
                INCR_CURSOR();
                break;

            case GLFW_KEY_P:
                if (capsLck || shift) { TYPE("P"); } else { TYPE("p"); }
                INCR_CURSOR();
                break;

            case GLFW_KEY_Q:
                if (capsLck || shift) { TYPE("Q"); } else { TYPE("q"); }
                INCR_CURSOR();
                break;

            case GLFW_KEY_R:
                if (capsLck || shift) { TYPE("R"); } else { TYPE("r"); }
                INCR_CURSOR();
                break;

            case GLFW_KEY_S:
                if (capsLck || shift) { TYPE("S"); } else { TYPE("s"); }
                INCR_CURSOR();
                break;

            case GLFW_KEY_T:
                if (capsLck || shift) { TYPE("T"); } else { TYPE("t"); }
                INCR_CURSOR();
                break;

            case GLFW_KEY_U:
                if (capsLck || shift) { TYPE("U"); } else { TYPE("u"); }
                INCR_CURSOR();
                break;

            case GLFW_KEY_V:
                if (capsLck || shift) { TYPE("V"); } else { TYPE("v"); }
                INCR_CURSOR();
                break;

            case GLFW_KEY_W:
                if (capsLck || shift) { TYPE("W"); } else { TYPE("w"); }
                INCR_CURSOR();
                break;

            case GLFW_KEY_X:
                if (capsLck || shift) { TYPE("X"); } else { TYPE("x"); }
                INCR_CURSOR();
                break;

            case GLFW_KEY_Y:
                if (ctrl) {
                    processCommand(elem, new Command(COMMAND_REDO, "", CUR_POS));
                } else if (capsLck || shift) {
                    TYPE("Y");
                    INCR_CURSOR();
                } else {
                    TYPE("y");
                    INCR_CURSOR();
                }
                break;

            case GLFW_KEY_Z:
                if (ctrl) {
                    processCommand(elem, new Command(COMMAND_UNDO, "", CUR_POS));
                } else if (capsLck || shift) {
                    TYPE("Z");
                    INCR_CURSOR();
                } else {
                    TYPE("z");
                    INCR_CURSOR();
                }
                break;

            case GLFW_KEY_0:
                if (shift) { TYPE(")"); } else { TYPE("0"); }
                INCR_CURSOR();
                break;

            case GLFW_KEY_1:
                if (shift) { TYPE("!"); } else { TYPE("1"); }
                INCR_CURSOR();
                break;

            case GLFW_KEY_2:
                if (shift) { TYPE("@"); } else { TYPE("2"); }
                INCR_CURSOR();
                break;

            case GLFW_KEY_3:
                if (shift) { TYPE("#"); } else { TYPE("3"); }
                INCR_CURSOR();
                break;

            case GLFW_KEY_4:
                if (shift) { TYPE("$"); } else { TYPE("4"); }
                INCR_CURSOR();
                break;

            case GLFW_KEY_5:
                if (shift) { TYPE("%"); } else { TYPE("5"); }
                INCR_CURSOR();
                break;

            case GLFW_KEY_6:
                if (shift) { TYPE("^"); } else { TYPE("6"); }
                INCR_CURSOR();
                break;

            case GLFW_KEY_7:
                if (shift) { TYPE("&"); } else { TYPE("7"); }
                INCR_CURSOR();
                break;

            case GLFW_KEY_8:
                if (shift) { TYPE("*"); } else { TYPE("8"); }
                INCR_CURSOR();
                break;

            case GLFW_KEY_9:
                if (shift) { TYPE("("); } else { TYPE("9"); }
                INCR_CURSOR();
                break;

            case GLFW_KEY_MINUS:
                if (shift) { TYPE("_"); } else { TYPE("-"); }
                INCR_CURSOR();
                break;

            case GLFW_KEY_EQUAL:
                if (shift) { TYPE("+"); } else { TYPE("="); }
                INCR_CURSOR();
                break;

            case GLFW_KEY_PERIOD:
                if (shift) { TYPE(">"); } else { TYPE("."); }
                INCR_CURSOR();
                break;

            case GLFW_KEY_COMMA:
                if (shift) { TYPE("<"); } else { TYPE(","); }
                INCR_CURSOR();
                break;

            case GLFW_KEY_SLASH:
                if (shift) { TYPE("?"); } else { TYPE("/"); }
                INCR_CURSOR();
                break;

            case GLFW_KEY_BACKSLASH:
                if (shift) { TYPE("|"); } else { TYPE("\\"); }
                INCR_CURSOR();
                break;

            case GLFW_KEY_LEFT_BRACKET:
                if (shift) { TYPE("{"); } else { TYPE("["); }
                INCR_CURSOR();
                break;

            case GLFW_KEY_RIGHT_BRACKET:
                if (shift) { TYPE("}"); } else { TYPE("]"); }
                INCR_CURSOR();
                break;

            case GLFW_KEY_SEMICOLON:
                if (shift) { TYPE(":"); } else { TYPE(";"); }
                INCR_CURSOR();
                break;

            case GLFW_KEY_APOSTROPHE:
                if (shift) { TYPE("\""); } else { TYPE("'"); }
                INCR_CURSOR();
                break;

            case GLFW_KEY_GRAVE_ACCENT:
                if (shift) { TYPE("~"); } else { TYPE("`"); }
                INCR_CURSOR();
                break;

            case GLFW_KEY_KP_0:
                if (numLck) { TYPE("0"); INCR_CURSOR(); }
                break;

            case GLFW_KEY_KP_1:
                if (numLck) { TYPE("1"); INCR_CURSOR(); }
                break;

            case GLFW_KEY_KP_2:
                if (numLck) { TYPE("2"); INCR_CURSOR(); }
                break;

            case GLFW_KEY_KP_3:
                if (numLck) { TYPE("3"); INCR_CURSOR(); }
                break;

            case GLFW_KEY_KP_4:
                if (numLck) { TYPE("4"); INCR_CURSOR(); }
                break;

            case GLFW_KEY_KP_5:
                if (numLck) { TYPE("5"); INCR_CURSOR(); }
                break;

            case GLFW_KEY_KP_6:
                if (numLck) { TYPE("6"); INCR_CURSOR(); }
                break;

            case GLFW_KEY_KP_7:
                if (numLck) { TYPE("7"); INCR_CURSOR(); }
                break;

            case GLFW_KEY_KP_8:
                if (numLck) { TYPE("8"); INCR_CURSOR(); }
                break;

            case GLFW_KEY_KP_9:
                if (numLck) { TYPE("9"); INCR_CURSOR(); }
                break;

            case GLFW_KEY_KP_DECIMAL:
                if (numLck) { TYPE("."); INCR_CURSOR(); }
                break;

            case GLFW_KEY_KP_DIVIDE:
                TYPE("/"); INCR_CURSOR();
                break;

            case GLFW_KEY_KP_MULTIPLY:
                TYPE("*"); INCR_CURSOR();
                break;

            case GLFW_KEY_KP_SUBTRACT:
                TYPE("-"); INCR_CURSOR();
                break;

            case GLFW_KEY_KP_ADD:
                TYPE("+"); INCR_CURSOR();
                break;

            case GLFW_KEY_KP_EQUAL:
                TYPE("="); INCR_CURSOR();
                break;

            case GLFW_KEY_SPACE:
                TYPE(" "); INCR_CURSOR();
                break;

            default: {
                // Nothing!
            }
        }
        #undef TYPE
        #undef INCR_CURSOR
        #undef DECR_CURSOR
        #undef CUR_POS
        #undef UPDATE_CURSOR_TIME
    }
}

Command* replaceSelectedText(UIElement* elem, Command* oldCommand) {
    TextInputData* tid = &elem->extraData->textInputData;

    uint32_t len = tid->textSelectEndCol - tid->textSelectStartCol;

    Command * newCommand = new Command(
        0, 0, tid->textSelectStartCol, tid->textSelectEndCol,
        oldCommand->str,
        elem->text.substr(tid->textSelectStartCol, len),
        oldCommand->cursorPos
    );

    elem->text.erase(tid->textSelectStartCol, len);
    elem->text.insert(tid->textSelectStartCol, oldCommand->str);

    deleteCommand(oldCommand);

    textInputUnselect(tid);

    return newCommand;
}

SelectionData getTextIndexForScreenCoords(UIElement* elem, float mouseX, float mouseY, glm::vec2 res) {
    uint32_t currentIndex = 0;
    uint32_t row = 0;
    uint32_t col = 0;

    const float xStart = elem->left + BUTTON_PADDING;

    float x = xStart;
    float y = elem->top + BUTTON_PADDING;

    // cout << "\nMouse x: " << mouseX << " y: " << mouseY;
    const float Y_ADVANCE = static_cast<float>(elem->fontSize + LINE_GAP);
    bool shouldYAdvance = false;

    for (const char c: elem->text) {
        shouldYAdvance = c == '\n';
        GlyphDimensions gd = getGlyphDimensions(c, x, y, res, elem->fontSize);

        // cout << "\ncharacter: " << c << ", leftX: " << gd.leftX
        //     << ", rightX: " << gd.rightX << ", bottomY: " << gd.bottomY
        //     << ", topY: " << gd.topY << "\nx: " << x;

        uint32_t net_advance = (static_cast<uint32_t>(gd.advance * gd.scale) >> 6);

        if (
            x + static_cast<float>(net_advance)/2 > mouseX
            && (
                // !shouldYAdvance
                // || shouldYAdvance &&
                y + Y_ADVANCE > mouseY
            )
        ) {
            #ifdef DEBUG_TEXT_SELECTION
                cout << "\nx: " << x << ", y: " << y << ", shouldYAdvance: " << shouldYAdvance << " ";
            #endif
            break;
        }

        if (shouldYAdvance) {
            x = xStart;
            y += Y_ADVANCE;
            col = 0;
            ++row;
        } else {
            ++col;
            x += net_advance;
            ++currentIndex;
        }
    }

    return {
        .cursorPosition = currentIndex,
        .row = row,
        .col = col,
    };
}

void textInputMouseDownCallback(
    UIElement* elem,
    float mouseX,
    float mouseY,
    glm::vec2 res
) {
    if (elem->extraData == nullptr) { return; }

    TextInputData * tid = &elem->extraData->textInputData;

    SelectionData sd = getTextIndexForScreenCoords(
        elem,
        mouseX,
        mouseY,
        res
    );

    tid->cursorPosition = sd.cursorPosition;

    if (tid->textSelected) {
        tid->textSelectStartCol = sd.col;
        tid->textSelectStartRow = sd.row;
        textInputUnselect(tid, "Text Input Mouse Down callback");
    }

    tid->mouseDown = true;
    tid->mouseDownX = mouseX;
    tid->mouseDownY = mouseY;
    tid->lastActiveTime = TIME();
}

void textInputMouseDragCallback(UIElement* elem, float mouseX, float mouseY, glm::vec2 res) {
    if (elem->extraData == nullptr) { return; }

    #ifdef DEBUG_TEXT_SELECTION
        cout << "\nDrag Callback: ";
    #endif

    TextInputData* tid = &elem->extraData->textInputData;

    if (!tid->mouseDown) {
        #ifdef DEBUG_TEXT_SELECTION
            cout << "Nothing!";
        #endif
        return;
    }

    SelectionData sd = getTextIndexForScreenCoords(elem, mouseX, mouseY, res);

    // cout << "\nsd.cursorPosition: " << sd.cursorPosition << ", sd.col: " << sd.col << ", sd.row: " << sd.row;

    if (!tid->textSelected) {
        tid->textSelected = true;
        tid->initialCursorPosition = tid->cursorPosition;
        tid->textSelectStartCol = sd.col;
        tid->textSelectEndCol = sd.col;
        tid->textSelectStartRow = sd.row;
        tid->textSelectEndRow = sd.row;
        tid->initialRow = sd.row;
        tid->initialCol = sd.col;

        tid->cursorPosition = sd.cursorPosition;
        tid->textSelectUpdated = true;

        #ifdef DEBUG_TEXT_SELECTION
            printTextInputData(tid, "Text Selection 1");
        #endif
    } else if (tid->textSelected) {
        if (sd.row == tid->initialRow) {
            tid->textSelectStartRow = sd.row;
            tid->textSelectEndRow = sd.row;
        } else if (sd.row < tid->initialRow) {
            tid->textSelectStartRow = sd.row;
        } else {
            tid->textSelectEndRow = sd.row;
        }

        if (
            sd.row == tid->textSelectStartRow
            && sd.row == tid->textSelectEndRow
            && sd.row == tid->initialRow
        ) {
            if (sd.col == tid->initialCol) {
                tid->textSelectStartCol = sd.col;
                tid->textSelectEndCol = sd.col;
            } else if (sd.col < tid->initialCol) {
                if (tid->textSelectStartCol > tid->textSelectEndCol) {
                    if (tid->textSelectStartRow > tid->prevRow) {
                        tid->textSelectEndCol = tid->textSelectStartCol;
                        tid->textSelectStartCol = sd.col;
                    } else if (tid->textSelectStartRow < tid->prevRow) {
                        tid->textSelectEndCol = tid->textSelectStartCol;
                        tid->textSelectStartCol = sd.col;
                    }
                } else {
                    tid->textSelectStartCol = sd.col;
                }
            } else {
                if (tid->textSelectStartCol > tid->textSelectEndCol) {
                    if (tid->textSelectStartRow > tid->prevRow) {
                        tid->textSelectStartCol = tid->textSelectEndCol;
                        tid->textSelectEndCol = sd.col;
                    } else if (tid->textSelectStartRow < tid->prevRow) {
                        tid->textSelectEndCol = tid->textSelectStartCol;
                        tid->textSelectStartCol = sd.col;
                    }
                } else {
                    tid->textSelectEndCol = sd.col;
                }
            }
        } else if (sd.row == tid->textSelectStartRow) {
            if (tid->prevRow != tid->textSelectStartRow && tid->textSelectStartCol < sd.col) {
                tid->textSelectEndCol = tid->textSelectStartCol;
            }

            tid->textSelectStartCol = sd.col;
        } else {
            if (tid->prevRow == tid->textSelectStartRow && tid->textSelectEndCol > sd.col) {
                tid->textSelectStartCol = tid->textSelectEndCol;
            }

            tid->textSelectEndCol = sd.col;
        }

        tid->prevRow = sd.row;
        tid->cursorPosition = sd.cursorPosition;
        tid->textSelectUpdated = true;

        #ifdef DEBUG_TEXT_SELECTION
            printTextInputData(tid, "Text Selection 2");
        #endif
    } else {
        #ifdef DEBUG_TEXT_SELECTION
            cout << "\nMarked selected to false";
        #endif
        textInputUnselect(tid, "Text Input Mouse Drag Callback 2");
    }

    tid->lastActiveTime = TIME();
}

void textInputMouseUpCallback(
    UIElement* elem,
    float mouseX,
    float mouseY,
    glm::vec2 res
) {
    if (elem->extraData == nullptr) {
        return;
    }

    TextInputData * tid = &elem->extraData->textInputData;

    tid->mouseDown = false;

    if (
        tid->textSelected
        && tid->textSelectStartCol == tid->textSelectEndCol
        && tid->textSelectStartRow == tid->textSelectEndRow
    ) {
        textInputUnselect(tid, "Text Input Mouse Up Callback");
    }
}

void textInputUnselect(TextInputData * tid, string token) {
    if (tid == nullptr) { return; }

    #ifdef DEBUG_TEXT_SELECTION
        cout << "\nMarked selected to false";
        if (!token.empty()) {
            cout << " (" << token << ")";
        }
    #endif

    tid->textSelected = false;
    // tid->cursorPosition = tid->textSelectStartCol;
    // tid->textSelectStartCol = tid->textSelectStartCol;
    tid->textSelectEndCol = tid->textSelectStartCol;
    tid->initialCursorPosition = tid->textSelectStartCol;
    tid->textSelectEndRow = tid->textSelectStartRow;
}

// TODO: delete
void selectTextWithMouse(
    UIElement* elem,
    float mouseX,
    float mouseY,
    float mouseDownX,
    float mouseDownY,
    bool mouseUp,
    glm::vec2& res
) {
    if (elem->extraData == nullptr) {
        return;
    }

    TextInputData * tid = &elem->extraData->textInputData;

    if (!tid->textSelected && mouseUp) {
        uint32_t currentIndex = 0;
        float x = elem->left + BUTTON_PADDING;
        float y = elem->top + BUTTON_PADDING;

        // cout << "\nMouse x: " << mouseX << " y: " << mouseY;

        for (const char c: elem->text) {
            GlyphDimensions gd = getGlyphDimensions(c, x, y, res, elem->fontSize);

            // cout << "\ncharacter: " << c << ", leftX: " << gd.leftX
            //     << ", rightX: " << gd.rightX << ", bottomY: " << gd.bottomY
            //     << ", topY: " << gd.topY << "\nx: " << x;

            uint32_t net_advance = (static_cast<uint32_t>(gd.advance * gd.scale) >> 6);

            if (static_cast<float>(x) + static_cast<float>(net_advance)/2 > mouseX) {
                break;
            }

            x += net_advance;

            ++currentIndex;
        }

        tid->cursorPosition = currentIndex;
    }
}

void initElement(UIElement& elem, UIElementType type, float top, float left, float width, float height) {
    UIExtraData * extraData = nullptr;
    Rope* rope = nullptr;

    glm::vec4 backgroundColor;
    glm::vec4 hoverColor;
    glm::vec4 color;
    uint8_t borderRadius = 0;

    UIPreferences* pref = nullptr;

    void (*onKey)(UIElement*, int, int, int) = defaultKeyCallback;
    void (*onMouseDown)(UIElement*, float, float, glm::vec2) = defaultOnMouseUpDownCallback;
    void (*onMouseDrag)(UIElement*, float, float, glm::vec2) = defaultOnMouseUpDownCallback;
    void (*onMouseUp)(UIElement*, float, float, glm::vec2) = defaultOnMouseUpDownCallback;

    switch (type) {
        case UI_BUTTON: {
            if (darkTheme) {
                backgroundColor = BUTTON_BG_DARK;
                hoverColor = BUTTON_HOVER_BG_DARK;
                color = BUTTON_COLOR_DARK;
            } else {
                backgroundColor = BUTTON_BG;
                hoverColor = BUTTON_HOVER_BG;
                color = BUTTON_COLOR;
            }

            borderRadius = 4;
            break;
        }

        case UI_ICON_BUTTON:
            if (darkTheme) {
                backgroundColor = ICON_BUTTON_BG_DARK;
                hoverColor = ICON_BUTTON_HOVER_BG_DARK;
                color = ICON_BUTTON_COLOR_DARK;
            } else {
                backgroundColor = ICON_BUTTON_BG;
                hoverColor = ICON_BUTTON_HOVER_BG;
                color = ICON_BUTTON_COLOR;
            }
            break;

        case UI_LABEL:
            pref = initPreferences();
            pref->widthPreference = UI_GROW;

            backgroundColor = TRANSPARENT;
            hoverColor = TRANSPARENT;

            if (darkTheme) {
                color = WHITE;
            } else {
                color = BLACK;
            }
            break;

        case UI_CONTAINER:
            pref = initPreferences();

            backgroundColor = TRANSPARENT;
            hoverColor = TRANSPARENT;

            if (darkTheme) {
                color = WHITE;
            } else {
                color = BLACK;
            }
            break;

        case UI_SCROLL_AREA:
            extraData = new UIExtraData {
                .scrollData = ScrollData {
                    0.0, 0.0, 0.0, 0.0, 0.0, false, false, false, 0, 0, 0, 0
                }
            };

            backgroundColor = TRANSPARENT;
            hoverColor = TRANSPARENT;

            if (darkTheme) {
                color = WHITE;
            } else {
                color = BLACK;
            }
            break;

        case UI_TEXT_AREA:
            rope = new Rope();
        case UI_TEXT_INPUT:
            pref = initPreferences();

            // backgroundColor = TRANSPARENT;
            backgroundColor = glm::vec4(0.9, 0.9, 0.9, 1.0);
            hoverColor = TRANSPARENT;

            if (darkTheme) {
                color = WHITE;
            } else {
                color = BLACK;
            }

            extraData = new UIExtraData {
                .textInputData = TextInputData {
                    .cursorPosition = 0,
                    .cursorVAO = 0,
                    .cursorVBO = 0,
                    .textSelectStartRow = 0,
                    .textSelectStartCol = 0,
                    .textSelectEndRow = 0,
                    .textSelectEndCol = 0,
                    .initialCursorPosition = 0,
                    .initialRow = 0,
                    .initialCol = 0,
                    .prevRow = 0,
                    .textSelected = false,
                    .mouseDown = false,
                    .textSelectUpdated = false,
                    .mouseDownX = 0.0f,
                    .mouseDownY = 0.0f,
                    .lastActiveTime = 0.0f,
                    .cmdList = CommandList(),
                }
            };

            onMouseDown = textInputMouseDownCallback;
            onMouseDrag = textInputMouseDragCallback;
            onMouseUp = textInputMouseUpCallback;
            onKey = textInputKeyCallback;
            break;

        default:
            backgroundColor = TRANSPARENT;
            hoverColor = TRANSPARENT;
            color = BLACK;
    }

    elem = {
        type,
        top,
        left,
        width,
        height,

        false,
        false,
        false,
        true,

        defaultCallback,
        onMouseDown,
        onMouseDrag,
        onMouseUp,
        defaultCallback,
        onKey,

        "",
        nullptr,

        nullptr,

        0,
        0,
        nullptr,

        backgroundColor,
        hoverColor,
        color,

        0,
        0,
        0,

        ICON_NONE,

        16,
        borderRadius,
        UI_POSITIONING_ABSOLUTE,
        pref,
        extraData,
    };
}

UIPreferences* initPreferences() {
    UIPreferences* pref = new UIPreferences;

    pref->maxWidth = -1;
    pref->maxHeight = -1;
    pref->direction = true;
    pref->verticalAlign = UI_ALIGN_START;
    pref->horizontalAlign = UI_ALIGN_START;
    pref->heightPreference = UI_FIXED;
    pref->widthPreference = UI_FIXED;

    return pref;
}

void initializeCapacity(UIElement* parent, int capacity) {
    if (parent->children == nullptr) {
        parent->children = new UIElement*[capacity];
        parent->capacity = capacity;
    }
}

void growCapacity(UIElement* parent) {
    if (parent->capacity > GROWTH_THRESHOLD) {
        parent->capacity += INCR_POST_GROWTH_THRESHOLD;
    } else {
        parent->capacity *= GROWTH_FACTOR;
    }

    UIElement** newlist = new UIElement*[parent->capacity];

    for (uint32_t i=0; i<parent->totalChildren; ++i) {
        newlist[i] = parent->children[i];
    }

    delete [] parent->children;

    parent->children = newlist;
}

void addChild(UIElement* parent, UIElement * child) {
    if (child->parent != nullptr) {
        cerr << "\nError: child already has a parent";
    }

    if (parent->capacity == 0) {
        initializeCapacity(parent, INITIAL_PARENT_CAPACITY);
    } else if (parent->totalChildren + 1 > parent->capacity) {
        growCapacity(parent);
    }

    parent->children[parent->totalChildren] = child;
    child->parent = parent;

    ++parent->totalChildren;
}

void recalculateLayout(UIElement * elem) {
    elem->reDraw = true;

    if (elem->preferences != nullptr) {
        switch (elem->preferences->widthPreference) {
            case UI_FULL:
                if (elem->parent != nullptr) {
                    cout << "\nparent element!";
                    elem->width = elem->parent->width;
                }
            default:
                break;
        }

        switch (elem->preferences->heightPreference) {
            case UI_FULL:
                if (elem->parent != nullptr) {
                    elem->height = elem->parent->height;
                }
            default:
                break;
        }
    }

    for (uint32_t i=0; i<elem->totalChildren; ++i) {
        recalculateLayout(elem->children[i]);
    }
}

void renderElement(UIElement& elem, glm::vec2 &resolution, glm::vec2 &cursor) {
    // cout << "resolution: " << resolution.x << "x" << resolution.y;
    switch (elem.type) {
        case UI_CONTAINER: {
            if (elem.reDraw) {
                // recalculateLayout(&elem);

                cout << "\nRe-drawing Container";
                if (elem.vao != 0) {
                    glDeleteVertexArrays(1, &elem.vao);
                }

                if (elem.vbo != 0) {
                    glDeleteBuffers(1, &elem.vbo);
                }

                float right = elem.left + elem.width;
                float bottom = elem.top + elem.height;

                float vertices[] = {
                    right, bottom,
                    right, elem.top,
                    elem.left, bottom,
                    elem.left, elem.top,
                };

                // cout << "\nvertices: (" << vertices[0] << ", " << vertices[1]
                //     << "),\n(" << vertices[2] << ", " << vertices[3]
                //     << "),\n(" << vertices[4] << ", " << vertices[5]
                //     << "),\n(" << vertices[6] << ", " << vertices[7] << ")";

                glGenVertexArrays(1, &elem.vao);
                glBindVertexArray(elem.vao);
                glGenBuffers(1, &elem.vbo);
                glBindBuffer(GL_ARRAY_BUFFER, elem.vbo);
                glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
                glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
                glEnableVertexAttribArray(0);
            }

            glBindVertexArray(elem.vao);
            glUseProgram(shaders[SHADER_DEFAULT].programId);

            uint32_t resU = glGetUniformLocation(shaders[SHADER_DEFAULT].programId, "resolution");
            uint32_t colorU = glGetUniformLocation(shaders[SHADER_DEFAULT].programId, "solidColor");
            uint32_t timeU = glGetUniformLocation(shaders[SHADER_DEFAULT].programId, "time");

            // cout << "\n" << elem.text << ": background color: "
            //     << elem.backgroundColor.x << ", " << elem.backgroundColor.y << ", "
            //     << elem.backgroundColor.z << ", " << elem.backgroundColor.w;

            glUniform2f(resU, resolution.x, resolution.y);
            glUniform4f(
                colorU,
                elem.backgroundColor.x,
                elem.backgroundColor.y,
                elem.backgroundColor.z,
                elem.backgroundColor.w
            );

            glUniform1f(timeU, TIME());

            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

            if (elem.totalChildren) {
                for (uint32_t i=0; i<elem.totalChildren; ++i) {
                    renderElement(*elem.children[i], resolution, cursor);
                }
            }

            elem.reDraw = false;

            break;
        }

        case UI_LABEL: {
            glm::vec4 dim;

            if (elem.positioning == UI_POSITIONING_RELATIVE) {
                float top = elem.parent->top + elem.top;
                float left = elem.parent->left + elem.left;

                if (elem.parent->type == UI_SCROLL_AREA) {
                    top -= elem.parent->extraData->scrollData.yOffset;
                    left -= elem.parent->extraData->scrollData.xOffset;
                }

                dim = renderText(elem.text, glm::vec2(left, top), resolution, elem.color, elem.fontSize);

                if (
                    elem.reDraw
                    && elem.preferences->widthPreference == UI_GROW
                    && elem.parent->type == UI_SCROLL_AREA
                    && elem.parent->extraData->scrollData.width < dim.w
                ) {
                    elem.parent->extraData->scrollData.width = dim.w;
                    elem.reDraw = false;
                }
                break;
            }

            dim = renderText(elem.text, glm::vec2(elem.left, elem.top), resolution, elem.color, elem.fontSize);

            if (
                elem.reDraw
                && elem.preferences->widthPreference == UI_GROW
                && elem.parent->type == UI_SCROLL_AREA
                && elem.parent->extraData->scrollData.width < dim.w
            ) {
                elem.parent->extraData->scrollData.width = dim.w;
                elem.reDraw = false;
            }
            break;
        }

        case UI_BUTTON: {
            if (elem.reDraw) {
                // recalculateLayout(&elem);

                cout << "\nRe-drawing";
                if (elem.vao != 0) {
                    glDeleteVertexArrays(1, &elem.vao);
                }

                if (elem.vbo != 0) {
                    glDeleteBuffers(1, &elem.vbo);
                }

                float right = elem.left + elem.width;
                float bottom = elem.top + elem.height;

                float vertices[] = {
                    right, bottom,
                    right, elem.top,
                    elem.left, bottom,
                    elem.left, elem.top,
                };

                // cout << "\nvertices: (" << vertices[0] << ", " << vertices[1]
                //     << "),\n(" << vertices[2] << ", " << vertices[3]
                //     << "),\n(" << vertices[4] << ", " << vertices[5]
                //     << "),\n(" << vertices[6] << ", " << vertices[7] << ")";

                glGenVertexArrays(1, &elem.vao);
                glBindVertexArray(elem.vao);
                glGenBuffers(1, &elem.vbo);
                glBindBuffer(GL_ARRAY_BUFFER, elem.vbo);
                glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
                glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
                glEnableVertexAttribArray(0);
            }

            glBindVertexArray(elem.vao);
            glUseProgram(shaders[SHADER_BUTTON].programId);

            uint32_t resU = glGetUniformLocation(shaders[SHADER_BUTTON].programId, "resolution");
            uint32_t backgroundColorU = glGetUniformLocation(shaders[SHADER_BUTTON].programId, "backgroundColor");
            uint32_t hoverColorU = glGetUniformLocation(shaders[SHADER_BUTTON].programId, "hoverColor");
            uint32_t timeU = glGetUniformLocation(shaders[SHADER_BUTTON].programId, "time");
            uint32_t borderRadiusU = glGetUniformLocation(shaders[SHADER_BUTTON].programId, "borderRadius");
            uint32_t topleftU = glGetUniformLocation(shaders[SHADER_BUTTON].programId, "topLeft");
            uint32_t dimensionsU = glGetUniformLocation(shaders[SHADER_BUTTON].programId, "dimensions");
            uint32_t cursorU = glGetUniformLocation(shaders[SHADER_BUTTON].programId, "cursor");

            glUniform2f(resU, resolution.x, resolution.y);
            glUniform4f(
                backgroundColorU,
                elem.backgroundColor.x,
                elem.backgroundColor.y,
                elem.backgroundColor.z,
                elem.backgroundColor.w
            );
            glUniform4f(
                hoverColorU,
                elem.hoverColor.x,
                elem.hoverColor.y,
                elem.hoverColor.z,
                elem.hoverColor.w
            );

            glUniform2f(topleftU, elem.left, elem.top);
            glUniform2f(dimensionsU, elem.width, elem.height);

            glUniform1f(timeU, TIME());
            glUniform1f(borderRadiusU, static_cast<float>(elem.borderRadius));
            glUniform2f(cursorU, cursor.x, cursor.y);

            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

            glm::vec4 textDimensions = renderText(
                elem.text,
                glm::vec2(elem.left + BUTTON_PADDING, elem.top + BUTTON_PADDING),
                resolution,
                elem.color,
                elem.fontSize
            );

            if (elem.width == textDimensions.w + BUTTON_PADDING) {
                elem.reDraw = false;
            } else {
                elem.reDraw = true;
                elem.width = textDimensions.w + BUTTON_PADDING;
                elem.height = textDimensions.z + BUTTON_PADDING - elem.top;
                cout << "\nRecalculating dimensions of button";
            }

            break;
        }

        case UI_SCROLL_AREA: {
            // TODO: Render the rectangle with background colors. If borders
            // are set, then render borders.
            // Clip the content that is not completely inside the defined
            // dimensions. Don't render anything outside the bounding box.

            if (elem.totalChildren) {
                if (elem.reDraw) {
                    elem.extraData->scrollData.showVerticalScrollbar = elem.extraData->scrollData.height > elem.height;
                    elem.extraData->scrollData.showHorizontalScrollbar = elem.extraData->scrollData.width > elem.width;
                }

                // glScissor(elem.left, resolution.y - elem.top + elem.height, elem.width, elem.height);
                glScissor(elem.left, resolution.y - elem.top - elem.height, elem.width, elem.height);
                glEnable(GL_SCISSOR_TEST);

                for (uint32_t i=0; i<elem.totalChildren; ++i) {
                    renderElement(*elem.children[i], resolution, cursor);
                }

                elem.reDraw = false;

                glDisable(GL_SCISSOR_TEST);

                bool update = !elem.extraData->scrollData.renderedUpdate;

                if (elem.extraData->scrollData.showVerticalScrollbar) {
                    if (elem.extraData->scrollData.verticalScrollBarVAO == 0) {
                        float right = elem.left + elem.width;
                        float left = right - 10.0f;
                        float bottom = elem.top + elem.height;

                        float vertices[] = {
                            right, bottom,
                            right, elem.top,
                            left, bottom,
                            left, elem.top,
                        };

                        glGenVertexArrays(1, &elem.extraData->scrollData.verticalScrollBarVAO);
                        glBindVertexArray(elem.extraData->scrollData.verticalScrollBarVAO);

                        if (elem.extraData->scrollData.verticalScrollBarVBO != 0) {
                            glDeleteBuffers(1, &elem.extraData->scrollData.verticalScrollBarVBO);
                        }

                        glGenBuffers(1, &elem.extraData->scrollData.verticalScrollBarVBO);
                        glBindBuffer(GL_ARRAY_BUFFER, elem.extraData->scrollData.verticalScrollBarVBO);
                        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
                        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
                        glEnableVertexAttribArray(0);
                    }

                    glBindVertexArray(elem.extraData->scrollData.verticalScrollBarVAO);
                    glUseProgram(shaders[SHADER_DEFAULT].programId);

                    uint32_t color = glGetUniformLocation(shaders[SHADER_DEFAULT].programId, "solidColor");

                    glUniform4f(color, 0.9f, 0.9f, 0.9f, 1.0f);

                    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

                    if (elem.extraData->scrollData.verticalScrollThumbVAO != 0 && update) {
                        glDeleteVertexArrays(1, &elem.extraData->scrollData.verticalScrollThumbVAO);
                        glDeleteBuffers(1, &elem.extraData->scrollData.verticalScrollThumbVBO);
                        elem.extraData->scrollData.verticalScrollThumbVAO = 0;
                        elem.extraData->scrollData.verticalScrollThumbVBO = 0;
                    }

                    if (elem.extraData->scrollData.verticalScrollThumbVAO == 0) {
                        float height = elem.height;

                        if (elem.extraData->scrollData.showHorizontalScrollbar) {
                            height -= 10.0f;
                        }

                        float right = elem.left + elem.width;
                        float left = right - 10.0f;
                        float top = elem.top + elem.extraData->scrollData.yOffset / elem.extraData->scrollData.height * height;
                        float bottom = top + elem.height / elem.extraData->scrollData.height * height;

                        float vertices[] = {
                            right, bottom,
                            right, top,
                            left, bottom,
                            left, top,
                        };

                        glGenVertexArrays(1, &elem.extraData->scrollData.verticalScrollThumbVAO);
                        glBindVertexArray(elem.extraData->scrollData.verticalScrollThumbVAO);

                        if (elem.extraData->scrollData.verticalScrollThumbVBO != 0) {
                            glDeleteBuffers(1, &elem.extraData->scrollData.verticalScrollThumbVBO);
                        }

                        glGenBuffers(1, &elem.extraData->scrollData.verticalScrollThumbVBO);
                        glBindBuffer(GL_ARRAY_BUFFER, elem.extraData->scrollData.verticalScrollThumbVBO);
                        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
                        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
                        glEnableVertexAttribArray(0);
                        elem.extraData->scrollData.renderedUpdate = true;
                    }

                    glBindVertexArray(elem.extraData->scrollData.verticalScrollThumbVAO);
                    glUseProgram(shaders[SHADER_DEFAULT].programId);

                    color = glGetUniformLocation(shaders[SHADER_DEFAULT].programId, "solidColor");
                    glUniform4f(color, 0.6f, 0.6f, 0.6f, 1.0f);

                    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
                }

                if (elem.extraData->scrollData.showHorizontalScrollbar) {
                    if (elem.extraData->scrollData.horizontalScrollBarVAO == 0) {
                        float bottom = elem.top + elem.height;
                        float top = bottom - 10.0f;
                        float right = elem.left + elem.width;
                        float left = elem.left;

                        float vertices[] = {
                            right, bottom,
                            right, top,
                            left, bottom,
                            left, top,
                        };

                        glGenVertexArrays(1, &elem.extraData->scrollData.horizontalScrollBarVAO);
                        glBindVertexArray(elem.extraData->scrollData.horizontalScrollBarVAO);

                        if (elem.extraData->scrollData.horizontalScrollBarVBO != 0) {
                            glDeleteBuffers(1, &elem.extraData->scrollData.horizontalScrollBarVBO);
                        }

                        glGenBuffers(1, &elem.extraData->scrollData.horizontalScrollBarVBO);
                        glBindBuffer(GL_ARRAY_BUFFER, elem.extraData->scrollData.horizontalScrollBarVBO);
                        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
                        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
                        glEnableVertexAttribArray(0);
                    }

                    glBindVertexArray(elem.extraData->scrollData.horizontalScrollBarVAO);
                    glUseProgram(shaders[SHADER_DEFAULT].programId);

                    uint32_t color = glGetUniformLocation(shaders[SHADER_DEFAULT].programId, "solidColor");

                    glUniform4f(color, 0.9f, 0.9f, 0.9f, 1.0f);

                    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

                    if (elem.extraData->scrollData.horizontalScrollThumbVAO != 0 && update) {
                        glDeleteVertexArrays(1, &elem.extraData->scrollData.horizontalScrollThumbVAO);
                        glDeleteBuffers(1, &elem.extraData->scrollData.horizontalScrollThumbVBO);
                        elem.extraData->scrollData.horizontalScrollThumbVAO = 0;
                        elem.extraData->scrollData.horizontalScrollThumbVBO = 0;
                    }

                    if (elem.extraData->scrollData.horizontalScrollThumbVAO == 0) {
                        float width = elem.width;

                        if (elem.extraData->scrollData.showHorizontalScrollbar) {
                            width -= 10.0f;
                        }

                        float bottom = elem.top + elem.height;
                        float top = bottom - 10.0f;
                        float left = elem.left + elem.extraData->scrollData.xOffset / elem.extraData->scrollData.width * width;
                        float right = left + elem.width / elem.extraData->scrollData.width * width;

                        float vertices[] = {
                            right, bottom,
                            right, top,
                            left, bottom,
                            left, top,
                        };

                        glGenVertexArrays(1, &elem.extraData->scrollData.horizontalScrollThumbVAO);
                        glBindVertexArray(elem.extraData->scrollData.horizontalScrollThumbVAO);

                        if (elem.extraData->scrollData.horizontalScrollThumbVBO != 0) {
                            glDeleteBuffers(1, &elem.extraData->scrollData.horizontalScrollThumbVBO);
                        }

                        glGenBuffers(1, &elem.extraData->scrollData.horizontalScrollThumbVBO);
                        glBindBuffer(GL_ARRAY_BUFFER, elem.extraData->scrollData.horizontalScrollThumbVBO);
                        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
                        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
                        glEnableVertexAttribArray(0);
                        elem.extraData->scrollData.renderedUpdate = true;
                    }

                    glBindVertexArray(elem.extraData->scrollData.horizontalScrollThumbVAO);
                    glUseProgram(shaders[SHADER_DEFAULT].programId);

                    color = glGetUniformLocation(shaders[SHADER_DEFAULT].programId, "solidColor");
                    glUniform4f(color, 0.6f, 0.6f, 0.6f, 1.0f);

                    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
                }
            }
            break;
        }

        case UI_ICON_BUTTON: {
            if (iconButtonVertices[0] == -1.0) {
                // TODO: update this to run on every initialization and changes
                // in appearance settings

                // cout << "\nInitializing icon button vertices";
                iconButtonVertices[0] = 48.0f;
                iconButtonVertices[1] = 48.0f;
                iconButtonVertices[2] = 1.0f;
                iconButtonVertices[3] = 0.0f;

                iconButtonVertices[4] = 48.0f;
                iconButtonVertices[5] = 0.0f;
                iconButtonVertices[6] = 1.0f;
                iconButtonVertices[7] = 1.0f;

                iconButtonVertices[8] = 0.0f;
                iconButtonVertices[9] = 48.0f;
                iconButtonVertices[10] = 0.0f;
                iconButtonVertices[11] = 0.0f;

                iconButtonVertices[12] = 0.0f;
                iconButtonVertices[13] = 0.0f;
                iconButtonVertices[14] = 0.0f;
                iconButtonVertices[15] = 1.0f;

                if (elem.reDraw) {
                    cout << "\nRedrawing icon button";
                    glGenVertexArrays(1, &iconButtonVAO);
                    glBindVertexArray(iconButtonVAO);
                    glGenBuffers(1, &iconButtonVBO);
                    glBindBuffer(GL_ARRAY_BUFFER, iconButtonVBO);
                    glBufferData(GL_ARRAY_BUFFER, sizeof(iconButtonVertices), iconButtonVertices, GL_STATIC_DRAW);
                    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
                    glEnableVertexAttribArray(0);
                    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
                    glEnableVertexAttribArray(1);
                }
            }

            glBindVertexArray(iconButtonVAO);
            glUseProgram(shaders[SHADER_ICON_BUTTON].programId);

            int resU = glGetUniformLocation(shaders[SHADER_ICON_BUTTON].programId, "resolution");
            int colorU = glGetUniformLocation(shaders[SHADER_ICON_BUTTON].programId, "solidColor");
            int bgU = glGetUniformLocation(shaders[SHADER_ICON_BUTTON].programId, "backgroundColor");
            int hoverColorU = glGetUniformLocation(shaders[SHADER_ICON_BUTTON].programId, "hoverColor");
            int timeU = glGetUniformLocation(shaders[SHADER_ICON_BUTTON].programId, "time");
            int topLeftU = glGetUniformLocation(shaders[SHADER_ICON_BUTTON].programId, "topLeft");
            int widthU = glGetUniformLocation(shaders[SHADER_ICON_BUTTON].programId, "width");
            int heightU = glGetUniformLocation(shaders[SHADER_ICON_BUTTON].programId, "height");
            int cursorU = glGetUniformLocation(shaders[SHADER_ICON_BUTTON].programId, "cursor");

            // cout << "icon button color: r: " << elem.color.r << " g: " << elem.color.g << " b: " << elem.color.b << " a: " << elem.color.a;

            glUniform2f(resU, resolution.x, resolution.y);
            glUniform4f(
                bgU,
                elem.backgroundColor.r,
                elem.backgroundColor.g,
                elem.backgroundColor.b,
                elem.backgroundColor.a
            );
            glUniform4f(
                colorU,
                elem.color.r,
                elem.color.g,
                elem.color.b,
                elem.color.a
            );
            glUniform4f(
                hoverColorU,
                elem.hoverColor.r,
                elem.hoverColor.g,
                elem.hoverColor.b,
                elem.hoverColor.a
            );
            glUniform2f(topLeftU, elem.left, elem.top);
            glUniform2f(cursorU, cursor.x, cursor.y);
            glUniform1f(timeU, TIME());
            glUniform1f(widthU, elem.width);
            glUniform1f(heightU, elem.height);

            if (elem.icon != ICON_NONE) {
                if (elem.texture == 0) {
                    glGenTextures(1, &elem.texture);
                    glBindTexture(GL_TEXTURE_2D, elem.texture);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

                    int width, height, nrChannels;

                    stbi_set_flip_vertically_on_load(true);

                    string location = getIconLocation(elem.icon);
                    uint8_t* data = stbi_load(location.c_str(), &width, &height, &nrChannels, 0);

                    if (data) {
                        cout << "\nTexture " << location << " loaded.";
                        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
                        glGenerateMipmap(GL_TEXTURE_2D);
                    } else {
                        cerr << "\nFailed to load texture.";
                    }

                    stbi_image_free(data);
                }

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, elem.texture);
            }

            uint32_t itexU = glGetUniformLocation(shaders[SHADER_ICON_BUTTON].programId, "iconTexture");
            glUniform1i(itexU, 0);

            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

            elem.reDraw = false;
            break;
        }

        case UI_TEXT_INPUT: {
            if (elem.reDraw) {
                cout << "\nRe-drawing Text Input";
                if (elem.vao != 0) {
                    glDeleteVertexArrays(1, &elem.vao);
                }

                if (elem.vbo != 0) {
                    glDeleteBuffers(1, &elem.vbo);
                }

                float right = elem.left + elem.width;
                float bottom = elem.top + elem.height;

                float vertices[] = {
                    right, bottom,
                    right, elem.top,
                    elem.left, bottom,
                    elem.left, elem.top,
                };

                glGenVertexArrays(1, &elem.vao);
                glBindVertexArray(elem.vao);
                glGenBuffers(1, &elem.vbo);
                glBindBuffer(GL_ARRAY_BUFFER, elem.vbo);
                glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
                glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
                glEnableVertexAttribArray(0);
                elem.reDraw = false;
            }

            glBindVertexArray(elem.vao);
            glUseProgram(shaders[SHADER_TEXT_INPUT].programId);

            uint32_t resU = glGetUniformLocation(shaders[SHADER_TEXT_INPUT].programId, "resolution");
            uint32_t colorU = glGetUniformLocation(shaders[SHADER_TEXT_INPUT].programId, "color");
            uint32_t backgroundColorU = glGetUniformLocation(shaders[SHADER_TEXT_INPUT].programId, "backgroundColor");
            uint32_t hoverColorU = glGetUniformLocation(shaders[SHADER_TEXT_INPUT].programId, "hoverColor");
            uint32_t timeU = glGetUniformLocation(shaders[SHADER_TEXT_INPUT].programId, "time");
            uint32_t borderRadiusU = glGetUniformLocation(shaders[SHADER_TEXT_INPUT].programId, "borderRadius");
            uint32_t topleftU = glGetUniformLocation(shaders[SHADER_TEXT_INPUT].programId, "topLeft");
            uint32_t dimensionsU = glGetUniformLocation(shaders[SHADER_TEXT_INPUT].programId, "dimensions");
            uint32_t cursorU = glGetUniformLocation(shaders[SHADER_TEXT_INPUT].programId, "cursor");

            glUniform2f(resU, resolution.x, resolution.y);
            glUniform4f(
                colorU,
                elem.color.x,
                elem.color.y,
                elem.color.z,
                elem.color.w
            );
            glUniform4f(
                backgroundColorU,
                elem.backgroundColor.x,
                elem.backgroundColor.y,
                elem.backgroundColor.z,
                elem.backgroundColor.w
            );
            glUniform4f(
                hoverColorU,
                elem.hoverColor.x,
                elem.hoverColor.y,
                elem.hoverColor.z,
                elem.hoverColor.w
            );

            glUniform2f(topleftU, elem.left, elem.top);
            glUniform2f(dimensionsU, elem.width, elem.height);

            glUniform1f(timeU, TIME());
            glUniform1f(borderRadiusU, static_cast<float>(elem.borderRadius));
            glUniform2f(cursorU, cursor.x, cursor.y);

            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

            int32_t pid = shaders[SHADER_TEXT].programId;

            glUseProgram(pid);

            glUniform3f(
                glGetUniformLocation(pid, "textColor"),
                elem.color.r,
                elem.color.g,
                elem.color.b
            );
            glUniform2f(glGetUniformLocation(pid, "resolution"), resolution.x, resolution.y);
            glActiveTexture(GL_TEXTURE0);
            glBindVertexArray(textVAO);
            glBindBuffer(GL_ARRAY_BUFFER, textVBO);

            uint32_t currentIndex = 0;
            float x = elem.left + BUTTON_PADDING;
            float y = elem.top + BUTTON_PADDING;

            float cursorX = -1;
            float cursorY = -1;
            float selectStartX = -1;
            float selectEndX = -1;

            TextInputData* tid = &elem.extraData->textInputData;

            for (const char c: elem.text) {
                if (currentIndex == tid->cursorPosition) {
                    cursorX = x;
                    cursorY = y;
                }

                if (tid->textSelected) {
                    if (currentIndex == tid->textSelectStartCol) {
                        selectStartX = x;
                    }

                    if (currentIndex == tid->textSelectEndCol) {
                        selectEndX = x;
                    }
                }

                x += renderGlyph(c, x, y, resolution, elem.fontSize);

                ++currentIndex;
            }

            if (tid->cursorPosition == elem.text.length()) {
                cursorX = x;
                cursorY = y;
            }

            if (tid->textSelected && tid->textSelectEndCol == elem.text.length()) {
                cursorX = x;
                cursorY = y;
                selectEndX = x;
            }

            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);
            glBindTexture(GL_TEXTURE_2D, 0);

            // When text input is active, draw the cursor.
            if (elem.active) {
                if (tid->cursorVAO != 0) {
                    glDeleteVertexArrays(1, &tid->cursorVAO);
                }

                if (tid->cursorVBO != 0) {
                    glDeleteBuffers(1, &tid->cursorVBO);
                }

                // cout << "\ncursorX: " << cursorX << " cursorY: " << cursorY;

                float vertices[] = {
                    cursorX - 1, cursorY + elem.fontSize,
                    cursorX + 1, cursorY + elem.fontSize,
                    cursorX - 1, cursorY,
                    cursorX + 1, cursorY,
                };

                glGenVertexArrays(1, &tid->cursorVAO);
                glBindVertexArray(tid->cursorVAO);
                glGenBuffers(1, &tid->cursorVBO);
                glBindBuffer(GL_ARRAY_BUFFER, tid->cursorVBO);
                glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
                glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
                glEnableVertexAttribArray(0);

                glUseProgram(shaders[SHADER_TEXT_CURSOR].programId);

                uint32_t colorU = glGetUniformLocation(shaders[SHADER_TEXT_CURSOR].programId, "color");
                uint32_t resU = glGetUniformLocation(shaders[SHADER_TEXT_CURSOR].programId, "resolution");
                uint32_t timeU = glGetUniformLocation(shaders[SHADER_TEXT_CURSOR].programId, "time");

                glm::vec4 color = CURSOR_COLOR;

                if (darkTheme) {
                    color = CURSOR_COLOR_DARK;
                }

                glUniform4f(colorU, color.x, color.y, color.z, color.w);
                glUniform2f(resU, resolution.x, resolution.y);
                glUniform1f(timeU, TIME() - tid->lastActiveTime);

                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

                if (tid->textSelected && selectStartX != -1 && selectEndX != -1) {
                    if (tid->textSelectUpdated) {
                        if (tid->textSelectVAO != 0) {
                            glDeleteVertexArrays(1, &tid->textSelectVAO);
                        }

                        if (tid->textSelectVBO != 0) {
                            glDeleteBuffers(1, &tid->textSelectVBO);
                        }

                        float vertices[] = {
                            selectStartX, cursorY + elem.fontSize,
                            selectEndX, cursorY + elem.fontSize,
                            selectStartX, cursorY,
                            selectEndX, cursorY,
                        };

                        glGenVertexArrays(1, &tid->textSelectVAO);
                        glBindVertexArray(tid->textSelectVAO);
                        glGenBuffers(1, &tid->textSelectVBO);
                        glBindBuffer(GL_ARRAY_BUFFER, tid->textSelectVBO);
                        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
                        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
                        glEnableVertexAttribArray(0);

                        tid->textSelectUpdated = false;
                        #ifdef DEBUG_TEXT_SELECTION
                            cout << "\ncursorY: " << cursorY;
                        #endif
                    }

                    glBindVertexArray(tid->textSelectVAO);

                    glUseProgram(shaders[SHADER_DEFAULT].programId);

                    uint32_t colorU = glGetUniformLocation(shaders[SHADER_DEFAULT].programId, "solidColor");

                    glm::vec4 color = CURSOR_COLOR;

                    if (darkTheme) {
                        color = CURSOR_COLOR_DARK;
                    }

                    glUniform4f(colorU, color.x, color.y, color.z, 0.25);

                    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
                }
            }

            break;
        }

        case UI_TEXT_AREA: {
            if (elem.reDraw) {
                cout << "\nRe-drawing Text Area" << "\nwidth: " << elem.width << ", height: " << elem.height;
                if (elem.vao != 0) {
                    glDeleteVertexArrays(1, &elem.vao);
                }

                if (elem.vbo != 0) {
                    glDeleteBuffers(1, &elem.vbo);
                }

                float right = elem.left + elem.width;
                float bottom = elem.top + elem.height;

                float vertices[] = {
                    right, bottom,
                    right, elem.top,
                    elem.left, bottom,
                    elem.left, elem.top,
                };

                glGenVertexArrays(1, &elem.vao);
                glBindVertexArray(elem.vao);
                glGenBuffers(1, &elem.vbo);
                glBindBuffer(GL_ARRAY_BUFFER, elem.vbo);
                glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
                glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
                glEnableVertexAttribArray(0);
                elem.reDraw = false;
            }

            glBindVertexArray(elem.vao);
            glUseProgram(shaders[SHADER_TEXT_INPUT].programId);

            uint32_t resU = glGetUniformLocation(shaders[SHADER_TEXT_INPUT].programId, "resolution");
            uint32_t colorU = glGetUniformLocation(shaders[SHADER_TEXT_INPUT].programId, "color");
            uint32_t backgroundColorU = glGetUniformLocation(shaders[SHADER_TEXT_INPUT].programId, "backgroundColor");
            uint32_t hoverColorU = glGetUniformLocation(shaders[SHADER_TEXT_INPUT].programId, "hoverColor");
            uint32_t timeU = glGetUniformLocation(shaders[SHADER_TEXT_INPUT].programId, "time");
            uint32_t borderRadiusU = glGetUniformLocation(shaders[SHADER_TEXT_INPUT].programId, "borderRadius");
            uint32_t topleftU = glGetUniformLocation(shaders[SHADER_TEXT_INPUT].programId, "topLeft");
            uint32_t dimensionsU = glGetUniformLocation(shaders[SHADER_TEXT_INPUT].programId, "dimensions");
            uint32_t cursorU = glGetUniformLocation(shaders[SHADER_TEXT_INPUT].programId, "cursor");

            glUniform2f(resU, resolution.x, resolution.y);
            glUniform4f(
                colorU,
                elem.color.x,
                elem.color.y,
                elem.color.z,
                elem.color.w
            );
            glUniform4f(
                backgroundColorU,
                elem.backgroundColor.x,
                elem.backgroundColor.y,
                elem.backgroundColor.z,
                elem.backgroundColor.w
            );
            glUniform4f(
                hoverColorU,
                elem.hoverColor.x,
                elem.hoverColor.y,
                elem.hoverColor.z,
                elem.hoverColor.w
            );

            glUniform2f(topleftU, elem.left, elem.top);
            glUniform2f(dimensionsU, elem.width, elem.height);

            glUniform1f(timeU, TIME());
            glUniform1f(borderRadiusU, static_cast<float>(elem.borderRadius));
            glUniform2f(cursorU, cursor.x, cursor.y);

            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

            int32_t pid = shaders[SHADER_TEXT].programId;

            glUseProgram(pid);

            glUniform3f(
                glGetUniformLocation(pid, "textColor"),
                elem.color.r,
                elem.color.g,
                elem.color.b
            );
            glUniform2f(glGetUniformLocation(pid, "resolution"), resolution.x, resolution.y);
            glActiveTexture(GL_TEXTURE0);
            glBindVertexArray(textVAO);
            glBindBuffer(GL_ARRAY_BUFFER, textVBO);

            uint32_t currentIndex = 0;
            uint32_t row = 0;
            uint32_t col = 0;
            const float xStart = elem.left + BUTTON_PADDING; 
            float x = xStart;
            float y = elem.top + BUTTON_PADDING;

            float cursorX = -1;
            float cursorY = -1;
            float selectStartX = -1;
            float selectStartY = -1;
            float selectEndX = -1;
            float selectEndY = -1;

            bool selectStarted = false;
            bool selectEnded = false;

            TextInputData* tid = &elem.extraData->textInputData;

            vector<float> selectionVertices;
            selectionVertices.reserve(18);

            const float newLineSize = static_cast<float>(elem.fontSize)
                + static_cast<float>(BUTTON_PADDING);

            const float heightIncrement = static_cast<float>(elem.fontSize + LINE_GAP);

            for (const char c: elem.text) {
                if (c == '\n') {
                    if (tid->textSelected && selectStarted && !selectEnded) {
                        const float y1 = y + elem.fontSize;

                        selectEndX = x;
                        selectEndY = y;

                        selectionVertices.push_back(selectStartX);
                        selectionVertices.push_back(y1);
                        selectionVertices.push_back(selectEndX);
                        selectionVertices.push_back(y1);
                        selectionVertices.push_back(selectStartX);
                        selectionVertices.push_back(y);

                        selectionVertices.push_back(selectEndX);
                        selectionVertices.push_back(y1);
                        selectionVertices.push_back(selectStartX);
                        selectionVertices.push_back(y);
                        selectionVertices.push_back(selectEndX);
                        selectionVertices.push_back(y);
                    }

                    y += heightIncrement;
                    x = xStart;
                    col = 0;
                    ++row;

                    selectStartX = x;
                    selectStartY = y;
                    continue;
                }

                if (currentIndex == tid->cursorPosition) {
                    cursorX = x;
                    cursorY = y;
                }

                if (tid->textSelected && !selectEnded) {
                    if (col == tid->textSelectStartCol && row == tid->textSelectStartRow) {
                        selectStartX = x;
                        selectStartY = y;
                        selectStarted = true;
                    }

                    if (col == tid->textSelectEndCol && row == tid->textSelectEndRow) {
                        selectEndX = x;
                        selectEndY = y;
                        selectEnded = true;

                        const float y1 = y + elem.fontSize;

                        selectionVertices.push_back(selectStartX);
                        selectionVertices.push_back(y1);
                        selectionVertices.push_back(selectEndX);
                        selectionVertices.push_back(y1);
                        selectionVertices.push_back(selectStartX);
                        selectionVertices.push_back(y);

                        selectionVertices.push_back(selectEndX);
                        selectionVertices.push_back(y1);
                        selectionVertices.push_back(selectStartX);
                        selectionVertices.push_back(y);
                        selectionVertices.push_back(selectEndX);
                        selectionVertices.push_back(y);
                    }
                }

                x += renderGlyph(c, x, y, resolution, elem.fontSize);
                ++col;

                ++currentIndex;
            }

            if (tid->cursorPosition == elem.text.length()) {
                cursorX = x;
                cursorY = y;
            }

            if (tid->textSelected && tid->textSelectEndCol == elem.text.length()) {
                cursorX = x;
                cursorY = y;
                selectEndX = x;

                const float y1 = y + elem.fontSize;
                selectionVertices.push_back(selectStartX);
                selectionVertices.push_back(y1);
                selectionVertices.push_back(selectEndX);
                selectionVertices.push_back(y1);
                selectionVertices.push_back(selectStartX);
                selectionVertices.push_back(y);

                selectionVertices.push_back(selectEndX);
                selectionVertices.push_back(y1);
                selectionVertices.push_back(selectStartX);
                selectionVertices.push_back(y);
                selectionVertices.push_back(selectEndX);
                selectionVertices.push_back(y);
            }

            #ifdef DEBUG_TEXT_SELECTION
                if (tid->textSelected) {
                    printTextSelectionVertices(selectionVertices);
                }
            #endif

            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);
            glBindTexture(GL_TEXTURE_2D, 0);

            // When text input is active, draw the cursor.
            if (elem.active) {
                if (tid->cursorVAO != 0) {
                    glDeleteVertexArrays(1, &tid->cursorVAO);
                }

                if (tid->cursorVBO != 0) {
                    glDeleteBuffers(1, &tid->cursorVBO);
                }

                // cout << "\ncursorX: " << cursorX << " cursorY: " << cursorY;

                float vertices[] = {
                    cursorX - 1, cursorY + elem.fontSize,
                    cursorX + 1, cursorY + elem.fontSize,
                    cursorX - 1, cursorY,
                    cursorX + 1, cursorY,
                };

                glGenVertexArrays(1, &tid->cursorVAO);
                glBindVertexArray(tid->cursorVAO);
                glGenBuffers(1, &tid->cursorVBO);
                glBindBuffer(GL_ARRAY_BUFFER, tid->cursorVBO);
                glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
                glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
                glEnableVertexAttribArray(0);

                glUseProgram(shaders[SHADER_TEXT_CURSOR].programId);

                uint32_t colorU = glGetUniformLocation(shaders[SHADER_TEXT_CURSOR].programId, "color");
                uint32_t resU = glGetUniformLocation(shaders[SHADER_TEXT_CURSOR].programId, "resolution");
                uint32_t timeU = glGetUniformLocation(shaders[SHADER_TEXT_CURSOR].programId, "time");

                glm::vec4 color = CURSOR_COLOR;

                if (darkTheme) {
                    color = CURSOR_COLOR_DARK;
                }

                glUniform4f(colorU, color.x, color.y, color.z, color.w);
                glUniform2f(resU, resolution.x, resolution.y);
                glUniform1f(timeU, TIME() - tid->lastActiveTime);

                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

                if (tid->textSelected && selectStartX != -1 && selectEndX != -1) {
                    if (tid->textSelectUpdated && !selectionVertices.empty()) {
                        if (tid->textSelectVAO != 0) {
                            glDeleteVertexArrays(1, &tid->textSelectVAO);
                        }

                        if (tid->textSelectVBO != 0) {
                            glDeleteBuffers(1, &tid->textSelectVBO);
                        }

                        // float vertices[] = {
                        //     selectStartX, cursorY + elem.fontSize,
                        //     selectEndX, cursorY + elem.fontSize,
                        //     selectStartX, cursorY,
                        //     selectEndX, cursorY,
                        // };

                        glGenVertexArrays(1, &tid->textSelectVAO);
                        glBindVertexArray(tid->textSelectVAO);
                        glGenBuffers(1, &tid->textSelectVBO);
                        glBindBuffer(GL_ARRAY_BUFFER, tid->textSelectVBO);
                        glBufferData(GL_ARRAY_BUFFER, selectionVertices.size() * sizeof(float), &selectionVertices[0], GL_STATIC_DRAW);
                        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
                        glEnableVertexAttribArray(0);

                        tid->textSelectUpdated = false;
                        #ifdef DEBUG_TEXT_SELECTION
                            cout << "\ncursorY: " << cursorY;
                        #endif
                    }

                    glBindVertexArray(tid->textSelectVAO);

                    glUseProgram(shaders[SHADER_DEFAULT].programId);

                    uint32_t colorU = glGetUniformLocation(shaders[SHADER_DEFAULT].programId, "solidColor");

                    glm::vec4 color = CURSOR_COLOR;

                    if (darkTheme) {
                        color = CURSOR_COLOR_DARK;
                    }

                    glUniform4f(colorU, color.x, color.y, color.z, 0.25);

                    glDrawArrays(GL_TRIANGLES, 0, selectionVertices.size() / 2);
                }
            }

            break;
        }

        case UI_TAB_BAR: {
            break;
        }

        case UI_TAB: {
            break;
        }

        default: break;
    }
}

void processRedo(UIElement* element) {
    #define CUR_CMD element->extraData->textInputData.cmdList.currentCommand
    #define CUR_POS element->extraData->textInputData.cursorPosition

    Command* cmd = CUR_CMD;

    if (cmd == nullptr || cmd->next == nullptr) {
        return;
    }

    moveToNextCommand(element->extraData->textInputData.cmdList);

    cmd = CUR_CMD;

    switch (cmd->type) {
        case COMMAND_TYPE: {
            // cout << "\nCursor Position: " << elem->extraData->textInputData.cursorPosition;
            if (cmd->cursorPos == element->text.length()) {
                // cout << "\nAppending at the end! " << cmd->str.length();
                element->text += cmd->str;
                // elem->extraData->textInputData.cursorPosition += cmd->str.length();
                break;
            }

            element->text.insert(cmd->cursorPos, cmd->str);
            CUR_POS = cmd->cursorPos + 1;
            break;
        }

        case COMMAND_DELETE: {
            element->text.erase(cmd->cursorPos, 1);
            CUR_POS = cmd->cursorPos;
            break;
        }

        case COMMAND_BACKSPACE: {
            element->text.erase(cmd->cursorPos-1, 1);
            CUR_POS = cmd->cursorPos - 1;
            break;
        }

        case COMMAND_REPLACE: {
            TextInputData* tid = &element->extraData->textInputData;
            element->text.erase(tid->textSelectStartCol, cmd->replace.prevString.length());

            if (!cmd->replace.newString.empty()) {
                element->text.insert(tid->textSelectStartCol, cmd->replace.newString);
            }

            CUR_POS = cmd->cursorPos;
            break;
        }

        default:
            // Nothing!
            break;
    }

    #undef CUR_CMD
    #undef CUR_POS
}

void processUndo(UIElement* element) {
    #define CUR_POS element->extraData->textInputData.cursorPosition

    TextInputData* tid = &element->extraData->textInputData;

    if (tid == nullptr) { return; }

    Command* currentCmd = tid->cmdList.currentCommand;

    if (currentCmd == nullptr) { return; }

    textInputUnselect(tid);

    switch (currentCmd->type) {
        case COMMAND_TYPE: {
            element->text.erase(currentCmd->cursorPos, 1);
            moveToPrevCommand(tid->cmdList);
            CUR_POS = currentCmd->cursorPos;

            uint32_t newLines = 0;

            for (char c: currentCmd->str) {
                if (c == '\n') { ++newLines; }
            }

            if (newLines > 0) {
                element->height -= static_cast<float>(element->fontSize + LINE_GAP);
                element->reDraw = true;
            }
            break;
        }
        case COMMAND_BACKSPACE: {
            element->text.insert(currentCmd->cursorPos-1, currentCmd->str);
            moveToPrevCommand(tid->cmdList);
            CUR_POS = currentCmd->cursorPos;
            break;
        }
        case COMMAND_DELETE: {
            element->text.insert(currentCmd->cursorPos, currentCmd->str);
            moveToPrevCommand(tid->cmdList);
            CUR_POS = currentCmd->cursorPos;
            break;
        }

        case COMMAND_REPLACE: {
            element->text.erase(currentCmd->replace.selectColStart, currentCmd->replace.newString.length());
            element->text.insert(currentCmd->replace.selectColStart, currentCmd->replace.prevString);
            moveToPrevCommand(tid->cmdList);
            CUR_POS = currentCmd->cursorPos;
            break;
        }

        default:
            // Nothing!
            break;
    }
    #undef CUR_POS
}

UIElement* getElementFromScreenCoords(UIElement * root, float x, float y) {
    // Check if the cursor is above the exact same position as it was when
    // mouse was down.
    // TODO: check instead if the mouse down element and mouse release
    // elements are the same.
    stack<UIElement*> elemStack;

    for (uint8_t i=0; i<root->totalChildren; ++i) {
        elemStack.push(root->children[(int)i]);
    }

    UIElement* elem = nullptr;

    while (!elemStack.empty()) {
        elem = elemStack.top();

        if (
            elem->type == UI_BUTTON || elem->type == UI_TEXT_INPUT
            || elem->type == UI_TEXT_AREA || elem->type == UI_SCROLL_AREA
        ) {
            if (
                x > elem->left && x < elem->left + elem->width
                && y > elem->top && y < elem->top + elem->height
            ) {
                return elem;
            }
        }

        elemStack.pop();

        if (elem->totalChildren > 0) {
            for (uint8_t i=0; i<elem->totalChildren; ++i) {
                elemStack.push(elem->children[i]);
            }
        }
    }

    return nullptr;
}

void processCommand(UIElement* elem, Command* cmd) {
    #define CUR_POS elem->extraData->textInputData.cursorPosition
    if (
        (elem->type == UI_TEXT_INPUT || elem->type == UI_TEXT_AREA)
        && elem->extraData != nullptr
    ) {
        bool append = false;

        TextInputData * tid = &elem->extraData->textInputData;

        switch(cmd->type) {
            case COMMAND_NONE: break;
            case COMMAND_TYPE: {
                if (tid->textSelected) {
                    cmd = replaceSelectedText(elem, cmd);
                } else {
                    if (CUR_POS == elem->text.length()) {
                        elem->text += cmd->str;
                        append = true;
                        uint32_t newLines = 0;

                        for (char c: cmd->str) {
                            if (c == '\n') { ++newLines; }
                        }

                        if (newLines > 0) {
                            elem->height += static_cast<float>(elem->fontSize + LINE_GAP);
                            elem->reDraw = true;
                        }

                        break;
                    }

                    elem->text.insert(CUR_POS, cmd->str);
                }
                append = true;
                break;
            }

            case COMMAND_DELETE: {
                uint32_t newLines = 0;

                for (char c: cmd->str) {
                    if (c == '\n') { ++newLines; }
                }

                if (tid->textSelected) {
                    cmd = replaceSelectedText(elem, cmd);
                } else {
                    elem->text.erase(CUR_POS, 1);
                }


                if (newLines > 0 && elem->height > 32.0) {
                    elem->height -= static_cast<float>(elem->fontSize + LINE_GAP);
                    elem->reDraw = true;
                }
                append = true;
                break;
            }

            case COMMAND_BACKSPACE: {
                uint32_t newLines = 0;

                for (char c: cmd->str) {
                    if (c == '\n') { ++newLines; }
                }

                if (tid->textSelected) {
                    cmd = replaceSelectedText(elem, cmd);
                } else {
                    elem->text.erase(CUR_POS-1, 1);
                }

                append = true;

                if (newLines > 0 && elem->height > 32.0) {
                    elem->height -= static_cast<float>(elem->fontSize + LINE_GAP);
                    elem->reDraw = true;
                }
                break;
            }

            case COMMAND_UNDO:
                processUndo(elem);
                break;

            case COMMAND_REDO:
                processRedo(elem);
                break;

            default:
                // Nothing!
                break;
        }

        if (append) {
            appendCommand(elem->extraData->textInputData.cmdList, cmd);
        }

        #ifdef DEBUG_COMMANDS
            printCommands(elem->extraData->textInputData.cmdList);
        #endif
    }
    #undef CUR_POS
}

string getIconLocation(Icon icon) {
    string path = "assets/images/";

    switch (icon) {
        case ICON_NONE: return "";
        case ICON_PLUS: path += "plus.png"; break;
        case ICON_FILE: path += "file.png"; break;
    }

    return path;
}

string elementType(UIElement *elem) {
    switch(elem->type) {
        case UI_LABEL: return "UI Element Type: Label";
        case UI_BUTTON: return "UI Element Type: Button";
        case UI_ICON_BUTTON: return "UI Element Type: Icon Button";
        case UI_CONTAINER: return "UI Element Type: Container";
        case UI_TEXT_INPUT: return "UI Element Type: Text Input";
        case UI_TEXT_AREA: return "UI Element Type: Text Area";
        case UI_TAB_BAR: return "UI Element Type: Tab Bar";
        case UI_TAB: return "UI Element Type: Tab";
        case UI_SCROLL_AREA: return "UI Element Type: Scroll Area";
    }

    return "UI Element Type: Unknown";
}

#ifdef DEBUG_TEXT_SELECTION
    float debug_lastTextSelTime = 0;
    float debug_lastTid = 0;

    void printTextSelectionVertices(vector<float> &fvec) {
        if (!(debug_lastTextSelTime + 1.0 < glfwGetTime() || debug_lastTextSelTime == 0)) {
            return;
        }

        bool pair = false;

        cout << "\nSelection vertices: ";

        for (float c: fvec) {
            if (pair) {
                cout << ", " << c << "), ";
            } else {
                cout << "(" << c;
            }

            pair = !pair;
        }

        debug_lastTextSelTime = glfwGetTime();
    }

    void printTextInputData(TextInputData* tid, string token) {
        // if (!(debug_lastTid + 1.0 < glfwGetTime() || debug_lastTid == 0)) {
        //     return;
        // }

        if (token.empty()) {
            cout << "\nText Input Data:";
        } else {
            cout << "\nText Input Data(" << token << "):";
        }

        cout << " Selection Start: (" << tid->textSelectStartRow << ", "
            << tid->textSelectStartCol << ") Selection End: ("
            << tid->textSelectEndRow << ", " << tid->textSelectEndCol << ")";

        debug_lastTid = glfwGetTime();
    }
#endif

