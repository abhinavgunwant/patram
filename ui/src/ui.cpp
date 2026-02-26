#include <algorithm>
#include <iostream>
#include <stack>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "include/ui.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "include/command.hpp"
#include "include/shader.hpp"
#include "include/text.hpp"
#include "../lib/stb_image.h"

#define TIME() static_cast<float>(glfwGetTime())
#define LAST_IN_LIST(item) while (item->next != nullptr) { item = item->next; }

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

float contentLeft = CONTENT_HORIZONTAL_MARGIN;
float contentRight = 550.0f;

HighlightedContentBlock highlightedContentBlock = {
    .cb = nullptr,
    .parentElement = nullptr,
    .show =  false,
    .updated =  false,
    .mouseDown = false,
    .top =  0.0f,
    .left =  0.0f,
    .bottom =  0.0f,
    .right =  0.0f,
    .backgroundVAO = 0,
    .backgroundVBO = 0,
    .dragTabVAO = 0,
    .dragTabVBO = 0,
};

ContentBlockMoveMarker cbMove = { nullptr, 0, 0, 0, false, true,  };

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

    #ifdef DEBUG_TEXT_INPUT
        cout << "\n Key: ";

        if (alt) {
            cout << "Alt + ";
        }

        if (shift) {
            cout << "Shift + ";
        }

        if (ctrl) {
            cout << "Control + ";
        }

        if (capsLck) {
            cout << "Caps Lock + ";
        }

        if (numLck) {
            cout << "Num Lock + ";
        }

        cout << key;
    #endif

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
                #ifdef DEBUG_TEXT_INPUT
                    cout << "\nEnter pressed";
                #endif

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

void scrollAreaMouseDownCallback(UIElement* elem, float mouseX, float mouseY, glm::vec2 res) {
    if (elem->type == UI_SCROLL_AREA || elem->type == UI_CONTENT_CONTAINER) {
        float top, left, bottom, right, thumbStart, thumbEnd;

        if (elem->extraData->scrollData.showVerticalScrollbar) {
            getScrollbarDimensions(
                elem, top, right, bottom, left,
                thumbStart, thumbEnd,
                true
            );

            if (
                isPointInside(mouseX, mouseY, left, right, top, bottom)
                && !isPointInside(mouseX, mouseY, left, right, thumbStart, thumbEnd)
            ) {
                float height = elem->height;
                float barHeight = thumbEnd - thumbStart;
                float newThumbStart = mouseY - elem->top - barHeight/2;

                if (newThumbStart < 0) {
                    newThumbStart = 0;
                }

                if (elem->extraData->scrollData.showHorizontalScrollbar) {
                    height -= SCROLL_BAR_THICKNESS;
                }

                if (newThumbStart + barHeight > height) {
                    newThumbStart = elem->top + height - barHeight;
                }

                elem->extraData->scrollData.yOffset = newThumbStart * elem->extraData->scrollData.height / height;
                elem->extraData->scrollData.renderedUpdate = false;
                elem->extraData->scrollData.lastScrolled = glfwGetTime();
                return;
            }
        }

        if (elem->extraData->scrollData.showHorizontalScrollbar) {
            getScrollbarDimensions(
                elem, top, right, bottom, left,
                thumbStart, thumbEnd,
                false
            );

            if (
                isPointInside(mouseX, mouseY, left, right, top, bottom)
                && !isPointInside(mouseX, mouseY, left, right, thumbStart, thumbEnd)
            ) {
                float width = elem->width;
                float barWidth = thumbEnd - thumbStart;
                float newThumbStart = mouseX - elem->left - barWidth/2;

                if (newThumbStart < 0) {
                    newThumbStart = 0;
                }

                if (elem->extraData->scrollData.showVerticalScrollbar) {
                    width -= SCROLL_BAR_THICKNESS;
                }

                if (newThumbStart + barWidth > elem->width) {
                    newThumbStart = elem->left + width - barWidth;
                }

                elem->extraData->scrollData.xOffset = newThumbStart * elem->extraData->scrollData.width / width;
                elem->extraData->scrollData.renderedUpdate = false;
                elem->extraData->scrollData.lastScrolled = glfwGetTime();
            }
        }
    }
}

void scrollAreaMouseDragCallback(UIElement* elem, float mouseX, float mouseY, glm::vec2 res) {
    if (glfwGetTime() - elem->extraData->scrollData.lastScrolled < SCROLL_DEBOUNCE_DELAY) {
        return;
    }

    scrollAreaMouseDownCallback(elem, mouseX, mouseY, res);
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

    glm::vec4 backgroundColor = TRANSPARENT;
    glm::vec4 hoverColor = TRANSPARENT;
    glm::vec4 color = darkTheme ? WHITE : BLACK;
    uint8_t borderRadius = 0;
    UIPositioning positioning = UI_POSITIONING_ABSOLUTE;

    uint8_t fontSize = 16;

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

        case UI_CONTENT_CONTAINER:
        case UI_SCROLL_AREA:
            pref = initPreferences();
            pref->widthPreference = UI_GROW;

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

            onMouseDown = scrollAreaMouseDownCallback;
            onMouseDrag = scrollAreaMouseDragCallback;
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

        Icon::None,

        fontSize,
        borderRadius,
        positioning,
        pref,
        extraData,
        nullptr,
    };
}

ContentBlock* initContentBlock(ContentBlockType type, string text) {
    return new ContentBlock {
        .type = type,
        .flags = CONTENT_BLOCK_REDRAW | CONTENT_BLOCK_VISIBLE,

        .top = 0.0f,
        .left = 0.0f,
        .height = 0.0f,

        .rope = new Rope(text),

        .prev = nullptr,
        .next = nullptr,
    };
}

ContentBlock* addContentBlock(UIElement& parentElement, ContentBlockType type, string text, ContentBlock* addAfter) {
    if (parentElement.contentBlocks == nullptr) {
        parentElement.contentBlocks = initContentBlock(type, text);
        return parentElement.contentBlocks;
    }

    ContentBlock* block = initContentBlock(type, text);

    if (addAfter != nullptr) {
        block->prev = addAfter;
        block->next = addAfter->next;

        addAfter->next = block;
        return block;
    }

    ContentBlock* last = parentElement.contentBlocks;

    LAST_IN_LIST(last);

    last->next = block;
    block->prev = last;

    return block;
}

void moveContentBlock(UIElement* parentElement, ContentBlock* cb, ContentBlock* before = nullptr) {
    if (parentElement->contentBlocks == nullptr) {
        cout << "\nError: There are no content blocks in the parent element!";
        return;
    }

    if (cb == nullptr) {
        cout << "\nError: There is no content block to move!";
        return;
    }

    if (cb == before) {
        return;
    }

    if (before != nullptr && before->prev == cb) {
        cout << "\nThis element is in the right place. Aborting...";
        return;
    }

    cb->flags |= CONTENT_BLOCK_REDRAW;

    if (cb->prev != nullptr) {
        cb->prev->next = cb->next;
    }

    if (cb->next != nullptr) {
        cb->next->prev = cb->prev;
    }

    if (before == nullptr) {
        cout << "\nMoving after last element.";
        auto last = parentElement->contentBlocks;

        while (last->next != nullptr) {
            last = last->next;
        }

        last->next = cb;
        cb->prev = last;
        cb->next = nullptr;
        return;
    }

    before->flags |= CONTENT_BLOCK_REDRAW;

    if (before == parentElement->contentBlocks) {
        cb->next = parentElement->contentBlocks->next;
        cb->prev = parentElement->contentBlocks;
        parentElement->contentBlocks->next->prev = cb;
        parentElement->contentBlocks->next = cb;
        cout << "\nError: There is no content block in the \"before\"!";
        return;
    }

    cb->next = before;

    if (before->prev != nullptr) {
        before->prev->next = cb;
        cb->prev = before->prev;
    }

    before->prev = cb;

    cout << "\nFinished moving content block!";

    #ifdef DEBUG_CONTENT_BLOCKS
        printContentBlocks(parentElement);
    #endif
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
    bool updated = false;

    if (elem->preferences != nullptr) {
        switch (elem->preferences->widthPreference) {
            case UI_FULL:
                if (elem->parent != nullptr) {
                    #ifdef DEBUG_LAYOUT
                        cout << "\n-> Resizing width of element (text: "
                            << elem->text << ") with UI_FULL width preference.";
                    #endif

                    elem->width = elem->parent->width;
                    updated = true;
                }
                break;

            case UI_STRETCH:
                if (elem->parent != nullptr) {
                    #ifdef DEBUG_LAYOUT
                        cout << "\n-> Resizing width of element (text: "
                            << elem->text << ") with UI_STRETCH width preference";
                    #endif

                    float width = elem->parent->width;

                    UIElement* siblingElem = nullptr;

                    uint32_t totalChildren = elem->parent->totalChildren;
                    uint32_t stretchCandidates = 0;

                    for (uint32_t i=0; i<elem->parent->totalChildren; ++i) {
                        siblingElem = elem->parent->children[i];

                        if (siblingElem != elem && siblingElem->preferences != nullptr) {
                            switch (siblingElem->preferences->widthPreference) {
                                case UI_FIXED:
                                case UI_GROW:
                                    width -= siblingElem->width;
                                    break;

                                case UI_PERCENT:
                                    width -= elem->parent->width * siblingElem->width;

                                case UI_STRETCH:
                                    ++stretchCandidates;
                                    break;

                                default:
                                    // Do nothing!
                                    break;
                            }
                        }
                    }

                    if (stretchCandidates) {
                        width /= stretchCandidates;
                    }

                    if (width < 0) {
                        #ifdef DEBUG_LAYOUT
                            cout << "\nLAYOUT ERROR: width less than 0";
                        #endif
                        break;
                    }

                    elem->width = width;

                    if (elem->type == UI_SCROLL_AREA && elem->extraData->scrollData.width < width) {
                        cout << "\n!!!!!";
                        elem->extraData->scrollData.width = width;
                        elem->extraData->scrollData.xOffset = 0;
                    }

                    updated = true;
                }
                break;

            default:
                break;
        }

        switch (elem->preferences->heightPreference) {
            case UI_FULL:
                if (elem->parent != nullptr) {
                    #ifdef DEBUG_LAYOUT
                        cout << "\n-> Resizing height of element (text: "
                            << elem->text << ") with UI_FULL height preference.";
                    #endif

                    elem->height = elem->parent->height;
                    updated = true;
                }
                break;

            case UI_STRETCH:
                if (elem->parent != nullptr) {
                    #ifdef DEBUG_LAYOUT
                        cout << "\n-> Resizing height of element (text: "
                            << elem->text << ") with UI_STRETCH height preference.";
                    #endif

                    float height = elem->parent->height;

                    UIElement* siblingElem = nullptr;

                    uint32_t totalChildren = elem->parent->totalChildren;
                    uint32_t stretchCandidates = 0;

                    for (uint32_t i=0; i<elem->parent->totalChildren; ++i) {
                        siblingElem = elem->parent->children[i];

                        if (siblingElem != elem && siblingElem->preferences != nullptr) {
                            switch (siblingElem->preferences->heightPreference) {
                                case UI_FIXED:
                                case UI_GROW:
                                    height -= siblingElem->height;
                                    break;

                                case UI_PERCENT:
                                    height -= elem->parent->height * siblingElem->height;

                                case UI_STRETCH:
                                    ++stretchCandidates;
                                    break;

                                default:
                                    // Do nothing!
                                    break;
                            }
                        }
                    }

                    if (stretchCandidates) {
                        height /= stretchCandidates;
                    }

                    if (height < 0) {
                        cout << "\nLAYOUT ERROR: height less than 0";
                        break;
                    }

                    elem->height = height;
                    updated = true;
                }
                break;

            default:
                break;
        }
    }

    if (updated) {
        #ifdef DEBUG_SCROLL_AREA
            cout << "\n    -> Updated dimensions: width: " << elem->width << ", height: " << elem->height;
        #endif

        if (elem->type == UI_SCROLL_AREA || elem->type == UI_CONTENT_CONTAINER) {
            elem->extraData->scrollData.renderedUpdate = false;
            elem->reDraw = true;

            if (elem->contentBlocks != nullptr) {
                reDrawAllContentBlocks(elem);

                elem->extraData->scrollData.height = 0;
            }

            if (elem->extraData->scrollData.width < elem->width) {
                elem->extraData->scrollData.xOffset = 0;
            }
        }
    }

    for (uint32_t i=0; i<elem->totalChildren; ++i) {
        recalculateLayout(elem->children[i]);
    }
}

// NOTE: only for UI_CONTENT_CONTAINER and UI_SCROLL_AREA.
inline void renderScrollbars(UIElement* elem) {
    #define SCROLL_DATA elem->extraData->scrollData

    bool update = !SCROLL_DATA.renderedUpdate;

    if (SCROLL_DATA.showVerticalScrollbar) {
        // WARNING: side effects!
        float top, left, bottom, right, thumbStart, thumbEnd;
        bool dimensionsInitialized = false;

        if (SCROLL_DATA.verticalScrollBarVAO != 0 && update) {
            glDeleteVertexArrays(1, &SCROLL_DATA.verticalScrollBarVAO);
            SCROLL_DATA.verticalScrollBarVAO = 0;
            #ifdef DEBUG_SCROLL_AREA
                cout << "\nCleared vertical scrollbar vertex array";
            #endif

            if (SCROLL_DATA.verticalScrollBarVBO != 0) {
                glDeleteBuffers(1, &SCROLL_DATA.verticalScrollBarVAO);
                SCROLL_DATA.verticalScrollBarVBO = 0;

                #ifdef DEBUG_SCROLL_AREA
                    cout << "\nCleared vertical scrollbar vertex buffers";
                #endif
            }
        }

        if (SCROLL_DATA.verticalScrollBarVAO == 0) {
            #ifdef DEBUG_SCROLL_AREA
                cout << "\nShowing vertical scrollbar. Element width: " << elem->width;
            #endif
            // WARNING: side effects!
            getScrollbarDimensions(
                elem, top, right, bottom, left,
                thumbStart, thumbEnd,
                true
            );

            #ifdef DEBUG_SCROLL_AREA
                cout << "\n--> Vertical scrollbar dimensions. top: "
                    << top << ", left: " << left << ", bottom: "
                    << bottom << ", right:" << right
                    << ", thumbStart: " << thumbStart << ", thumbEnd: " << thumbEnd;
            #endif

            dimensionsInitialized = true;

            float vertices[] = {
                right, bottom,
                right, elem->top,
                left, bottom,
                left, elem->top,
            };

            glGenVertexArrays(1, &SCROLL_DATA.verticalScrollBarVAO);
            glBindVertexArray(SCROLL_DATA.verticalScrollBarVAO);

            if (SCROLL_DATA.verticalScrollBarVBO != 0) {
                glDeleteBuffers(1, &SCROLL_DATA.verticalScrollBarVBO);
            }

            glGenBuffers(1, &SCROLL_DATA.verticalScrollBarVBO);
            glBindBuffer(GL_ARRAY_BUFFER, SCROLL_DATA.verticalScrollBarVBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);
        }

        glBindVertexArray(SCROLL_DATA.verticalScrollBarVAO);
        glUseProgram(shaders[SHADER_DEFAULT].programId);

        uint32_t colorU = glGetUniformLocation(shaders[SHADER_DEFAULT].programId, "solidColor");

        glm::vec4 color = SCROLL_BAR_TRACK_COLOR;

        if (darkTheme) {
            color = SCROLL_BAR_TRACK_COLOR_DARK;
        }

        glUniform4f(colorU, color.r, color.g, color.b, color.a);

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        if (SCROLL_DATA.verticalScrollThumbVAO != 0 && update) {
            glDeleteVertexArrays(1, &SCROLL_DATA.verticalScrollThumbVAO);
            glDeleteBuffers(1, &SCROLL_DATA.verticalScrollThumbVBO);
            SCROLL_DATA.verticalScrollThumbVAO = 0;
            SCROLL_DATA.verticalScrollThumbVBO = 0;
        }

        if (SCROLL_DATA.verticalScrollThumbVAO == 0) {
            if (!dimensionsInitialized) {
                // WARNING: side effects!
                getScrollbarDimensions(
                    elem, top, right, bottom, left,
                    thumbStart, thumbEnd,
                    true
                );
            }

            float vertices[] = {
                right, thumbEnd,
                right, thumbStart,
                left, thumbEnd,
                left, thumbStart,
            };

            glGenVertexArrays(1, &SCROLL_DATA.verticalScrollThumbVAO);
            glBindVertexArray(SCROLL_DATA.verticalScrollThumbVAO);

            if (SCROLL_DATA.verticalScrollThumbVBO != 0) {
                glDeleteBuffers(1, &SCROLL_DATA.verticalScrollThumbVBO);
            }

            glGenBuffers(1, &SCROLL_DATA.verticalScrollThumbVBO);
            glBindBuffer(GL_ARRAY_BUFFER, SCROLL_DATA.verticalScrollThumbVBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);
            SCROLL_DATA.renderedUpdate = true;
        }

        glBindVertexArray(SCROLL_DATA.verticalScrollThumbVAO);
        glUseProgram(shaders[SHADER_DEFAULT].programId);

        colorU = glGetUniformLocation(shaders[SHADER_DEFAULT].programId, "solidColor");
        glUniform4f(colorU, 0.6f, 0.6f, 0.6f, 1.0f);

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    } else {
        if (SCROLL_DATA.verticalScrollBarVAO != 0) {
            glDeleteVertexArrays(1, &SCROLL_DATA.verticalScrollBarVAO);
            SCROLL_DATA.verticalScrollBarVAO = 0;

            #ifdef DEBUG_SCROLL_AREA
                cout << "\nCleared vertical scrollbar vertex array";
            #endif

            if (SCROLL_DATA.verticalScrollBarVBO != 0) {
                glDeleteBuffers(1, &SCROLL_DATA.verticalScrollBarVBO);
                SCROLL_DATA.verticalScrollBarVBO = 0;

                #ifdef DEBUG_SCROLL_AREA
                    cout << "\nCleared vertical scroll bar vertex buffers";
                #endif
            }
        }

        if (SCROLL_DATA.verticalScrollThumbVAO != 0) {
            glDeleteVertexArrays(1, &SCROLL_DATA.verticalScrollThumbVAO);
            SCROLL_DATA.verticalScrollThumbVAO = 0;

            #ifdef DEBUG_SCROLL_AREA
                cout << "\nCleared vertical scrollbar thumb vertex array";
            #endif

            if (SCROLL_DATA.verticalScrollThumbVBO != 0) {
                glDeleteBuffers(1, &SCROLL_DATA.verticalScrollThumbVBO);
                SCROLL_DATA.verticalScrollThumbVBO = 0;

                #ifdef DEBUG_SCROLL_AREA
                    cout << "\nCleared vertical scroll bar thumb vertex buffers";
                #endif
            }
        }
    }

    if (SCROLL_DATA.showHorizontalScrollbar) {
        // WARNING: side effects!
        float top, left, bottom, right, thumbStart, thumbEnd;
        bool dimensionsInitialized = false;

        if (SCROLL_DATA.horizontalScrollBarVAO != 0 && update) {
            glDeleteVertexArrays(1, &SCROLL_DATA.horizontalScrollBarVAO);
            SCROLL_DATA.horizontalScrollBarVAO = 0;

            #ifdef DEBUG_SCROLL_AREA
                cout << "\nCleared horizontal scrollbar vertex array";
            #endif

            if (SCROLL_DATA.horizontalScrollBarVBO != 0) {
                glDeleteBuffers(1, &SCROLL_DATA.horizontalScrollBarVAO);
                SCROLL_DATA.horizontalScrollBarVBO = 0;

                #ifdef DEBUG_SCROLL_AREA
                    cout << "\nCleared horizontal scrollbar vertex buffers";
                #endif
            }
        }

        if (SCROLL_DATA.horizontalScrollBarVAO == 0) {
            getScrollbarDimensions(
                elem, top, right, bottom, left,
                thumbStart, thumbEnd,
                false
            );

            dimensionsInitialized = true;

            float vertices[] = {
                right, bottom,
                right, top,
                left, bottom,
                left, top,
            };

            glGenVertexArrays(1, &SCROLL_DATA.horizontalScrollBarVAO);
            glBindVertexArray(SCROLL_DATA.horizontalScrollBarVAO);

            if (SCROLL_DATA.horizontalScrollBarVBO != 0) {
                glDeleteBuffers(1, &SCROLL_DATA.horizontalScrollBarVBO);
            }

            glGenBuffers(1, &SCROLL_DATA.horizontalScrollBarVBO);
            glBindBuffer(GL_ARRAY_BUFFER, SCROLL_DATA.horizontalScrollBarVBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);
        }

        glBindVertexArray(SCROLL_DATA.horizontalScrollBarVAO);
        glUseProgram(shaders[SHADER_DEFAULT].programId);

        uint32_t colorU = glGetUniformLocation(shaders[SHADER_DEFAULT].programId, "solidColor");

        glUniform4f(colorU, 0.9f, 0.9f, 0.9f, 1.0f);

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        if (SCROLL_DATA.horizontalScrollThumbVAO != 0 && update) {
            glDeleteVertexArrays(1, &SCROLL_DATA.horizontalScrollThumbVAO);
            glDeleteBuffers(1, &SCROLL_DATA.horizontalScrollThumbVBO);
            SCROLL_DATA.horizontalScrollThumbVAO = 0;
            SCROLL_DATA.horizontalScrollThumbVBO = 0;
        }

        if (SCROLL_DATA.horizontalScrollThumbVAO == 0) {
            if (!dimensionsInitialized) {
                // WARNING: side effects!
                getScrollbarDimensions(
                    elem, top, right, bottom, left,
                    thumbStart, thumbEnd,
                    false
                );
            }

            float vertices[] = {
                thumbStart, bottom,
                thumbStart, top,
                thumbEnd, bottom,
                thumbEnd, top,
            };

            glGenVertexArrays(1, &SCROLL_DATA.horizontalScrollThumbVAO);
            glBindVertexArray(SCROLL_DATA.horizontalScrollThumbVAO);

            if (SCROLL_DATA.horizontalScrollThumbVBO != 0) {
                glDeleteBuffers(1, &SCROLL_DATA.horizontalScrollThumbVBO);
            }

            glGenBuffers(1, &SCROLL_DATA.horizontalScrollThumbVBO);
            glBindBuffer(GL_ARRAY_BUFFER, SCROLL_DATA.horizontalScrollThumbVBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);
            SCROLL_DATA.renderedUpdate = true;
        }

        glBindVertexArray(SCROLL_DATA.horizontalScrollThumbVAO);
        glUseProgram(shaders[SHADER_DEFAULT].programId);

        colorU = glGetUniformLocation(shaders[SHADER_DEFAULT].programId, "solidColor");
        glUniform4f(colorU, 0.6f, 0.6f, 0.6f, 1.0f);

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    } else {
        if (SCROLL_DATA.horizontalScrollBarVAO != 0) {
            glDeleteVertexArrays(1, &SCROLL_DATA.horizontalScrollThumbVAO);
            SCROLL_DATA.horizontalScrollBarVAO = 0;

            #ifdef DEBUG_SCROLL_AREA
                cout << "\nCleared horizontal scrollbar vertex array";
            #endif

            if (SCROLL_DATA.horizontalScrollBarVBO != 0) {
                glDeleteBuffers(1, &SCROLL_DATA.horizontalScrollThumbVBO);
                SCROLL_DATA.horizontalScrollBarVBO = 0;

                #ifdef DEBUG_SCROLL_AREA
                    cout << "\nCleared horizontal scroll bar vertex buffers";
                #endif
            }
        }

        if (SCROLL_DATA.horizontalScrollThumbVAO != 0) {
            glDeleteVertexArrays(1, &SCROLL_DATA.horizontalScrollThumbVAO);
            SCROLL_DATA.horizontalScrollThumbVAO = 0;

            #ifdef DEBUG_SCROLL_AREA
                cout << "\nCleared horizontal scrollbar thumb vertex array";
            #endif

            if (SCROLL_DATA.horizontalScrollThumbVBO != 0) {
                glDeleteBuffers(1, &SCROLL_DATA.horizontalScrollThumbVBO);
                SCROLL_DATA.horizontalScrollThumbVBO = 0;

                #ifdef DEBUG_SCROLL_AREA
                    cout << "\nCleared horizontal scroll bar thumb vertex buffers";
                #endif
            }
        }
    }

    #undef SCROLL_DATA
}

void renderElement(UIElement& elem, glm::vec2 &resolution, glm::vec2 &cursor) {
    // cout << "resolution: " << resolution.x << "x" << resolution.y;
    switch (elem.type) {
        case UI_CONTAINER: {
            if (elem.reDraw) {
                // recalculateLayout(&elem);

                #ifdef DEBUG_RENDER
                    cout << "\nRe-drawing Container";
                #endif

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

        case UI_CONTENT_CONTAINER: {
            if (elem.contentBlocks != nullptr) {
                if (elem.reDraw) {
                    elem.extraData->scrollData.showVerticalScrollbar = elem.extraData->scrollData.height > elem.height;
                    elem.extraData->scrollData.showHorizontalScrollbar = elem.extraData->scrollData.width > elem.width;

                    #ifdef DEBUG_SCROLL_AREA
                        cout << "\nShow vertical scrollbar: " << elem.extraData->scrollData.showVerticalScrollbar;
                        cout << "\nShow horizontal scrollbar: " << elem.extraData->scrollData.showHorizontalScrollbar;
                    #endif
                }

                elem.reDraw = false;

                float width = elem.width;
                float height = elem.height;

                if (elem.extraData->scrollData.showVerticalScrollbar) {
                    width -= SCROLL_BAR_THICKNESS;
                }

                if (elem.extraData->scrollData.showHorizontalScrollbar) {
                    height -= SCROLL_BAR_THICKNESS;
                }

                glScissor(elem.left, resolution.y - elem.top - elem.height, width, height);
                glEnable(GL_SCISSOR_TEST);

                renderContentBlockHighlight();

                ContentBlock* temp = elem.contentBlocks;

                while (temp != nullptr) {
                    renderContentBlock(&elem, temp, resolution, cursor);
                    temp = temp->next;
                }

                renderCBMoveMarker();

                glDisable(GL_SCISSOR_TEST);

                renderScrollbars(&elem);
            }
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

                if (elem.parent->type == UI_SCROLL_AREA) {
                    if (elem.parent->extraData->scrollData.height < dim.z) {
                        #ifdef DEBUG_SCROLL_AREA
                            cout << "\nScroll height less than dim.z " << dim.z;
                        #endif

                        elem.parent->extraData->scrollData.height = dim.z;
                        elem.parent->extraData->scrollData.renderedUpdate = false;
                        elem.parent->reDraw = true;
                    }
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

                #ifdef DEBUG_RENDER
                    cout << "\nRe-drawing";
                #endif

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

            if (elem.totalChildren) {
                if (elem.reDraw) {
                    elem.extraData->scrollData.showVerticalScrollbar = elem.extraData->scrollData.height > elem.height;
                    elem.extraData->scrollData.showHorizontalScrollbar = elem.extraData->scrollData.width > elem.width;

                    #ifdef DEBUG_SCROLL_AREA
                        cout << "\nShow vertical scrollbar: " << elem.extraData->scrollData.showVerticalScrollbar;
                        cout << "\nShow horizontal scrollbar: " << elem.extraData->scrollData.showHorizontalScrollbar;
                    #endif
                }

                elem.reDraw = false;

                float width = elem.width;
                float height = elem.height;

                if (elem.extraData->scrollData.showVerticalScrollbar) {
                    width -= SCROLL_BAR_THICKNESS;
                }

                if (elem.extraData->scrollData.showHorizontalScrollbar) {
                    height -= SCROLL_BAR_THICKNESS;
                }

                glScissor(elem.left, resolution.y - elem.top - elem.height, width, height);
                glEnable(GL_SCISSOR_TEST);

                for (uint32_t i=0; i<elem.totalChildren; ++i) {
                    renderElement(*elem.children[i], resolution, cursor);
                }

                // highlightedContentBlock.show = false;

                // renderContentBlockHighlight();

                // ContentBlock* temp = elem.contentBlocks;

                // while (temp != nullptr) {
                //     renderContentBlock(&elem, temp, resolution, cursor);
                //     temp = temp->next;
                // }

                // renderCBMoveMarker();

                // if (!highlightedContentBlock.show && highlightedContentBlock.backgroundVAO) {
                    // glDeleteVertexArrays(1, &highlightedContentBlock.backgroundVAO);
                    // glDeleteBuffers(1, &highlightedContentBlock.backgroundVBO);

                    // glDeleteVertexArrays(1, &highlightedContentBlock.dragTabVAO);
                    // glDeleteBuffers(1, &highlightedContentBlock.dragTabVBO);

                    // highlightedContentBlock.backgroundVAO = 0;
                    // highlightedContentBlock.backgroundVBO = 0;
                    // highlightedContentBlock.dragTabVAO = 0;
                    // highlightedContentBlock.dragTabVBO = 0;
                // }

                glDisable(GL_SCISSOR_TEST);
                renderScrollbars(&elem);
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
                    #ifdef DEBUG_RENDER
                        cout << "\nRedrawing icon button";
                    #endif

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

            if (elem.icon != Icon::None) {
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
                        #ifdef DEBUG_TEXTURE
                            cout << "\nTexture " << location << " loaded.";
                        #endif

                        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
                        glGenerateMipmap(GL_TEXTURE_2D);
                    } else {
                        cerr << "\nError: Failed to load texture (" << location << ").";
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
                #ifdef DEBUG_RENDER
                    cout << "\nRe-drawing Text Input";
                #endif

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
                #ifdef DEBUG_RENDER
                    cout << "\nRe-drawing Text Area" << "\nwidth: " << elem.width << ", height: " << elem.height;
                #endif

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

void renderContentBlock(
    UIElement* parentElement,
    ContentBlock* cb,
    glm::vec2 &resolution,
    glm::vec2 &cursor
) {
    // NOTE: Assuming parent element will always be of type UI_SCROLL_AREA.
 
    if (cb == nullptr || !(cb->flags & CONTENT_BLOCK_VISIBLE)) {
        cout << "\nContent block not visible";
        return;
    }

    uint8_t fontSize = 16;
    float topMargin = 0.0f;

    switch (cb->type) {
        case ContentBlockType::Heading1: fontSize = 48; topMargin = CONTENT_BLOCK_MARGIN; break;
        case ContentBlockType::Heading2: fontSize = 40; topMargin = CONTENT_BLOCK_MARGIN; break;
        case ContentBlockType::Heading3: fontSize = 32; topMargin = CONTENT_BLOCK_MARGIN; break;
        case ContentBlockType::Heading4: fontSize = 24; topMargin = CONTENT_BLOCK_MARGIN; break;
        case ContentBlockType::Heading5: fontSize = 16; topMargin = CONTENT_BLOCK_MARGIN; break;
        case ContentBlockType::Heading6: fontSize = 12; topMargin = CONTENT_BLOCK_MARGIN; break;
        case ContentBlockType::Paragraph: fontSize = 16; break;
        case ContentBlockType::Title:  fontSize = 48; topMargin = 5.0f; break;
    }

    if (parentElement->extraData == nullptr) {
        cout << "\nRender Error: UIElement's parent is NULL!";
        return;
    }

    glm::vec4 dim;

    if ((cb->flags & CONTENT_BLOCK_REDRAW) != 0) {
        if (cb->prev != nullptr) {
            cb->top = cb->prev->top + cb->prev->height + 8.0f;
            cb->left = cb->prev->left;

            switch (cb->prev->type) {
                case ContentBlockType::Heading1:
                case ContentBlockType::Heading2:
                case ContentBlockType::Heading3:
                case ContentBlockType::Heading4:
                case ContentBlockType::Heading5:
                case ContentBlockType::Heading6:
                    topMargin += CONTENT_BLOCK_MARGIN;
                    break;
                case ContentBlockType::Title:
                    topMargin += 4.0f;
                default:
                    break;
            }

            cb->top += topMargin;
        } else {
            cb->top = parentElement->top;
            cb->left = parentElement->left;
        }
    }

    float top = cb->top - parentElement->extraData->scrollData.yOffset;
    float left = cb->left - parentElement->extraData->scrollData.xOffset;

    if (resolution.x >= CONTENT_BREAKPOINT) {
        left += contentLeft;
    } else {
        left += CONTENT_HORIZONTAL_MARGIN;
    }

    dim = renderContentRope(
        cb->rope,
        glm::vec2(left, top),
        resolution,
        darkTheme ? WHITE : BLACK,
        fontSize,
        parentElement
    );

    if ((cb->flags & CONTENT_BLOCK_REDRAW) != 0) {
        cb->height = dim[2];
        cb->flags &= ~(CONTENT_BLOCK_REDRAW);
    }

    if (parentElement->extraData->scrollData.height < cb->top + cb->height) {
        #ifdef DEBUG_SCROLL_AREA
            cout << "\nScroll height less than dim.z " << dim.z;
        #endif

        parentElement->extraData->scrollData.height = cb->top + cb->height;
        parentElement->extraData->scrollData.renderedUpdate = false;
        parentElement->reDraw = true;
    }
}

void renderContentBlockHighlight() {
    if (!highlightedContentBlock.show) {
        return;
    }

    uint32_t colorU;

    if (highlightedContentBlock.backgroundVAO == 0) {
        glGenVertexArrays(1, &highlightedContentBlock.backgroundVAO);
        glBindVertexArray(highlightedContentBlock.backgroundVAO);

        if (highlightedContentBlock.backgroundVBO) {
            glDeleteBuffers(1, &highlightedContentBlock.backgroundVBO);
            highlightedContentBlock.backgroundVBO = 0;
        }

        float vertices[] = {
            highlightedContentBlock.right, highlightedContentBlock.top,
            highlightedContentBlock.right, highlightedContentBlock.bottom,
            highlightedContentBlock.left, highlightedContentBlock.top,
            highlightedContentBlock.left, highlightedContentBlock.bottom,
        };

        glGenBuffers(1, &highlightedContentBlock.backgroundVBO);
        glBindBuffer(GL_ARRAY_BUFFER, highlightedContentBlock.backgroundVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
    } else if (highlightedContentBlock.updated) {
        float vertices[] = {
            highlightedContentBlock.right, highlightedContentBlock.top,
            highlightedContentBlock.right, highlightedContentBlock.bottom,
            highlightedContentBlock.left, highlightedContentBlock.top,
            highlightedContentBlock.left, highlightedContentBlock.bottom,
        };

        glBindVertexArray(highlightedContentBlock.backgroundVAO);
        glBindBuffer(GL_ARRAY_BUFFER, highlightedContentBlock.backgroundVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    }

    glBindVertexArray(highlightedContentBlock.backgroundVAO);
    glUseProgram(shaders[SHADER_DEFAULT].programId);

    colorU = glGetUniformLocation(shaders[SHADER_DEFAULT].programId, "solidColor");

    glm::vec4 color = CONTENT_HIGHLIGHT_BG;

    if (darkTheme) {
        color = CONTENT_HIGHLIGHT_BG_DARK;
    }

    glUniform4f(colorU, color.r, color.g, color.b, color.a);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    if (highlightedContentBlock.dragTabVAO == 0) {
        glGenVertexArrays(1, &highlightedContentBlock.dragTabVAO);
        glBindVertexArray(highlightedContentBlock.dragTabVAO);

        if (highlightedContentBlock.dragTabVBO) {
            glDeleteBuffers(1, &highlightedContentBlock.dragTabVBO);
            highlightedContentBlock.dragTabVBO = 0;
        }

        float vertices[] = {
            highlightedContentBlock.left, highlightedContentBlock.top,
            highlightedContentBlock.left, highlightedContentBlock.bottom,
            highlightedContentBlock.left - 20.0f, highlightedContentBlock.top,
            highlightedContentBlock.left - 20.0f, highlightedContentBlock.bottom,
        };

        glGenBuffers(1, &highlightedContentBlock.dragTabVBO);
        glBindBuffer(GL_ARRAY_BUFFER, highlightedContentBlock.dragTabVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
    } else if (highlightedContentBlock.updated) {
        float vertices[] = {
            highlightedContentBlock.left, highlightedContentBlock.top,
            highlightedContentBlock.left, highlightedContentBlock.bottom,
            highlightedContentBlock.left - 20.0f, highlightedContentBlock.top,
            highlightedContentBlock.left - 20.0f, highlightedContentBlock.bottom,
        };

        glBindVertexArray(highlightedContentBlock.dragTabVAO);
        glBindBuffer(GL_ARRAY_BUFFER, highlightedContentBlock.dragTabVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    }

    glBindVertexArray(highlightedContentBlock.dragTabVAO);
    glUseProgram(shaders[SHADER_DEFAULT].programId);

    colorU = glGetUniformLocation(shaders[SHADER_DEFAULT].programId, "solidColor");

    color = CONTENT_HIGHLIGHT_DRAG;

    if (darkTheme) {
        color = CONTENT_HIGHLIGHT_DRAG_DARK;
    }

    glUniform4f(colorU, color.r, color.g, color.b, color.a);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    highlightedContentBlock.updated = false;
}

void renderCBMoveMarker() {
    #define GET_TOP(block) block->top - 1.5f - highlightedContentBlock.parentElement->extraData->scrollData.yOffset

    if (
        highlightedContentBlock.mouseDown
        && highlightedContentBlock.parentElement != nullptr
        && cbMove.show
    ) {
        if (cbMove.updated) {
            if (cbMove.colorU == 0) {
                cbMove.colorU = glGetUniformLocation(shaders[SHADER_DEFAULT].programId, "solidColor");
            }

            float top;

            if (cbMove.targetBlock == nullptr) {
                ContentBlock* last = highlightedContentBlock.parentElement->contentBlocks;

                LAST_IN_LIST(last);

                top = GET_TOP(last) + last->height;
            } else {
                top = GET_TOP(cbMove.targetBlock);
            }

            float bottom = top + 3.0f;

            float vertices[] = {
                contentRight, top,
                contentRight, bottom,
                contentLeft, top,
                contentLeft, bottom,
            };

            if (cbMove.vao) {
                glBindVertexArray(cbMove.vao);
                glBindBuffer(GL_ARRAY_BUFFER, cbMove.vbo);
                glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

                glBindVertexArray(0);
                glBindBuffer(GL_ARRAY_BUFFER, 0);
            } else {
                glGenVertexArrays(1, &cbMove.vao);
                glBindVertexArray(cbMove.vao);

                if (cbMove.vbo) {
                    glDeleteBuffers(1, &cbMove.vbo);
                    cbMove.vbo = 0;
                }

                glGenBuffers(1, &cbMove.vbo);
                glBindBuffer(GL_ARRAY_BUFFER, cbMove.vbo);
                glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
                glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
                glEnableVertexAttribArray(0);
            }

            cbMove.updated = false;
        }

        glBindVertexArray(cbMove.vao);
        glUseProgram(shaders[SHADER_DEFAULT].programId);
        glUniform4f(cbMove.colorU, 1.0f, 0.0f, 0.0f, 1.0f);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }

    #undef GET_TOP
}

void hideCBMoveMarker() {
    cbMove.show = false;
    glDeleteVertexArrays(1, &cbMove.vao);
    glDeleteBuffers(1, &cbMove.vbo);

    cbMove.vao = 0;
    cbMove.vbo = 0;
    cbMove.targetBlock = nullptr;
}

void reDrawAllContentBlocks(UIElement* parentElement) {
    if (parentElement->contentBlocks == nullptr) {
        return;
    }

    ContentBlock* temp = parentElement->contentBlocks;

    while (temp != nullptr) {
        temp->flags |= CONTENT_BLOCK_REDRAW;
        temp = temp->next;
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
            || elem->type == UI_CONTENT_CONTAINER
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
        case Icon::None: return "";
        case Icon::Plus: path += "plus.png"; break;
        case Icon::File: path += "file.png"; break;
    }

    return path;
}

bool isPointInside(float pointX, float pointY, float left, float right, float top, float bottom) {
    return pointX > left && pointX < right && pointY > top && pointY < bottom;
}

void getScrollbarDimensions(
    UIElement* elem,
    float& top, float& right, float& bottom, float& left,
    float& thumbStart, float& thumbEnd,
    bool vertical
) {
    if (vertical) {
        float height = elem->height;

        if (elem->extraData->scrollData.showHorizontalScrollbar) {
            height -= SCROLL_BAR_THICKNESS;
        }

        top = elem->top;
        right = elem->left + elem->width;
        left = right - SCROLL_BAR_THICKNESS;
        bottom = top + height;

        thumbStart = top + elem->extraData->scrollData.yOffset / elem->extraData->scrollData.height * height;
        thumbEnd = thumbStart + elem->height / elem->extraData->scrollData.height * height;

        if (thumbEnd > top + height) {
            float barWidth = thumbEnd - thumbStart;

            thumbEnd = top + height;
            thumbStart = thumbEnd - barWidth;
        }

        return;
    }

    float width = elem->width;

    if (elem->extraData->scrollData.showVerticalScrollbar) {
        width -= SCROLL_BAR_THICKNESS;
    }

    bottom = elem->top + elem->height;
    top = bottom - SCROLL_BAR_THICKNESS;
    right = elem->left + width;
    left = elem->left;

    thumbStart = left + elem->extraData->scrollData.xOffset / elem->extraData->scrollData.width * width;
    thumbEnd = thumbStart + elem->width / elem->extraData->scrollData.width * width;

    if (thumbEnd > elem->left + width) {
        float barWidth = thumbEnd - thumbStart;

        thumbEnd = left + width;
        thumbStart = thumbEnd - barWidth;
    }
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

#ifdef DEBUG_CONTENT_BLOCKS

void printContentBlocks(UIElement* parentElement) {
    cout << "\nContent Blocks: ";

    if (parentElement->contentBlocks == nullptr) {
        return;
    }

    ContentBlock* temp = parentElement->contentBlocks;

    while (temp != nullptr) {
        cout << "\n  ";
        switch (temp->type) {
            case ContentBlockType::Title: cout << "Title"; break;
            case ContentBlockType::Heading1: cout << "Heading1"; break;
            case ContentBlockType::Heading2: cout << "Heading2"; break;
            case ContentBlockType::Heading3: cout << "Heading3"; break;
            case ContentBlockType::Heading4: cout << "Heading4"; break;
            case ContentBlockType::Heading5: cout << "Heading5"; break;
            case ContentBlockType::Heading6: cout << "Heading6"; break;
            case ContentBlockType::Paragraph: cout << "Paragraph"; break;
        }

        cout << " -> " << temp->rope->text.substr(0, 10);
        temp = temp->next;
    }
}

#endif

