#include <iostream>
#include <stack>
#include <thread>

#ifdef DEBUG_TEXT_INPUT
    #include <string_view>
#endif

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "include/ui.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "include/command.hpp"
#include "include/shader.hpp"
#include "include/text.hpp"
#include "files.hpp"
#include "../lib/stb_image.h"

#define TIME() static_cast<float>(glfwGetTime())
#define LAST_IN_LIST(item) while (item->next != nullptr) { item = item->next; }

#define SETUP_BUFFERS(vao, vbo, vertices) do {\
        glGenVertexArrays(1, &vao);\
        glBindVertexArray(vao);\
        glGenBuffers(1, &vbo);\
        glBindBuffer(GL_ARRAY_BUFFER, vbo);\
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);\
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);\
        glEnableVertexAttribArray(0);\
    } while (false)

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
    .cbIndex = 0,
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

ContentBlockMoveMarker cbMove = { 0, 0, 0, 0, false, true };

vector<ContentTab> tabs;
uint8_t activeTabIndex;
UIElement* contentContainer;
UIElement* activeElement = nullptr;
uint16_t windowHeight;
uint16_t windowWidth;
glm::vec2 resolution;
GLFWwindow* window;
UIElement rootContainer;
bool minimized = false;
bool focused = true;
bool mouse1Down = false;
bool cursorShouldUpdate = false;
bool showFps = false;
bool cbTextDragging = false;

GLFWcursor* textCursor;
GLFWcursor* pointerCursor;

UIElement elements[16];

float mouseX = 25000;
float mouseY = 25000;
float mouseDownX = 25000;
float mouseDownY = 25000;
float cursorUpdateTime = 0.0;

void showWindow() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "patram", NULL, NULL);

    if (window == NULL) {
        cout << "\nFailed to create GLFW window";
        glfwTerminate();
        exit(-1);
    }

    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        cerr << "\nFailed to initialize GLAD";
    }

    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

    glfwSetKeyCallback(window, keyCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetFramebufferSizeCallback(window, resizeCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetScrollCallback(window, scrollCallback);
    glfwSetWindowIconifyCallback(window, minimizedCallback);
    glfwSetWindowMaximizeCallback(window, maximizedCallback);
    glfwSetWindowFocusCallback(window, windowFocusCallback);

    glfwSetInputMode(window, GLFW_LOCK_KEY_MODS, GLFW_TRUE);

    glClearColor(0, 0, 0, 1.0);

}

void updateCursorShape(CursorShape shape) {
    if (!cursorShouldUpdate) {
        return;
    }

    switch (shape) {
        case CursorShape::Auto: {
            UIElement* elem = getElementFromScreenCoords(&rootContainer, mouseX, mouseY);

            if (elem != nullptr && (elem->type == UI_TEXT_AREA || elem->type == UI_TEXT_INPUT)) {
                glfwSetCursor(window, textCursor);
            } else {
                glfwSetCursor(window, nullptr);
            }

            break;
        }

        case CursorShape::Default: glfwSetCursor(window, nullptr); break;
        case CursorShape::Text: glfwSetCursor(window, textCursor); break;
        case CursorShape::Pointer: glfwSetCursor(window, pointerCursor); break;
    }

    while (glfwGetTime() < cursorUpdateTime) {}

    cursorUpdateTime = glfwGetTime() + THROTTLE_FAST;
    cursorShouldUpdate = false;
}

void throttleUpdateCursorShape(CursorShape shape) {
    if (!cursorShouldUpdate) {
        cursorShouldUpdate = true;
        thread t(updateCursorShape, shape);
        t.detach();
    }
}

void inputClickCallback(UIElement* elem) {
    if (activeElement != nullptr) {
        #ifdef DEBUG_TEXT_SELECTION
            cout << "\nSetting text input as active!";
        #endif
        activeElement->active = false;
    }

    elem->active = true;
    activeElement = elem;

    if (elem->extraData != nullptr) {
        elem->extraData->textInputData.lastActiveTime = glfwGetTime();
        // selectTextWithMouse(elem, mouseX, mouseY, mouseDownX, mouseDownY, true, resolution);
    }
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_1) {
        if (action == GLFW_PRESS) {
            #ifdef DEBUG_MOUSE_BUTTON
                cout << "\nMouse button 1 pressed";
            #endif
            mouse1Down = true;
            mouseDownX = mouseX;
            mouseDownY = mouseY;

            if (highlightedContentBlock.show) {
                if (
                    mouseX >= highlightedContentBlock.left - 20.0f
                    && mouseX <= highlightedContentBlock.left
                    && mouseY >= highlightedContentBlock.top
                    && mouseY <= highlightedContentBlock.bottom
                    && highlightedContentBlock.cbIndex < tabs[activeTabIndex].contentBlocks.size()
                ) {
                    tabs[activeTabIndex].contentBlocks[highlightedContentBlock.cbIndex].flags |= CONTENT_BLOCK_DRAGGING;
                    highlightedContentBlock.mouseDown = true;
                    return;
                } else {
                    tabs[activeTabIndex].contentBlocks[highlightedContentBlock.cbIndex].flags &= ~(CONTENT_BLOCK_DRAGGING);
                    highlightedContentBlock.mouseDown = false;
                    cbMove.targetIndex = 0;
                }
            }

            UIElement* elem = getElementFromScreenCoords(&rootContainer, mouseX, mouseY);

            if (elem != nullptr) {
                #ifdef DEBUG_MOUSE_BUTTON
                    cout << " for " << elementType(elem);
                #endif

                if (activeElement != nullptr) {
                    activeElement->active = false;
                }

                activeElement = elem;
                elem->active = true;

                switch (activeElement->type) {
                    case UI_CONTENT_CONTAINER:
                        contentBlockMouseDownCallback(elem, mouseX, mouseY, resolution);
                        break;
                    case UI_SCROLL_AREA:
                        scrollAreaMouseDownCallback(elem, mouseX, mouseY, resolution);
                        break;
                    case UI_TEXT_AREA:
                    case UI_TEXT_INPUT:
                        textInputMouseDownCallback(elem, mouseX, mouseY, resolution);
                        break;
                }

                // elem->onMouseDown(elem, mouseX, mouseY, resolution);
            } else if (activeElement != nullptr) {
                activeElement->active = false;
                activeElement = nullptr;
            }
        } else if (action == GLFW_RELEASE) {
            #ifdef DEBUG_MOUSE_BUTTON
                cout << "\nMouse button 1 released";
            #endif
            mouse1Down = false;

            if (highlightedContentBlock.show && highlightedContentBlock.mouseDown) {
                cout << "\nHighlighted content block RELEASE target: ";

                if (cbMove.targetIndex != 0) {
                    cout << tabs[activeTabIndex].contentBlocks[cbMove.targetIndex].rope->text.substr(0, 10);
                } else {
                    cout << "NULL";
                }

                moveContentBlock(
                    activeTabIndex,
                    highlightedContentBlock.cbIndex,
                    cbMove.targetIndex
                );

                tabs[activeTabIndex].contentBlocks[highlightedContentBlock.cbIndex].flags &= ~(CONTENT_BLOCK_DRAGGING);
                reDrawAllContentBlocks();

                highlightedContentBlock.mouseDown = false;
                hideCBMoveMarker();
                return;
            }

            UIElement* elem = getElementFromScreenCoords(&rootContainer, mouseX, mouseY);

            if (elem != nullptr) {
                switch (activeElement->type) {
                    case UI_TEXT_AREA:
                    case UI_TEXT_INPUT:
                        textInputMouseUpCallback(elem, mouseX, mouseY, resolution);
                        break;
                    case UI_CONTENT_CONTAINER:
                        contentBlockMouseUpCallback(elem, mouseX, mouseY, resolution);
                        break;
                }

                // if (mouseDownX == mouseX && mouseDownY == mouseY) {
                //     elem->onClick(elem);
                // }
            }
        }
    }
}

void keyCallback(GLFWwindow* window, int key, int scanCode, int action, int mods) {
    #ifdef DEBUG_TEXT_INPUT
        cout << "\nKeyboard event. Key: " << key << ", Scan code: " << scanCode
            << ", Action: " << action << ", Modifier Keys: " << mods;
    #endif

    if (activeElement == nullptr) {
        return;
    }

    if (action == GLFW_PRESS && activeElement->type != UI_CONTENT_CONTAINER) {
        switch (key) {
            case GLFW_KEY_ESCAPE: {
                if (activeElement != nullptr && activeElement->type != UI_TEXT_INPUT) {
                    activeElement->active = false;
                    activeElement = nullptr;
                }

                break;
            }

            default: {
                // Nothing!
            }
        }
    }

    switch (activeElement->type) {
        case UI_CONTENT_CONTAINER: cbKeyCallback(key, action, mods); break;
        case UI_TEXT_INPUT:
        case UI_TEXT_AREA: textInputKeyCallback(activeElement, key, action, mods); break;
    }

    if (!activeElement->active) {
        activeElement = nullptr;
    }
}

void resizeCallback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);

    if (width >= CONTENT_BREAKPOINT) {
        contentLeft = (static_cast<float>(width) - CONTENT_MAX_WIDTH) / 2;
        contentRight = contentLeft + CONTENT_MAX_WIDTH;
    } else {
        contentLeft = CONTENT_HORIZONTAL_MARGIN;
        contentRight = width - CONTENT_HORIZONTAL_MARGIN;
    }

    rootContainer.width = width;
    rootContainer.height = height;

    windowWidth = width;
    windowHeight = height;

    if (!minimized) {
        #ifdef DEBUG_LAYOUT
            cout << "\nRecalculating layout";
        #endif

        recalculateLayout(&rootContainer);
    }
    highlightedContentBlock.show = false;
    highlightedContentBlock.updated = true;

    resolution = glm::vec2(width, height);
    glm::vec2 cursor = glm::vec2(mouseX, mouseY);

    ContentTab* tab = CURRENT_TAB;

    if (tab->showCursor) {
        tab->redrawCursor = true;
    }

    tab->scrollbarUpdated = false;

    renderElement(rootContainer, resolution, cursor);
    glfwSwapBuffers(window);
}

void cursorPosCallback(GLFWwindow * window, double xpos, double ypos) {
    mouseX = xpos;
    mouseY = ypos;

    ContentTab* tab = &tabs[activeTabIndex];

    CursorShape cShape = CursorShape::Default;

    if (!highlightedContentBlock.mouseDown) {
        float top;
        highlightedContentBlock.show = false;

        for (
            auto itr = tab->contentBlocks.begin();
            itr != tab->contentBlocks.end();
            ++itr
        ) {
            top = (*itr).top - tab->scrollOffsetY;

            if (ypos > top && ypos < top + (*itr).height) {
                cShape = CursorShape::Text;
                if ((*itr).type == ContentBlockType::Title) {
                    highlightedContentBlock.show = false;
                    break;
                }

                // TODO: Make sure the cursor is not over a scrollbar.
                highlightedContentBlock.show = true;
                highlightedContentBlock.left = contentLeft;
                highlightedContentBlock.top = top;

                tab->activeCBIndex = std::distance(tab->contentBlocks.begin(), itr);

                if (glyphDescender < 0) {
                    highlightedContentBlock.bottom = top + (*itr).height - glyphDescender;
                } else {
                    highlightedContentBlock.bottom = top + (*itr).height;
                }

                highlightedContentBlock.right = contentRight;

                uint32_t index = std::distance(tabs[activeTabIndex].contentBlocks.begin(), itr);

                if (highlightedContentBlock.cbIndex != index) {
                    highlightedContentBlock.updated = true;
                }

                highlightedContentBlock.cbIndex = index;
                break;
            }

            // TODO: remove this when 1 is completed
            if (top > windowHeight) {
                break;
            }
        }

        if (tab->cbTextMouseDown) {
            SelectionData sd = getCBTextCursorFromScreenCoords(mouseX, mouseY, resolution);

            if (sd.cursorCBIndex != tab->cursorCBIndex) {
                cbTextDragging = true;

                if (tab->textSelected) {
                    if (sd.cursorCBIndex < tab->initialCBIndex) {
                        tab->selStartCBIndex = sd.cursorCBIndex;
                        tab->selStartCursorPosition = sd.cursorPosition;
                        tab->selEndCBIndex = tab->initialCBIndex;
                    } else {
                        tab->selStartCBIndex = tab->initialCBIndex;
                        tab->selEndCBIndex = sd.cursorCBIndex;
                        tab->selEndCursorPosition = sd.cursorPosition;
                    }
                } else {
                    tab->textSelected = true;

                    if (sd.cursorCBIndex < tab->cursorCBIndex) {
                        tab->selStartCBIndex = sd.cursorCBIndex;
                        tab->selStartCursorPosition = sd.cursorPosition;
                        tab->selEndCBIndex = tab->cursorCBIndex;
                        tab->selEndCursorPosition = tab->cursorPosition;
                    } else {
                        tab->selStartCBIndex = tab->cursorCBIndex;
                        tab->selStartCursorPosition = tab->cursorPosition;
                        tab->selEndCBIndex = sd.cursorCBIndex;
                        tab->selEndCursorPosition = sd.cursorPosition;
                    }
                }

                tab->cursorCBIndex = sd.cursorCBIndex;
                tab->cursorPosition = sd.cursorPosition;
            } else if (sd.cursorPosition != tab->cursorPosition) {
                cbTextDragging = true;

                if (tab->textSelected) {
                    if (tab->selStartCBIndex == tab->selEndCBIndex) {
                        if (sd.cursorPosition == tab->initialCursorPosition) {
                            tab->textSelected = false;
                        } else if (sd.cursorPosition > tab->initialCursorPosition) {
                            tab->selEndCursorPosition = sd.cursorPosition;
                            tab->selStartCursorPosition = tab->initialCursorPosition;
                        } else {
                            tab->selStartCursorPosition = sd.cursorPosition;
                            tab->selEndCursorPosition = tab->initialCursorPosition;
                        }
                    } else {
                        if (tab->selStartCBIndex == sd.cursorCBIndex) {
                            tab->selStartCursorPosition = sd.cursorPosition;
                        } else if (tab->selEndCBIndex == sd.cursorCBIndex) {
                            tab->selEndCursorPosition = sd.cursorPosition;
                        } else {
                            // do nothing
                        }
                    }
                } else {
                    tab->textSelected = true;
                    tab->initialCursorPosition = tab->cursorPosition;
                    tab->initialCBIndex = tab->cursorCBIndex;

                    if (sd.cursorPosition > tab->cursorPosition) {
                        tab->selStartCBIndex = tab->cursorCBIndex;
                        tab->selEndCBIndex = sd.cursorCBIndex;
                        tab->selStartCursorPosition = tab->cursorPosition;
                        tab->selEndCursorPosition = sd.cursorPosition;
                    } else {
                        tab->selStartCBIndex = sd.cursorCBIndex;
                        tab->selEndCBIndex = tab->cursorCBIndex;
                        tab->selStartCursorPosition = sd.cursorPosition;
                        tab->selEndCursorPosition = tab->cursorPosition;
                    }
                }

                tab->cursorCBIndex = sd.cursorCBIndex;
                tab->cursorPosition = sd.cursorPosition;
                tab->updateCursorX = true;
                tab->redrawSel = true;
                updateCBCursor(tab, sd);
            }
        }
    }

    if (highlightedContentBlock.show && highlightedContentBlock.mouseDown) {
        #define SCROLL_DATA contentContainer->extraData->scrollData

        for (
            auto itr = tab->contentBlocks.begin();
            itr != tab->contentBlocks.end();
            ++itr
        ) {
            float top = (*itr).top - SCROLL_DATA.yOffset;

            uint32_t index = std::distance(tab->contentBlocks.begin(), itr);

            if (
                highlightedContentBlock.cbIndex != index
                && mouseY > top
                && mouseY < top + (*itr).height
            ) {
                if (
                    index == cbMove.targetIndex
                    && mouseY > top + (*itr).height/2
                ) {
                    cbMove.targetIndex = index + 1;
                } else {
                    if (index != cbMove.targetIndex && mouseY < top + (*itr).height/2) {
                        cbMove.targetIndex = index;
                    }
                }

                cbMove.show = true;
                cbMove.updated = true;
                break;
            }
        }

        if (mouseY > resolution.y - 50.0f) {
            float newScrollOffset = (50.0f - resolution.y + mouseY)/10 + SCROLL_DATA.yOffset;

            if (newScrollOffset > SCROLL_DATA.height - resolution.y) {
                SCROLL_DATA.yOffset = SCROLL_DATA.height - resolution.y + 50.0f;
            } else {
                SCROLL_DATA.yOffset = newScrollOffset;
            }
        } else if (mouseY < 114.0f) {
            float newScrollOffset = SCROLL_DATA.yOffset - (114.0f - mouseY)/10;

            if (newScrollOffset < 0) {
                SCROLL_DATA.yOffset = 0;
            } else {
                SCROLL_DATA.yOffset = newScrollOffset;
            }
        }

        #undef SCROLL_DATA
    }

    if (
        activeElement != nullptr
        && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1) == GLFW_PRESS
    ) {
        switch (activeElement->type) {
            case UI_SCROLL_AREA:
                cShape = CursorShape::Text;
                scrollAreaMouseDragCallback(activeElement, mouseX, mouseY, resolution);
                break;
            case UI_TEXT_AREA:
            case UI_TEXT_INPUT:
                cShape = CursorShape::Text;
                textInputMouseDragCallback(activeElement, mouseX, mouseY, resolution);
                break;
        }
    }

    throttleUpdateCursorShape(cShape);
}

void scrollCallback(GLFWwindow * window, double xOffset, double yOffset) {
    #define Y_OFFSET elem->extraData->scrollData.yOffset
    #define X_OFFSET elem->extraData->scrollData.xOffset

    #ifdef DEBUG_SCROLL
        cout << "\nScroll callback: xOffset: " << xOffset << ", yOffset: " << yOffset;
    #endif

    highlightedContentBlock.show = false;
    highlightedContentBlock.updated = true;
    UIElement* elem = getElementFromScreenCoords(&rootContainer, mouseX, mouseY);

    if (elem != nullptr) {
        switch (elem->type) {
            case UI_SCROLL_AREA: {
                bool scrolled = false;
                bool shiftPressed = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS
                    || glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
                bool horizontalScroll = xOffset != 0 || shiftPressed && yOffset != 0;
                bool verticalScroll = !shiftPressed && yOffset != 0;

                if (verticalScroll) {
                    if (elem->height + Y_OFFSET < elem->extraData->scrollData.height && yOffset < 0) {
                        Y_OFFSET += SCROLL_AMOUNT;
                    } else if (yOffset > 0) {
                        Y_OFFSET -= SCROLL_AMOUNT;
                    }

                    if (Y_OFFSET < 0) {
                        Y_OFFSET = 0;
                    } else if (Y_OFFSET > elem->extraData->scrollData.height) {
                        Y_OFFSET = elem->extraData->scrollData.height - elem->height;
                    }

                    scrolled = true;
                }

                if (horizontalScroll) {
                    if (
                        elem->width + X_OFFSET < elem->extraData->scrollData.width
                        && (!shiftPressed && xOffset < 0 || yOffset < 0)
                    ) {
                        X_OFFSET += SCROLL_AMOUNT;
                    } else if (
                        X_OFFSET > 0 && !shiftPressed && xOffset > 0 || yOffset > 0
                    ) {
                        X_OFFSET -= SCROLL_AMOUNT;
                    }

                    if (X_OFFSET < 0) {
                        X_OFFSET = 0;
                    }

                    scrolled = true;
                }

                if (scrolled) {
                    elem->extraData->scrollData.lastScrolled = glfwGetTime();
                    elem->extraData->scrollData.renderedUpdate = false;
                }
            }
            break;

        case UI_CONTENT_CONTAINER:
            ContentTab* tab = &tabs[activeTabIndex];

            if (elem->height + tab->scrollOffsetY < tab->maxHeight && yOffset < 0) {
                tab->scrollOffsetY += SCROLL_AMOUNT;
            } else if (yOffset > 0) {
                tab->scrollOffsetY -= SCROLL_AMOUNT;
            }

            if (tab->scrollOffsetY < 0) {
                tab->scrollOffsetY = 0;
            } else if (tab->scrollOffsetY > tab->maxHeight) {
                tab->scrollOffsetY = tab->maxHeight - contentContainer->height;
            }

            if (tab->showCursor) {
                tab->redrawCursor = true;
            }

            tab->scrollbarUpdated = false;

            cout << "\nDEBUG: ON SCROLL: tab->scrollOffsetY: " << tab->scrollOffsetY;

            break;
        }
    }

    #undef Y_OFFSET
    #undef X_OFFSET
}

void minimizedCallback(GLFWwindow* window, int isMinimized) {
    minimized = isMinimized != 0;
}

void maximizedCallback(GLFWwindow* window, int min) {
    minimized = min == 0;
}

void windowFocusCallback(GLFWwindow *window, int isFocused) {
    focused = isFocused != 0;
}

void setupUI(bool isDarkTheme) {
    windowHeight = WINDOW_HEIGHT;
    windowWidth = WINDOW_WIDTH;
    resolution = glm::vec2(windowWidth, windowHeight);

    textCursor = glfwCreateStandardCursor(GLFW_IBEAM_CURSOR);
    pointerCursor = glfwCreateStandardCursor(GLFW_HAND_CURSOR);

    initShaders();
    initTextRendering(windowWidth, windowHeight);

    glEnable(GL_BLEND);
    glEnable(GL_SAMPLE_SHADING);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    float vertices[] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };

    SETUP_BUFFERS(highlightedContentBlock.backgroundVAO, highlightedContentBlock.backgroundVBO, vertices);
    SETUP_BUFFERS(highlightedContentBlock.dragTabVAO, highlightedContentBlock.dragTabVBO, vertices);

    UIElement* menuBar = &elements[0];
    UIElement* newButton = &elements[1];
    UIElement* openButton = &elements[2];
    contentContainer = &elements[3];

    initElement(rootContainer, UI_CONTAINER, 0.0f, 0.0f, WINDOW_WIDTH, WINDOW_HEIGHT);
    initElement(*menuBar, UI_CONTAINER, 0.0f, 0.0f, WINDOW_WIDTH, 64.0f);
    initElement(*newButton, UI_ICON_BUTTON, 8.0f, 8.0f, 48.0f, 48.0f);
    initElement(*openButton, UI_ICON_BUTTON, 8.0f, 64.0f, 48.0f, 48.0f);
    initElement(*contentContainer, UI_CONTENT_CONTAINER, 64.0f, 0.0f, WINDOW_WIDTH, WINDOW_HEIGHT - 64.0f);

    if (darkTheme) {
        rootContainer.backgroundColor = ROOT_BG_DARK;
        menuBar->backgroundColor = TOOL_BAR_BG_DARK;
        menuBar->color = WHITE;
    } else {
        rootContainer.backgroundColor = ROOT_BG;
        menuBar->backgroundColor = TOOL_BAR_BG;
        menuBar->color = BLACK;
    }

    menuBar->text = "test";

    menuBar->preferences->widthPreference = UI_FULL;
    menuBar->preferences->heightPreference = UI_FIXED;
    menuBar->preferences->verticalAlign = UI_ALIGN_CENTER;
    menuBar->preferences->horizontalAlign = UI_ALIGN_CENTER;

    newButton->text = "New";
    newButton->icon = Icon::Plus;

    openButton->icon = Icon::File;
    openButton->text = "Open";

    contentContainer->text = "Main Scroll Area";
    contentContainer->extraData->scrollData.width = WINDOW_WIDTH;
    contentContainer->extraData->scrollData.height = WINDOW_HEIGHT - 64;
    contentContainer->preferences->widthPreference = UI_FULL;
    contentContainer->preferences->heightPreference = UI_STRETCH;

    newTab("This is a test note title!");

    appendContentBlock(0, ContentBlockType::Paragraph, "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Suspendisse accumsan lobortis sapien a facilisis. Fusce volutpat odio at purus ullamcorper, a feugiat leo vulputate. Etiam et tortor odio.");
    appendContentBlock(0, ContentBlockType::Heading1, "Vivamus scelerisque ac lorem quis lacinia.");
    appendContentBlock(0, ContentBlockType::Paragraph, "Etiam at erat vel ex placerat auctor at quis arcu. Nullam tempus arcu facilisis nisl sodales molestie. Curabitur enim nisi, aliquam sed risus id, ultricies fringilla nisl. Integer accumsan tincidunt pulvinar. Lorem ipsum dolor sit amet, consectetur adipiscing elit. Cras lacinia convallis velit, eu malesuada metus. Proin posuere gravida eros, a iaculis magna porta a.");
    appendContentBlock(0, ContentBlockType::Heading2, "Praesent rutrum odio in neque commodo pretium.");
    appendContentBlock(0, ContentBlockType::Paragraph, "Etiam auctor est sed sagittis ullamcorper. Cras suscipit eu ligula eu feugiat. Nulla vitae nibh justo. Curabitur pretium urna quis nisi ornare imperdiet. Donec diam diam, rutrum sed massa eu, tristique volutpat felis. Morbi pulvinar ante leo, eu tristique velit imperdiet sit amet. Praesent venenatis bibendum erat quis lacinia. Interdum et malesuada fames ac ante ipsum primis in faucibus. Pellentesque porta convallis rutrum.");
    appendContentBlock(0, ContentBlockType::Heading3, "Aenean porta tortor in tellus blandit consectetur.");
    appendContentBlock(0, ContentBlockType::Paragraph, "Pellentesque ac tellus maximus, scelerisque risus sit amet, consequat justo. Phasellus lobortis massa quis ipsum eleifend imperdiet. Aliquam in convallis tellus, sit amet pharetra diam. Praesent eu sagittis nunc, scelerisque maximus enim. Vivamus varius, elit sed rutrum dapibus, ante purus sodales lorem, vitae malesuada risus dolor quis leo. Proin porta ex orci, ac vulputate purus pulvinar sed. Curabitur non porta sapien. Mauris tortor magna, congue ac volutpat vel, aliquam eget elit. Nulla ullamcorper tempus felis, et fermentum dolor condimentum a. Nullam rhoncus cursus consequat.");

    addChild(menuBar, newButton);
    addChild(menuBar, openButton);

    addChild(&rootContainer, menuBar);
    addChild(&rootContainer, contentContainer);

    initFontTexture(getFontAtlasPath(0).string());

    activeElement = contentContainer;

    #ifdef DEBUG_CONTENT_BLOCKS
        printContentBlocks(0);
    #endif
}

void enterRenderLoop() {
    float fpsPrevTime = 0.0f;
    uint16_t fps = 0;
    uint16_t newFps = 0;

    while (!glfwWindowShouldClose(window)) {
        if (minimized) {
            glfwWaitEventsTimeout(4000.0f);
        }

        if (!focused) {
            glfwWaitEventsTimeout(80.0f);
        }

        glm::vec2 cursor(mouseX, mouseY);

        renderElement(rootContainer, resolution, cursor);

        if (showFps) {
            float time = glfwGetTime();

            if (time - fpsPrevTime > 1.0f) {
                fpsPrevTime = time;
                fps = newFps;
                newFps = 0;
            } else {
                ++newFps;
            }

            renderText(
                "FPS: " + to_string(fps),
                glm::vec2(0.0f, 0.0f),
                resolution,
                glm::vec4(1.0f, 0.0f, 0.0f, 1.0f),
                16,
                nullptr
            );
        }

        glfwSwapBuffers(window);
        glfwPollEvents();

        #ifdef DEBUG_TEXT
        debugTextFirstRenderPrint = false;
        #endif
        // glfwSwapInterval(1);
    }
}

void defaultCallback(UIElement* _elem) {
    cout << "\nDefault callback executed!";
}

void defaultKeyCallback(UIElement* elem, int key, int action, int mod) {
    cout << "\nDefault callback executed!";
}

void defaultOnMouseUpDownCallback(UIElement*, float, float, glm::vec2) {
    cout << "\nDefault mouse up/down callback executed!";
}

void cbKeyCallback(int key, int action, int mod) {
    ContentTab* tab = &tabs[activeTabIndex];
    ContentBlock* cb = &tab->contentBlocks[tab->cursorCBIndex];

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
        #define TYPE(c) processCBCommand(ContentCommand {\
                CommandType::Type, false, 0, tab->cursorCBIndex, tab->cursorPosition, c\
                });\
            tab->lastCursorTime = static_cast<float>(glfwGetTime());\
            tab->cursorPosition++;\
            tab->redrawCursor = true
        #define BASIC_CMD(cmdType) ContentCommand {\
                cmdType, false, 0, tab->cursorCBIndex, tab->cursorPosition, ""\
            }
        #define INCR_CURSOR() if (tab->cursorPosition < cb->rope->length()) { tab->cursorPosition++; }
        #define DECR_CURSOR() if (tab->cursorPosition > 0) { tab->cursorPosition--; }
        #define CUR_POS tab->cursorPosition
        #define UPDATE() tab->lastCursorTime = static_cast<float>(glfwGetTime()); tab->redrawCursor = true
        #define UNSELECT() tab->textSelected = false;\
            tab->selStartCBIndex = 0;\
            tab->selStartCursorPosition = 0;\
            tab->selEndCBIndex = 0;\
            tab->selEndCursorPosition = 0

        switch (key) {
            case GLFW_KEY_ESCAPE:
                if (tab->textSelected) {
                    UNSELECT();
                } else {
                    tab->showCursor = false;
                }
                break;

            case GLFW_KEY_LEFT:
                if (ctrl) {
                    uint32_t i = cb->rope->length();
                    uint32_t cumulativeLen = i;
                    --i;

                    ReverseRopeTraverser rt(cb->rope);

                    bool found = false;

                    while (rt.hasNext()) {
                        Rope* rope = rt.next();

                        if (cumulativeLen - rope->length() > tab->cursorPosition) {
                            cumulativeLen -= rope->length();
                            continue;
                        }

                        if (cumulativeLen >= 0) {
                            i = cumulativeLen - 1;
                        }

                        // for (char c: rope->text) {
                        for (string::reverse_iterator c = rope->text.rbegin(); c != rope->text.rend(); ++c) {
                            if (i >= tab->cursorPosition) {
                                --i;
                                continue;
                            }

                            if (*c == ' ') {
                                found = true;
                                break;
                            }

                            if (!isalnum(*c)) {
                                found = true;
                                break;
                            }

                            --i;
                        }

                        if (found) {
                            tab->cursorPosition = i;
                            cout << "\nCTRL + LEFT: cursorPosition = " << i;
                            break;
                        }
                    }

                    if (!found) {
                        if (tab->cursorPosition > 0) {
                            tab->cursorPosition = 0;
                        } else if (tab->cursorPosition == 0) {
                            UNSELECT();

                            if (tab->cursorPosition == 0 && tab->cursorCBIndex > 0) {
                                --tab->cursorCBIndex;
                                ContentBlock* newCB = &tab->contentBlocks[tab->cursorCBIndex];

                                tab->cursorPosition = newCB->rope->length();
                                tab->cursorRow = newCB->rows - 1;
                            } else {
                                DECR_CURSOR();
                            }
                        }
                    }
                } else if (shift) {
                    if (tab->textSelected) {
                        if (
                            tab->cursorPosition - 1 == tab->initialCursorPosition
                            && tab->cursorCBIndex == tab->selStartCBIndex
                            && tab->cursorCBIndex == tab->selEndCBIndex
                        ) {
                            tab->textSelected = false;
                            --tab->cursorPosition;
                        } else if (tab->selStartCBIndex != tab->selEndCBIndex) {
                            if (tab->cursorCBIndex > tab->initialCBIndex) {
                                if (tab->selEndCursorPosition == 0 && tab->cursorCBIndex > 0) {
                                    --tab->cursorCBIndex;
                                    --tab->selEndCBIndex;
                                    tab->selEndCursorPosition = tab->contentBlocks[tab->cursorCBIndex].rope->length();
                                    tab->cursorPosition = tab->selEndCursorPosition;
                                } else if (tab->selEndCursorPosition > 0) {
                                    --tab->selEndCursorPosition;
                                    --tab->cursorPosition;
                                }
                            } else {
                                if (tab->selStartCursorPosition == 0 && tab->cursorCBIndex > 0) {
                                    --tab->cursorCBIndex;
                                    --tab->selStartCBIndex;
                                    tab->selStartCursorPosition = tab->contentBlocks[tab->cursorCBIndex].rope->length();
                                    tab->cursorPosition = tab->selEndCursorPosition;
                                } else if (tab->selStartCursorPosition > 0) {
                                    --tab->selStartCursorPosition;
                                    --tab->cursorPosition;
                                }
                            }
                        } else {
                            if (tab->cursorPosition > tab->initialCursorPosition) {
                                if (tab->selEndCursorPosition == 0 && tab->cursorCBIndex > 0) {
                                    --tab->cursorCBIndex;
                                    --tab->selEndCBIndex;
                                    tab->selEndCursorPosition = tab->contentBlocks[tab->cursorCBIndex].rope->length();
                                    tab->cursorPosition = tab->selEndCursorPosition;
                                } else if (tab->selEndCursorPosition > 0) {
                                    --tab->selEndCursorPosition;
                                    --tab->cursorPosition;
                                }
                            } else {
                                if (tab->selStartCursorPosition == 0 && tab->cursorCBIndex > 0) {
                                    --tab->cursorCBIndex;
                                    --tab->selStartCBIndex;
                                    tab->selStartCursorPosition = tab->contentBlocks[tab->cursorCBIndex].rope->length();
                                    tab->cursorPosition = tab->selEndCursorPosition;
                                } else if (tab->selStartCursorPosition > 0) {
                                    --tab->selStartCursorPosition;
                                    --tab->cursorPosition;
                                }
                            }
                        }
                    } else {
                        tab->textSelected = true;
                        tab->initialCursorPosition = tab->cursorPosition;
                        tab->selStartCursorPosition = tab->cursorPosition - 1;
                        tab->selEndCursorPosition = tab->cursorPosition;
                        tab->selStartCBIndex = tab->cursorCBIndex;
                        tab->selEndCBIndex = tab->cursorCBIndex;
                        tab->initialCBIndex = tab->cursorCBIndex;

                        if (tab->cursorPosition > 0) {
                            --tab->cursorPosition;
                        }
                    }

                    tab->redrawSel = true;
                } else {
                    UNSELECT();

                    if (tab->cursorPosition == 0 && tab->cursorCBIndex > 0) {
                        --tab->cursorCBIndex;
                        ContentBlock* newCB = &tab->contentBlocks[tab->cursorCBIndex];

                        tab->cursorPosition = newCB->rope->length();
                        tab->cursorRow = newCB->rows - 1;
                    } else {
                        DECR_CURSOR();
                    }
                }

                tab->updateCursorX = true;
                UPDATE();
                break;

            case GLFW_KEY_RIGHT:
                if (ctrl) {
                    uint32_t i = 0;
                    uint32_t cumulativeLen = 0;

                    RopeTraverser rt(cb->rope);

                    bool found = false;

                    while (rt.hasNext()) {
                        Rope* rope = rt.next();

                        if (cumulativeLen + rope->length() < tab->cursorPosition) {
                            cumulativeLen += rope->length();
                            continue;
                        }

                        if (cumulativeLen > 0) {
                            i = cumulativeLen - 1;
                        }

                        for (char c: rope->text) {
                            if (i <= tab->cursorPosition) {
                                ++i;
                                continue;
                            }

                            if (c == ' ') {
                                ++i;
                                found = true;
                                break;
                            }

                            if (!isalnum(c)) {
                                // --i;
                                found = true;
                                break;
                            }

                            ++i;
                        }

                        if (found) {
                            tab->cursorPosition = i;
                            cout << "\nCTRL + RIGHT: cursorPosition = " << i;
                            break;
                        }
                    }

                    if (!found && tab->cursorPosition <= cb->rope->length()) {
                        UNSELECT();

                        if (
                            tab->cursorPosition == cb->rope->length()
                            && tab->cursorCBIndex < tab->contentBlocks.size() - 1
                        ) {
                            tab->cursorPosition = 0;
                            tab->cursorRow = 0;
                            ++tab->cursorCBIndex;
                        } else {
                            INCR_CURSOR();
                        }
                    }
                } else if (shift) {
                    if (tab->textSelected) {
                        if (
                            tab->cursorPosition + 1 == tab->initialCursorPosition
                            && tab->cursorCBIndex == tab->selStartCBIndex
                            && tab->cursorCBIndex == tab->selEndCBIndex
                        ) {
                            tab->textSelected = false;
                            ++tab->cursorPosition;
                        } else if (tab->selStartCBIndex != tab->selEndCBIndex) {
                            if (tab->cursorCBIndex > tab->initialCBIndex) {
                                if (
                                    tab->selEndCursorPosition == cb->rope->length()
                                    && tab->cursorCBIndex < tabs.size() - 1
                                ) {
                                    ++tab->cursorCBIndex;
                                    ++tab->selEndCBIndex;
                                    tab->selEndCursorPosition = 0;
                                    tab->cursorPosition = 0;
                                } else if (tab->selEndCursorPosition < cb->rope->length()) {
                                    ++tab->selEndCursorPosition;
                                    ++tab->cursorPosition;
                                }
                            } else {
                                if (
                                    tab->selStartCursorPosition == cb->rope->length()
                                    && tab->cursorCBIndex < tabs.size() - 1
                                ) {
                                    ++tab->cursorCBIndex;
                                    ++tab->selStartCBIndex;
                                    tab->selStartCursorPosition = 0;
                                    tab->cursorPosition = 0;
                                } else if (tab->selStartCursorPosition < cb->rope->length()) {
                                    ++tab->selStartCursorPosition;
                                    ++tab->cursorPosition;
                                }
                            }
                        } else {
                            if (tab->cursorPosition > tab->initialCursorPosition) {
                                if (
                                    tab->selEndCursorPosition == cb->rope->length()
                                    && tab->cursorCBIndex > tabs.size() - 1
                                ) {
                                    ++tab->cursorCBIndex;
                                    ++tab->selEndCBIndex;
                                    tab->selEndCursorPosition = 0;
                                    tab->cursorPosition = 0;
                                } else if (tab->selEndCursorPosition < cb->rope->length()) {
                                    ++tab->selEndCursorPosition;
                                    ++tab->cursorPosition;
                                }
                            } else {
                                if (
                                    tab->selStartCursorPosition == cb->rope->length()
                                    && tab->cursorCBIndex < tabs.size() - 1
                                ) {
                                    ++tab->cursorCBIndex;
                                    ++tab->selStartCBIndex;
                                    tab->selStartCursorPosition = 0;
                                    tab->cursorPosition = 0;
                                } else if (tab->selStartCursorPosition < cb->rope->length()) {
                                    ++tab->selStartCursorPosition;
                                    ++tab->cursorPosition;
                                }
                            }
                        }
                    } else {
                        tab->textSelected = true;
                        tab->initialCursorPosition = tab->cursorPosition;
                        tab->selStartCursorPosition = tab->cursorPosition;
                        tab->selEndCursorPosition = ++tab->cursorPosition;
                        tab->selStartCBIndex = tab->cursorCBIndex;
                        tab->selEndCBIndex = tab->cursorCBIndex;
                        tab->initialCBIndex = tab->cursorCBIndex;
                    }

                    tab->redrawSel = true;
                } else {
                    UNSELECT();

                    if (
                        tab->cursorPosition == cb->rope->length()
                        && tab->cursorCBIndex < tab->contentBlocks.size() - 1
                    ) {
                        tab->cursorPosition = 0;
                        tab->cursorRow = 0;
                        ++tab->cursorCBIndex;
                    } else {
                        INCR_CURSOR();
                    }
                }

                tab->updateCursorX = true;
                UPDATE();
                break;

            case GLFW_KEY_UP: {
                    SelectionData sd = getCBTextCursorVertical(tab, false, true);

                    cout << "\nSelectionData:  cursorPosition: " << sd.cursorPosition
                        << ", row: " << sd.row << ", col: " << sd.col;

                    if (shift) {
                        if (sd.cursorCBIndex == 0) {
                            tab->textSelected = false;
                            tab->cursorPosition = sd.cursorPosition;
                            tab->cursorCBIndex = 0;
                        } else if (sd.cursorCBIndex != tab->cursorCBIndex) {
                            if (tab->textSelected) {
                                if (sd.cursorCBIndex < tab->initialCBIndex) {
                                    tab->selStartCBIndex = sd.cursorCBIndex;
                                    tab->selStartCursorPosition = sd.cursorPosition;
                                    tab->selEndCBIndex = tab->initialCBIndex;
                                } else {
                                    tab->selStartCBIndex = tab->initialCBIndex;
                                    tab->selEndCBIndex = sd.cursorCBIndex;
                                    tab->selEndCursorPosition = sd.cursorPosition;
                                }
                            } else {
                                tab->textSelected = true;

                                tab->selStartCBIndex = sd.cursorCBIndex;
                                tab->selStartCursorPosition = sd.cursorPosition;
                                tab->selEndCBIndex = tab->cursorCBIndex;
                                tab->selEndCursorPosition = tab->cursorPosition;
                                tab->initialCBIndex = tab->cursorCBIndex;
                                tab->initialCursorPosition = tab->cursorPosition;
                            }
                        } else if (sd.cursorPosition != tab->cursorPosition) {
                            if (tab->textSelected) {
                                if (tab->selStartCBIndex == tab->selEndCBIndex) {
                                    if (sd.cursorPosition == tab->initialCursorPosition) {
                                        tab->textSelected = false;
                                    } else {
                                        if (sd.cursorPosition < tab->initialCursorPosition) {
                                            tab->selStartCursorPosition = sd.cursorPosition;
                                            tab->selEndCursorPosition = tab->initialCursorPosition;
                                        } else {
                                            tab->selStartCursorPosition = tab->initialCursorPosition;
                                            tab->selEndCursorPosition = sd.cursorPosition;
                                        }
                                    }
                                } else {
                                    if (tab->selStartCBIndex == sd.cursorCBIndex) {
                                        tab->selStartCursorPosition = sd.cursorPosition;
                                    } else if (tab->selEndCBIndex == sd.cursorCBIndex) {
                                        tab->selEndCursorPosition = sd.cursorPosition;
                                    } else {
                                        cout << "\n\nBhai is condition ke bare me toh socha hi nahi thaa!!";
                                    }
                                }
                            } else {
                                tab->textSelected = true;
                                tab->initialCursorPosition = tab->cursorPosition;
                                tab->initialCBIndex = tab->cursorCBIndex;

                                tab->selStartCBIndex = sd.cursorCBIndex;
                                tab->selEndCBIndex = tab->cursorCBIndex;
                                tab->selStartCursorPosition = sd.cursorPosition;
                                tab->selEndCursorPosition = tab->cursorPosition;
                                tab->initialCBIndex = tab->cursorCBIndex;
                            }
                        }

                        tab->redrawSel = true;
                    } else {
                        UNSELECT();
                    }

                    updateCBCursor(tab, sd);
                }
                break;

            case GLFW_KEY_DOWN: {
                    SelectionData sd = getCBTextCursorVertical(tab, false, false);

                    cout << "\nSelectionData:  cursorPosition: " << sd.cursorPosition
                        << ", row: " << sd.row << ", col: " << sd.col;

                    if (shift) {
                        if (sd.cursorCBIndex != tab->cursorCBIndex) {
                            cbTextDragging = true;

                            if (tab->textSelected) {
                                if (sd.cursorCBIndex == tab->initialCBIndex) {
                                    tab->selStartCBIndex = sd.cursorCBIndex;
                                    tab->selEndCBIndex = tab->initialCBIndex;
                                    tab->selStartCursorPosition = sd.cursorPosition;
                                } else if (sd.cursorCBIndex < tab->initialCBIndex) {
                                    tab->selStartCBIndex = sd.cursorCBIndex;
                                    tab->selEndCBIndex = tab->initialCBIndex;
                                    tab->selStartCursorPosition = sd.cursorPosition;
                                } else {
                                    tab->selStartCBIndex = tab->initialCBIndex;
                                    tab->selEndCBIndex = sd.cursorCBIndex;
                                    tab->selEndCursorPosition = sd.cursorPosition;
                                }
                            } else {
                                tab->textSelected = true;

                                tab->selStartCBIndex = tab->cursorCBIndex;
                                tab->selStartCursorPosition = tab->cursorPosition;
                                tab->selEndCBIndex = sd.cursorCBIndex;
                                tab->selEndCursorPosition = sd.cursorPosition;
                                tab->initialCBIndex = tab->cursorCBIndex;
                                tab->initialCursorPosition = tab->cursorPosition;
                            }

                            tab->cursorCBIndex = sd.cursorCBIndex;
                            tab->cursorPosition = sd.cursorPosition;
                        } else if (sd.cursorPosition != tab->cursorPosition) {
                            cbTextDragging = true;

                            if (tab->textSelected) {
                                if (tab->selStartCBIndex == tab->selEndCBIndex) {
                                    if (sd.cursorPosition == tab->initialCursorPosition) {
                                        tab->textSelected = false;
                                    } else {
                                        if (sd.cursorPosition < tab->initialCursorPosition) {
                                            tab->selStartCursorPosition = sd.cursorPosition;
                                            tab->selEndCursorPosition = tab->initialCursorPosition;
                                        } else {
                                            tab->selStartCursorPosition = tab->initialCursorPosition;
                                            tab->selEndCursorPosition = sd.cursorPosition;
                                        }
                                    }
                                } else {
                                    if (tab->selStartCBIndex == sd.cursorCBIndex) {
                                        tab->selStartCursorPosition = sd.cursorPosition;
                                    } else if (tab->selEndCBIndex == sd.cursorCBIndex) {
                                        tab->selEndCursorPosition = sd.cursorPosition;
                                    } else {
                                        cout << "\n\nBhai is condition ke bare me toh socha hi nahi thaa!!";
                                    }
                                }
                            } else {
                                tab->textSelected = true;
                                tab->initialCursorPosition = tab->cursorPosition;
                                tab->initialCBIndex = tab->cursorCBIndex;

                                tab->selStartCBIndex = tab->cursorCBIndex;
                                tab->selEndCBIndex = sd.cursorCBIndex;
                                tab->selStartCursorPosition = tab->cursorPosition;
                                tab->selEndCursorPosition = sd.cursorPosition;
                                tab->initialCBIndex = tab->cursorCBIndex;
                            }

                            tab->cursorCBIndex = sd.cursorCBIndex;
                            tab->cursorPosition = sd.cursorPosition;
                            tab->updateCursorX = true;
                            tab->redrawSel = true;
                        }

                        tab->redrawSel = true;
                    } else {
                        UNSELECT();
                    }

                    updateCBCursor(tab, sd);
                }
                break;

            case GLFW_KEY_ENTER:
                #ifdef DEBUG_TEXT_INPUT
                    cout << "\nEnter pressed";
                #endif

                processCBCommand(BASIC_CMD(CommandType::NewLine));
                tab->redrawCursor = true;
                break;

            case GLFW_KEY_END:
                CUR_POS = static_cast<uint32_t>(cb->rope->length());
                tab->updateCursorX = true;
                UPDATE();
                break;

            case GLFW_KEY_HOME:
                CUR_POS = 0;
                tab->updateCursorX = true;
                UPDATE();
                break;

            case GLFW_KEY_PAGE_UP: {
                    glm::vec2 dim = getActiveContentDimensions();

                    if (tab->scrollOffsetY - dim.y > 0) {
                        tab->scrollOffsetY -= dim.y;
                    } else {
                        tab->scrollOffsetY = 0;
                    }

                    vector<ContentBlock>::iterator fvcb = getFirstVisibleCB(tab);

                    if (fvcb != tab->contentBlocks.begin()) {
                        int64_t dist = distance(tab->contentBlocks.begin(), fvcb) - 1;

                        if (dist > -1) {
                            tab->cursorCBIndex = static_cast<uint32_t>(dist);
                        } else {
                            tab->cursorCBIndex = 0;
                        }
                    }

                    SelectionData sd = getCBTextCursorVertical(tab, true, true);
                    updateCBCursor(tab, sd);

                    tab->scrollbarUpdated = false;
                    highlightedContentBlock.show = false;
                    UPDATE();
                }
                break;

            case GLFW_KEY_PAGE_DOWN: {
                    glm::vec2 dim = getActiveContentDimensions();

                    if (tab->scrollOffsetY + dim.y < tab->maxHeight - dim.y) {
                        tab->scrollOffsetY += dim.y;
                    } else {
                        tab->scrollOffsetY = tab->maxHeight - contentContainer->height;
                    }

                    vector<ContentBlock>::iterator fvcb = getLastVisibleCB(tab);

                    if (fvcb != tab->contentBlocks.begin()) {
                        int64_t dist = distance(tab->contentBlocks.begin(), fvcb) + 1;

                        if (dist < tab->contentBlocks.size()) {
                            tab->cursorCBIndex = static_cast<uint32_t>(dist);
                        } else {
                            tab->cursorCBIndex = tab->contentBlocks.size() - 1;
                        }
                    }

                    SelectionData sd = getCBTextCursorVertical(tab, true, false);
                    updateCBCursor(tab, sd);

                    tab->scrollbarUpdated = false;
                    highlightedContentBlock.show = false;
                    UPDATE();
                }
                break;

            case GLFW_KEY_BACKSPACE:
                if (
                    CUR_POS <= cb->rope->length() && CUR_POS > 0
                    || tab->textSelected
                ) {
                    if (tab->textSelected){
                        tab->extCommands.push_back(ContentCommandExt {
                            tab->selEndCBIndex,
                            tab->selEndCursorPosition,
                            vector<ContentBlock>(),
                        });

                        processCBCommand(ContentCommand {
                            CommandType::DeleteSelected,
                            true,
                            static_cast<uint16_t>(tab->extCommands.size() - 1),
                            tab->selStartCBIndex,
                            tab->selStartCursorPosition,
                            ""
                        });

                        tab->cursorPosition = tab->selStartCursorPosition;
                        tab->cursorCBIndex = tab->selStartCBIndex;
                        UNSELECT();
                    } else {
                        processCBCommand(ContentCommand {
                            CommandType::Backspace,
                            false,
                            0,
                            tab->cursorCBIndex, CUR_POS,
                            string(1, (*cb->rope)[CUR_POS - 1])
                        });

                        if (CUR_POS != 0) {
                            DECR_CURSOR();
                        }
                    }

                    UPDATE();
                } else if (CUR_POS == 0) {
                    if (tab->textSelected) {
                        tab->extCommands.push_back(ContentCommandExt {
                            tab->selEndCBIndex,
                            tab->selEndCursorPosition,
                            vector<ContentBlock>(),
                        });

                        processCBCommand(ContentCommand {
                            CommandType::DeleteSelected,
                            true,
                            static_cast<uint16_t>(tab->extCommands.size() - 1),
                            tab->selStartCBIndex,
                            tab->selStartCursorPosition,
                            ""
                        });

                        tab->cursorPosition = tab->selStartCursorPosition;
                        tab->cursorCBIndex = tab->selStartCBIndex;
                        UNSELECT();
                    } else {
                        if (tab->cursorCBIndex > 1) {
                            vector<ContentBlock> cbs;
                            ContentBlock cb = tab->contentBlocks[tab->cursorCBIndex];
                            cbs.push_back(cb);

                            tab->extCommands.push_back(ContentCommandExt {
                                tab->cursorCBIndex,
                                0,
                                cbs,
                            });

                            uint32_t newCursorPos = tab->contentBlocks[tab->cursorCBIndex - 1].rope->length();

                            processCBCommand(ContentCommand {
                                CommandType::DeleteMergeCB,
                                true,
                                static_cast<uint16_t>(tab->extCommands.size() - 1),
                                tab->cursorCBIndex - 1,
                                0,
                                "",
                            });

                            --tab->cursorCBIndex;
                            tab->cursorPosition = newCursorPos;
                            UPDATE();
                        }
                    }
                }
                break;

            case GLFW_KEY_DELETE:
                if (
                    CUR_POS < cb->rope->length() && CUR_POS >= 0
                    || tab->textSelected
                ) {
                    if (tab->textSelected) {
                        tab->extCommands.push_back(ContentCommandExt {
                            tab->selEndCBIndex,
                            tab->selEndCursorPosition,
                            vector<ContentBlock>(),
                        });

                        processCBCommand(ContentCommand {
                            CommandType::DeleteSelected,
                            true,
                            static_cast<uint16_t>(tab->extCommands.size() - 1),
                            tab->selStartCBIndex,
                            tab->selStartCursorPosition,
                            ""
                        });

                        tab->cursorPosition = tab->selStartCursorPosition;
                        tab->cursorCBIndex = tab->selStartCBIndex;
                        UNSELECT();
                    } else {
                        processCBCommand(ContentCommand {
                            CommandType::Delete,
                            false,
                            0,
                            tab->cursorCBIndex, CUR_POS,
                            string(1, (*cb->rope)[CUR_POS])
                        });
                    }

                    UPDATE();
                } else if (CUR_POS == cb->rope->length()) {
                    if (tab->textSelected) {
                        tab->extCommands.push_back(ContentCommandExt {
                            tab->selEndCBIndex,
                            tab->selEndCursorPosition,
                            vector<ContentBlock>(),
                        });

                        processCBCommand(ContentCommand {
                            CommandType::DeleteSelected,
                            true,
                            static_cast<uint16_t>(tab->extCommands.size() - 1),
                            tab->selStartCBIndex,
                            tab->selStartCursorPosition,
                            ""
                        });

                        tab->cursorPosition = tab->selStartCursorPosition;
                        tab->cursorCBIndex = tab->selStartCBIndex;
                        UNSELECT();
                    } else {
                        if (
                            tab->cursorCBIndex != 0
                            && tab->cursorCBIndex + 1 < tab->contentBlocks.size()
                        ) {
                            vector<ContentBlock> cbs;
                            ContentBlock cb = tab->contentBlocks[tab->cursorCBIndex + 1];
                            cbs.push_back(cb);

                            tab->extCommands.push_back(ContentCommandExt {
                                tab->cursorCBIndex + 1,
                                0,
                                cbs,
                            });

                            processCBCommand(ContentCommand {
                                CommandType::DeleteMergeCB,
                                true,
                                static_cast<uint16_t>(tab->extCommands.size() - 1),
                                tab->cursorCBIndex,
                                0,
                                "",
                            });
                        }
                    }
                }
                break;

            case GLFW_KEY_A:
                if (ctrl) {
                    const uint32_t len = cb->rope->length();
                    tab->textSelected = true;
                    tab->initialCursorPosition = tab->cursorPosition;
                    // tab->selStartCBIndex = 0;
                    // tab->selEndCol = len;
                    tab->redrawSel = true;
                    tab->cursorPosition = len;
                } else if (capsLck || shift) { TYPE("A"); } else { TYPE("a"); }

                break;

            case GLFW_KEY_B: if (capsLck || shift) { TYPE("B"); } else { TYPE("b"); } break;
            case GLFW_KEY_C: if (capsLck || shift) { TYPE("C"); } else { TYPE("c"); } break;
            case GLFW_KEY_D: if (capsLck || shift) { TYPE("D"); } else { TYPE("d"); } break;
            case GLFW_KEY_E: if (capsLck || shift) { TYPE("E"); } else { TYPE("e"); } break;
            case GLFW_KEY_F: if (capsLck || shift) { TYPE("F"); } else { TYPE("f"); } break;
            case GLFW_KEY_G: if (capsLck || shift) { TYPE("G"); } else { TYPE("g"); } break;
            case GLFW_KEY_H: if (capsLck || shift) { TYPE("H"); } else { TYPE("h"); } break;
            case GLFW_KEY_I: if (capsLck || shift) { TYPE("I"); } else { TYPE("i"); } break;
            case GLFW_KEY_J: if (capsLck || shift) { TYPE("J"); } else { TYPE("j"); } break;
            case GLFW_KEY_K: if (capsLck || shift) { TYPE("K"); } else { TYPE("k"); } break;
            case GLFW_KEY_L: if (capsLck || shift) { TYPE("L"); } else { TYPE("l"); } break;
            case GLFW_KEY_M: if (capsLck || shift) { TYPE("M"); } else { TYPE("m"); } break;
            case GLFW_KEY_N: if (capsLck || shift) { TYPE("N"); } else { TYPE("n"); } break;
            case GLFW_KEY_O: if (capsLck || shift) { TYPE("O"); } else { TYPE("o"); } break;
            case GLFW_KEY_P: if (capsLck || shift) { TYPE("P"); } else { TYPE("p"); } break;
            case GLFW_KEY_Q: if (capsLck || shift) { TYPE("Q"); } else { TYPE("q"); } break;
            case GLFW_KEY_R: if (capsLck || shift) { TYPE("R"); } else { TYPE("r"); } break;
            case GLFW_KEY_S: if (capsLck || shift) { TYPE("S"); } else { TYPE("s"); } break;
            case GLFW_KEY_T: if (capsLck || shift) { TYPE("T"); } else { TYPE("t"); } break;
            case GLFW_KEY_U: if (capsLck || shift) { TYPE("U"); } else { TYPE("u"); } break;
            case GLFW_KEY_V: if (capsLck || shift) { TYPE("V"); } else { TYPE("v"); } break;
            case GLFW_KEY_W: if (capsLck || shift) { TYPE("W"); } else { TYPE("w"); } break;
            case GLFW_KEY_X: if (capsLck || shift) { TYPE("X"); } else { TYPE("x"); } break;

            case GLFW_KEY_Y:
                if (ctrl) {
                    processCBCommand(BASIC_CMD(CommandType::Redo));
                } else {
                    if (capsLck || shift) { TYPE("Y"); } else { TYPE("y"); }
                }
                break;

            case GLFW_KEY_Z:
                if (ctrl) {
                    processCBCommand(BASIC_CMD(CommandType::Undo));
                } else {
                    if (capsLck || shift) { TYPE("Z"); } else { TYPE("z"); }
                }
                break;
            case GLFW_KEY_0: if (shift) { TYPE(")"); } else { TYPE("0"); } break;
            case GLFW_KEY_1: if (shift) { TYPE("!"); } else { TYPE("1"); } break;
            case GLFW_KEY_2: if (shift) { TYPE("@"); } else { TYPE("2"); } break;
            case GLFW_KEY_3: if (shift) { TYPE("#"); } else { TYPE("3"); } break;
            case GLFW_KEY_4: if (shift) { TYPE("$"); } else { TYPE("4"); } break;
            case GLFW_KEY_5: if (shift) { TYPE("%"); } else { TYPE("5"); } break;
            case GLFW_KEY_6: if (shift) { TYPE("^"); } else { TYPE("6"); } break;
            case GLFW_KEY_7: if (shift) { TYPE("&"); } else { TYPE("7"); } break;
            case GLFW_KEY_8: if (shift) { TYPE("*"); } else { TYPE("8"); } break;
            case GLFW_KEY_9: if (shift) { TYPE("("); } else { TYPE("9"); } break;
            case GLFW_KEY_MINUS: if (shift) { TYPE("_"); } else { TYPE("-"); } break;
            case GLFW_KEY_EQUAL: if (shift) { TYPE("+"); } else { TYPE("="); } break;
            case GLFW_KEY_PERIOD: if (shift) { TYPE(">"); } else { TYPE("."); } break;
            case GLFW_KEY_COMMA: if (shift) { TYPE("<"); } else { TYPE(","); } break;
            case GLFW_KEY_SLASH: if (shift) { TYPE("?"); } else { TYPE("/"); } break;
            case GLFW_KEY_BACKSLASH: if (shift) { TYPE("|"); } else { TYPE("\\"); } break;
            case GLFW_KEY_LEFT_BRACKET: if (shift) { TYPE("{"); } else { TYPE("["); } break;
            case GLFW_KEY_RIGHT_BRACKET: if (shift) { TYPE("}"); } else { TYPE("]"); } break;
            case GLFW_KEY_SEMICOLON: if (shift) { TYPE(":"); } else { TYPE(";"); } break;
            case GLFW_KEY_APOSTROPHE: if (shift) { TYPE("\""); } else { TYPE("'"); } break;
            case GLFW_KEY_GRAVE_ACCENT: if (shift) { TYPE("~"); } else { TYPE("`"); } break;
            case GLFW_KEY_KP_0: if (numLck) { TYPE("0"); } break;
            case GLFW_KEY_KP_1: if (numLck) { TYPE("1"); } break;
            case GLFW_KEY_KP_2: if (numLck) { TYPE("2"); } break;
            case GLFW_KEY_KP_3: if (numLck) { TYPE("3"); } break;
            case GLFW_KEY_KP_4: if (numLck) { TYPE("4"); } break;
            case GLFW_KEY_KP_5: if (numLck) { TYPE("5"); } break;
            case GLFW_KEY_KP_6: if (numLck) { TYPE("6"); } break;
            case GLFW_KEY_KP_7: if (numLck) { TYPE("7"); } break;
            case GLFW_KEY_KP_8: if (numLck) { TYPE("8"); } break;
            case GLFW_KEY_KP_9: if (numLck) { TYPE("9"); } break;
            case GLFW_KEY_KP_DECIMAL: if (numLck) { TYPE("."); } break;
            case GLFW_KEY_KP_DIVIDE: TYPE("/"); break;
            case GLFW_KEY_KP_MULTIPLY: TYPE("*"); break;
            case GLFW_KEY_KP_SUBTRACT: TYPE("-"); break;
            case GLFW_KEY_KP_ADD: TYPE("+"); break;
            case GLFW_KEY_KP_EQUAL: TYPE("="); break;
            case GLFW_KEY_SPACE: TYPE(" "); break;

            default: {
                // Nothing!
            }
        }
        #undef TYPE
        #undef BASIC_CMD
        #undef INCR_CURSOR
        #undef DECR_CURSOR
        #undef CUR_POS
        #undef UPDATE
        #undef UNSELECT
    }
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
        #define TYPE(c) processCommand(elem, new Command(CommandType::Type, c, elem->extraData->textInputData.cursorPosition)); elem->extraData->textInputData.lastActiveTime = static_cast<float>(glfwGetTime())
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

                    processCommand(elem, new Command(CommandType::Backspace, d, CUR_POS));

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

                    processCommand(elem, new Command(CommandType::Delete, d, CUR_POS));
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
                    processCommand(elem, new Command(CommandType::Redo, "", CUR_POS));
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
                    processCommand(elem, new Command(CommandType::Undo, "", CUR_POS));
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

// NOTE: might not need this since the active content block calculation is done
// with the highlighted content box.
void activateCBFromScreenCoords(float mouseX, float mouseY) {
    ContentTab* tab = &tabs[activeTabIndex];

    for (auto itr = tab->contentBlocks.begin(); itr != tab->contentBlocks.end(); ++itr) {
        float top = (*itr).top - tab->scrollOffsetY;

        if (top <= mouseY && top + (*itr).height >= mouseY) {
            uint32_t index = distance(tab->contentBlocks.begin(), itr);

            tab->activeCBIndex = index;
            tab->cursorCBIndex = index;
            return;
        }

        if (top > mouseY) {
            return;
        }
    }
}

inline uint8_t getCBFontSize(ContentBlockType cbType) {
    switch (cbType) {
        case ContentBlockType::Heading1: return 48;
        case ContentBlockType::Heading2: return 40;
        case ContentBlockType::Heading3: return 32;
        case ContentBlockType::Heading4: return 24;
        case ContentBlockType::Heading5: return 16;
        case ContentBlockType::Heading6: return 12;
        case ContentBlockType::Paragraph: return 16;
        case ContentBlockType::Title:  return 48;
    }

    return 16;
}

SelectionData getCBTextCursor(glm::vec2 &resolution) {
    ContentTab* tab = &tabs[activeTabIndex];
    ContentBlock* cb = &tab->contentBlocks[tab->cursorCBIndex];

    RopeTraverser rt(cb->rope);

    uint8_t fontSize = getCBFontSize(cb->type);

    float x = (*cb).left;
    float y = (*cb).top - tab->scrollOffsetY;

    if (resolution.x >= CONTENT_BREAKPOINT) {
        x += contentLeft;
    } else {
        x += CONTENT_HORIZONTAL_MARGIN;
    }

    float xpos = x;
    float yNet = y;

    uint32_t index = 0;
    uint32_t row = 0;
    uint32_t col = 0;
    uint16_t h = static_cast<uint16_t>(static_cast<uint16_t>(fontSize) * glyphDescender);
    uint32_t i = 0;

    if (tab->cursorPosition == 0) {
        return SelectionData {
            .cursorPosition = i,
            .row = 0,
            .col = 0,
            .x = xpos,
            .y = yNet + static_cast<float>(fontSize) - static_cast<float>(fontSize) * glyphDescender,
            .height = static_cast<uint16_t>(fontSize - h),
        };
    }

    while (rt.hasNext()) {
        Rope* r = rt.next();

        for (const char c : r->text) {
            Character* ch = &characters[(int)c];

            float aspectRatio = static_cast<float>(ch->width) / static_cast<float>(ch->height);

            float fontHeightRatio = static_cast<float>(fontSize) / DEFAULT_FONT_SIZE;

            float glyphHeight = static_cast<float>(ch->height) * fontHeightRatio;
            float glyphWidth = glyphHeight * aspectRatio;

            if (glyphWidth != glyphWidth) {
                if (ch->width != 0) {
                    glyphWidth = static_cast<float>(ch->width) / static_cast<float>(fontSize);
                } else {
                    glyphWidth = 0.0f;
                }
            }

            float widthRatio = glyphWidth / ch->width;
            float heightRatio = glyphHeight / ch->height;

            float bearingTop = ch->bearingTop * heightRatio;
            float bearingLeft = ch->bearingLeft * widthRatio;

            if (bearingLeft != bearingLeft) {
                bearingLeft = 0.0f;
            }

            if (bearingTop != bearingTop) {
                bearingTop = 0.0f;
            }

            xpos += bearingLeft;

            float ypos = yNet + static_cast<float>(fontSize);

            if (
                contentContainer->width < 800.0f
                && xpos + glyphWidth > contentContainer->width - CONTENT_HORIZONTAL_MARGIN
                || xpos + glyphWidth > contentRight
            ) {
                yNet += static_cast<float>(fontSize) - static_cast<float>(fontSize) * glyphDescender;
                xpos = x;
                ypos = yNet + static_cast<float>(fontSize);
                ++row;
                col = 0;
            }

            if (i == tab->cursorPosition) {
                cout << "\nDEBUG: Cursor position: " << tab->cursorPosition << " found";

                return SelectionData {
                    .cursorPosition = i,
                    .row = row,
                    .col = col,
                    .x = xpos,
                    .y = ypos - fontSize * glyphDescender,
                    .height = static_cast<uint16_t>(fontSize - h),
                };
            }

            ++i;
            xpos += (static_cast<uint32_t>(ch->advance * fontHeightRatio) >> 6) - bearingLeft;
        }
    }

    if (tab->cursorPosition == cb->rope->length()) {
        return SelectionData {
            .cursorPosition = i,
            .row = row,
            .col = col,
            .x = xpos,
            .y = yNet + static_cast<float>(fontSize) - static_cast<float>(fontSize) * glyphDescender,
            .height = static_cast<uint16_t>(fontSize - h),
        };
    }

    return SelectionData { 0, 0, 0 };
}

SelectionData getCBTextCursorFromScreenCoords(float mouseX, float mouseY, glm::vec2 resolution) {
    ContentTab* tab = &tabs[activeTabIndex];
    ContentBlock* cb = &tab->contentBlocks[tab->activeCBIndex];

    RopeTraverser rt(cb->rope);

    float fontSize = static_cast<float>(getCBFontSize(cb->type));

    float x = (*cb).left;
    float y = (*cb).top - tab->scrollOffsetY;

    if (resolution.x >= CONTENT_BREAKPOINT) {
        x += contentLeft;
    } else {
        x += CONTENT_HORIZONTAL_MARGIN;
    }

    float xpos = x;
    float yNet = y;
    float xprev = x;

    uint32_t index = 0;
    uint32_t row = 0;
    uint32_t col = 0;
    float h = fontSize * glyphDescender;
    float height = fontSize - fontSize * glyphDescender;

    while (rt.hasNext()) {
        Rope* r = rt.next();

        for (const char c : r->text) {
            Character* ch = &characters[(int)c];

            float aspectRatio = static_cast<float>(ch->width) / static_cast<float>(ch->height);

            float fontHeightRatio = fontSize / DEFAULT_FONT_SIZE;

            float glyphHeight = static_cast<float>(ch->height) * fontHeightRatio;
            float glyphWidth = glyphHeight * aspectRatio;

            if (glyphWidth != glyphWidth) {
                if (ch->width != 0) {
                    glyphWidth = static_cast<float>(ch->width) / fontSize;
                } else {
                    glyphWidth = 0.0f;
                }
            }

            float widthRatio = glyphWidth / ch->width;
            float heightRatio = glyphHeight / ch->height;

            float bearingTop = ch->bearingTop * heightRatio;
            float bearingLeft = ch->bearingLeft * widthRatio;

            if (bearingLeft != bearingLeft) {
                bearingLeft = 0.0f;
            }

            if (bearingTop != bearingTop) {
                bearingTop = 0.0f;
            }

            xpos += bearingLeft;

            // float ypos = yNet + glyphHeight - bearingTop + fontSize;
            float ypos = yNet + fontSize;

            if (
                contentContainer->width < 800.0f
                && xpos + glyphWidth > contentContainer->width - CONTENT_HORIZONTAL_MARGIN
                || xpos + glyphWidth > contentRight
            ) {
                yNet += fontSize - fontSize * glyphDescender;
                xpos = x;
                xprev = x;
                ypos = yNet + fontSize;
                ++row;
                col = 0;
            }

            float xpos_plus_w = xpos + glyphWidth;
            float ypos_plus_h = ypos - height;

            #ifdef DEBUG_TEXT
            if (debugTextFirstRenderPrint) {
                cout << "; x: " << xpos << " y: " << ypos << " x + w: "
                    << xpos_plus_w << " y + h: " << ypos_plus_h
                    << " bearingLeft: " << bearingLeft
                    << " bearingTop: " << bearingTop
                    << " widthRatio: " << widthRatio
                    << " heightRatio: " << heightRatio;
            }
            #endif

            if (
                xpos <= mouseX && xpos + (glyphWidth/2) >= mouseX
                && ypos >= mouseY && ypos_plus_h <= mouseY
            ) {
                cout << "\n2: xpos: " << xpos << ", ypos: " << ypos;

                return SelectionData {
                    .cursorPosition = index,
                    .row = row,
                    .col = col,
                    .cursorCBIndex = tab->activeCBIndex,
                    .x = xprev,
                    .y = ypos - fontSize * glyphDescender,
                    .height = static_cast<uint16_t>(fontSize - h),
                };
            } else if (
                xpos + (glyphWidth/2) <= mouseX && xpos_plus_w >= mouseX
                && ypos >= mouseY && ypos_plus_h <= mouseY
            ) {
                cout << "\n3: xpos: " << xpos << ", ypos: " << ypos;

                return SelectionData {
                    .cursorPosition = index + 1,
                    .row = row,
                    .col = col + 1,
                    .cursorCBIndex = tab->activeCBIndex,
                    .x = xpos_plus_w,
                    .y = ypos - fontSize * glyphDescender,
                    .height = static_cast<uint16_t>(fontSize - h),
                };
            } else if (xpos_plus_w >= mouseX && ypos >= mouseY && ypos_plus_h <= mouseY) {
                // cout << "\n4: xpos: " << xpos << ", ypos: " << ypos;

                // if (tab->activeCBIndex == 2) {
                //     cout << "\nArey bhai!!!! h: " << height;
                // }

                return SelectionData {
                    .cursorPosition = index,
                    .row = row,
                    .col = col,
                    .cursorCBIndex = tab->activeCBIndex,
                    .x = xpos,
                    .y = ypos - fontSize * glyphDescender,
                    .height = static_cast<uint16_t>(fontSize - h),
                };
            }

            xprev = xpos_plus_w;
            xpos += (static_cast<uint32_t>(ch->advance * fontHeightRatio) >> 6) - bearingLeft;

            ++index;
            ++col;
        }
    }

    return SelectionData {
        tab->cursorPosition, 0, 0, tab->cursorCBIndex, 0.0f, 0.0f, 0, SelectionCBPosition::None
    };
}

SelectionData getCBTextCursorVertical(ContentTab* tab, bool page, bool up) {
    #define UP_AND_TOP_ROW          up && tab->cursorRow == 0
    #define DOWN_AND_BOTTOM_ROW     !up && tab->cursorRow == cb->rows - 1

    SelectionData sd = {
        tab->cursorPosition,
        tab->cursorRow,
        0,
        tab->cursorCBIndex,
        tab->cursorX,
        tab->cursorY,
        0,
        SelectionCBPosition::Current,
    };

    ContentBlock* cb = &tab->contentBlocks[tab->cursorCBIndex];

    if (page && !up) {
        sd.row = cb->rows - 1;
    }

    if (
        UP_AND_TOP_ROW && tab->activeCBIndex < 2
        || DOWN_AND_BOTTOM_ROW && tab->cursorCBIndex == tab->contentBlocks.size() - 1
    ) {
        return sd;
    }

    if (!page) {
        if (UP_AND_TOP_ROW && tab->cursorCBIndex > 0) {
            sd.cbPos = SelectionCBPosition::Previous;
            cb = &tab->contentBlocks[tab->cursorCBIndex - 1];

            if (cb->rows > 0) {
                sd.row = cb->rows - 1;
            }
        } else if (DOWN_AND_BOTTOM_ROW) {
            sd.cbPos = SelectionCBPosition::Next;
            sd.row = 0;
            cb = &tab->contentBlocks[tab->cursorCBIndex + 1];
        } else if (up) {
            if (tab->cursorRow > 0) {
                sd.row = tab->cursorRow - 1;
            }
        } else {
            sd.row = tab->cursorRow + 1;
        }
    }

    float x = (*cb).left;
    float y = (*cb).top - tab->scrollOffsetY;

    if (resolution.x >= CONTENT_BREAKPOINT) {
        x += contentLeft;
    } else {
        x += CONTENT_HORIZONTAL_MARGIN;
    }

    float xpos = x;
    float yNet = y;
    float xprev = x;

    uint8_t fontSize = getCBFontSize(cb->type);

    uint32_t index = 0;
    uint32_t row = 0;
    uint32_t col = 0;
    float h = fontSize * glyphDescender;

    RopeTraverser rt(cb->rope);

    bool foundRow = false;
    bool visibleRow = y > TAB_BAR_HEIGHT;

    while (rt.hasNext()) {
        Rope* r = rt.next();

        for (const char c : r->text) {
            Character* ch = &characters[(int)c];

            float aspectRatio = static_cast<float>(ch->width) / static_cast<float>(ch->height);

            float fontHeightRatio = static_cast<float>(fontSize) / DEFAULT_FONT_SIZE;

            float glyphHeight = static_cast<float>(ch->height) * fontHeightRatio;
            float glyphWidth = glyphHeight * aspectRatio;

            if (glyphWidth != glyphWidth) {
                if (ch->width != 0) {
                    glyphWidth = static_cast<float>(ch->width) / static_cast<float>(fontSize);
                } else {
                    glyphWidth = 0.0f;
                }
            }

            float widthRatio = glyphWidth / ch->width;
            float heightRatio = glyphHeight / ch->height;

            float bearingTop = ch->bearingTop * heightRatio;
            float bearingLeft = ch->bearingLeft * widthRatio;

            if (bearingLeft != bearingLeft) {
                bearingLeft = 0.0f;
            }

            if (bearingTop != bearingTop) {
                bearingTop = 0.0f;
            }

            xpos += bearingLeft;

            float ypos = yNet + static_cast<float>(fontSize);

            if (
                contentContainer->width < 800.0f
                && xpos + glyphWidth > contentContainer->width - CONTENT_HORIZONTAL_MARGIN
                || xpos + glyphWidth > contentRight
            ) {
                yNet += static_cast<float>(fontSize) - static_cast<float>(fontSize) * glyphDescender;
                xpos = x;
                xprev = x;
                ypos = yNet + static_cast<float>(fontSize);
                ++row;
                col = 0;
                visibleRow = ypos > TAB_BAR_HEIGHT && ypos < windowHeight;
            }

            if (page) {
                if (ypos > windowHeight) {
                    break;
                }

                if (!visibleRow) {
                    continue;
                }
            }

            float xpos_plus_w = xpos + glyphWidth;
            float ypos_plus_h = ypos - glyphHeight; // ypos + h

            #ifdef DEBUG_TEXT
            if (debugTextFirstRenderPrint) {
                cout << "; x: " << xpos << " y: " << ypos << " x + w: "
                    << xpos_plus_w << " y + h: " << ypos_plus_h
                    << " bearingLeft: " << bearingLeft
                    << " bearingTop: " << bearingTop
                    << " widthRatio: " << widthRatio
                    << " heightRatio: " << heightRatio;
            }
            #endif

            if (row == sd.row || page && up) {
                foundRow = true;

                if (xpos <= tab->cursorX && xpos + (glyphWidth/2) >= tab->cursorX) {
                    cout << "\n2: xpos: " << xpos << ", ypos: " << ypos;

                    uint32_t cursorCBIndex;

                    switch (sd.cbPos) {
                        case SelectionCBPosition::Next:
                            cursorCBIndex = tab->cursorCBIndex + 1;
                            break;
                        case SelectionCBPosition::Previous:
                            cursorCBIndex = tab->cursorCBIndex - 1;
                            break;
                        default:
                            cursorCBIndex = tab->cursorCBIndex;
                    }

                    return SelectionData {
                        .cursorPosition = index,
                        .row = row,
                        .col = col,
                        .cursorCBIndex = cursorCBIndex,
                        .x = xprev,
                        .y = ypos - fontSize * glyphDescender,
                        .height = static_cast<uint16_t>(fontSize - h),
                        .cbPos = sd.cbPos,
                    };
                } else if (xpos + (glyphWidth/2) <= tab->cursorX && xpos_plus_w >= tab->cursorX) {
                    cout << "\n3: xpos: " << xpos << ", ypos: " << ypos;

                    uint32_t cursorCBIndex;

                    switch (sd.cbPos) {
                        case SelectionCBPosition::Next:
                            cursorCBIndex = tab->cursorCBIndex + 1;
                            break;
                        case SelectionCBPosition::Previous:
                            cursorCBIndex = tab->cursorCBIndex - 1;
                            break;
                        default:
                            cursorCBIndex = tab->cursorCBIndex;
                    }

                    return SelectionData {
                        .cursorPosition = index + 1,
                        .row = row,
                        .col = col + 1,
                        .cursorCBIndex = cursorCBIndex,
                        .x = xpos_plus_w,
                        .y = ypos - fontSize * glyphDescender,
                        .height = static_cast<uint16_t>(fontSize - h),
                        .cbPos = sd.cbPos,
                    };
                } else if (xpos_plus_w >= tab->cursorX) {
                    cout << "\n4: xpos: " << xpos << ", ypos: " << ypos;

                    uint32_t cursorCBIndex;

                    switch (sd.cbPos) {
                        case SelectionCBPosition::Next:
                            cursorCBIndex = tab->cursorCBIndex + 1;
                            break;
                        case SelectionCBPosition::Previous:
                            cursorCBIndex = tab->cursorCBIndex - 1;
                            break;
                        default:
                            cursorCBIndex = tab->cursorCBIndex;
                    }

                    return SelectionData {
                        .cursorPosition = index,
                        .row = row,
                        .col = col,
                        .cursorCBIndex = cursorCBIndex,
                        .x = xpos,
                        .y = ypos - fontSize * glyphDescender,
                        .height = static_cast<uint16_t>(fontSize - h),
                        .cbPos = sd.cbPos,
                    };
                }
            }

            xprev = xpos_plus_w;
            xpos += (static_cast<uint32_t>(ch->advance * fontHeightRatio) >> 6) - bearingLeft;

            ++index;
            ++col;
        }
    }

    if (foundRow) {
        uint32_t cursorCBIndex;

        switch (sd.cbPos) {
            case SelectionCBPosition::Next:
                cursorCBIndex = tab->cursorCBIndex + 1;
                break;
            case SelectionCBPosition::Previous:
                cursorCBIndex = tab->cursorCBIndex - 1;
                break;
            default:
                cursorCBIndex = tab->cursorCBIndex;
        }

        return SelectionData {
            .cursorPosition = index,
            .row = sd.row,
            .col = col,
            .cursorCBIndex = cursorCBIndex,
            .x = xpos,
            .y = yNet + static_cast<float>(fontSize) - fontSize * glyphDescender,
            .height = static_cast<uint16_t>(fontSize - h),
            .cbPos = sd.cbPos,
        };
    }

    sd.cbPos = SelectionCBPosition::None;

    return sd;

    #undef UP_AND_TOP_ROW
    #undef DOWN_AND_BOTTOM_ROW
}

vector<ContentBlock>::iterator getFirstVisibleCB(ContentTab* tab) {
    vector<ContentBlock>::reverse_iterator cbIter = tab->contentBlocks.rbegin()
        + (tab->contentBlocks.size() - tab->cursorCBIndex);

    vector<ContentBlock>::reverse_iterator lastVisibleIter = tab->contentBlocks.rend();

    for (; cbIter != tab->contentBlocks.rend(); ++cbIter) {
        float top = (*cbIter).top - tab->scrollOffsetY;

        if (
            top > TAB_BAR_HEIGHT && top < windowHeight
            || top < TAB_BAR_HEIGHT && top + (*cbIter).height + TAB_BAR_HEIGHT > windowHeight
        ) {
            cout << "\nContent block \"" << (*cbIter).rope->getString().substr(0, 16) << " is visible";
            lastVisibleIter = cbIter;
        }
    }

    return lastVisibleIter.base();
}

vector<ContentBlock>::iterator getLastVisibleCB(ContentTab* tab) {
    vector<ContentBlock>::iterator cbIter = tab->contentBlocks.begin() + tab->cursorCBIndex;

    vector<ContentBlock>::iterator lastVisibleIter = tab->contentBlocks.end();

    for (; cbIter != tab->contentBlocks.end(); ++cbIter) {
        float top = (*cbIter).top - tab->scrollOffsetY;

        if (
            top > TAB_BAR_HEIGHT && top < windowHeight
            || top < TAB_BAR_HEIGHT && top + (*cbIter).height + TAB_BAR_HEIGHT > windowHeight
        ) {
            cout << "\nContent block \"" << (*cbIter).rope->getString().substr(0, 16) << " is visible";
            lastVisibleIter = cbIter;
        }
    }

    return lastVisibleIter;
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

// TODO: reimpl for content block
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
    // Rope* rope = nullptr;

    glm::vec4 backgroundColor = TRANSPARENT;
    glm::vec4 hoverColor = TRANSPARENT;
    glm::vec4 color = darkTheme ? WHITE : BLACK;
    uint8_t borderRadius = 0;
    UIPositioning positioning = UIPositioning::Absolute;

    uint8_t fontSize = 16;

    UIPreferences* pref = nullptr;

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

            break;

        case UI_TEXT_AREA:
            // rope = new Rope();
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

        "",
        // nullptr,

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
    };
}

void insertContentBlock(uint32_t tabIndex, ContentBlockType type, string text, uint32_t index) {
    if (tabIndex >= tabs.size()) {
        return;
    }

    ContentTab* tab = &tabs[tabIndex];

    ContentBlock cb = ContentBlock {
        type,
        CONTENT_BLOCK_REDRAW | CONTENT_BLOCK_VISIBLE,
        0.0f,
        0.0f,
        0.0f,
        0,
        new Rope(text),
    };

    auto itr = tab->contentBlocks.begin() + index;

    tab->contentBlocks.insert(itr, cb);
}

void appendContentBlock(uint32_t tabIndex, ContentBlockType type, string text) {
    if (tabIndex >= tabs.size()) {
        return;
    }

    tabs[tabIndex].contentBlocks.push_back(ContentBlock {
        type,
        CONTENT_BLOCK_REDRAW | CONTENT_BLOCK_VISIBLE,
        0.0f,
        0.0f,
        0.0f,
        0,
        new Rope(text),
    });
}

void moveContentBlock(uint32_t tabIndex, uint32_t newIndex, uint32_t oldIndex) {
    if (tabIndex >= tabs.size()) {
        cerr << "\nError: Invalid tab!";
        return;
    }

    if (newIndex == oldIndex || newIndex == 0 || oldIndex == 0) {
        return;
    }

    ContentTab* tab = &tabs[tabIndex];

    if (abs(static_cast<int64_t>(newIndex) - static_cast<int64_t>(oldIndex)) == 1) {
        ContentBlock temp = tab->contentBlocks[newIndex];
        tab->contentBlocks[newIndex] = tab->contentBlocks[oldIndex];
        tab->contentBlocks[oldIndex] = temp;
        return;
    }

    if (newIndex < oldIndex) {
        ContentBlock temp = tab->contentBlocks[newIndex];
        tab->contentBlocks[newIndex] = tab->contentBlocks[oldIndex];

        for (
            auto itr = tab->contentBlocks.begin() + newIndex + 1;
            itr != tab->contentBlocks.begin() + oldIndex + 1;
            ++itr
        ) {
            ContentBlock temp2 = *itr;
            *itr = temp;
            temp = temp2;
        }

        return;
    }

    ContentBlock temp = tab->contentBlocks[oldIndex];
    tab->contentBlocks[oldIndex] = tab->contentBlocks[newIndex];

    for (
        auto itr = tab->contentBlocks.begin() + oldIndex + 1;
        itr != tab->contentBlocks.begin() + newIndex + 1;
        ++itr
    ) {
        ContentBlock temp2 = *itr;
        *itr = temp;
        temp = temp2;
    }

    #ifdef DEBUG_CONTENT_BLOCKS
        printContentBlocks(activeTabIndex);
    #endif
}

void newTab(string title) {
    vector<ContentBlock> content;
    vector<ContentCommand> commands;
    vector<ContentCommandExt> extCommands;

    commands.reserve(8);

    extCommands.reserve(4);

    content.push_back(ContentBlock {
        ContentBlockType::Title,
        CONTENT_BLOCK_REDRAW | CONTENT_BLOCK_VISIBLE,
        0.0f, 0.0f, 0.0f,
        0,
        new Rope(title),
    });

    uint32_t vao[4];
    uint32_t vbo[4];

    float vertices[] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };

    for (int i=0; i<4; ++i) {
        SETUP_BUFFERS(vao[i], vbo[i], vertices);
    }

    tabs.push_back(ContentTab {
        false, false, false, false, false, false, false, false,
        0.0f, 0.0f, 0.0f, 0.0f,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        8, vao[0], vbo[0], vao[1], vbo[1], vao[2], vbo[2], vao[3], vbo[3],
        commands, extCommands, content
    });

    activeTabIndex = tabs.size() - 1;
}

void closeTab(uint32_t index) {
    ContentTab* tab = &tabs[activeTabIndex];

    if (tab->selVAO != 0) {
        glDeleteVertexArrays(1, &tab->selVAO);
    }

    if (tab->selVBO != 0) {
        glDeleteBuffers(1, &tab->selVBO);
    }

    if (tab->cursorVAO != 0) {
        glDeleteVertexArrays(1, &tab->cursorVAO);
    }

    if (tab->cursorVBO != 0) {
        glDeleteBuffers(1, &tab->cursorVBO);
    }

    if (tab->scrollbarVAO != 0) {
        glDeleteVertexArrays(1, &tab->scrollbarVAO);
    }

    if (tab->scrollbarVAO != 0) {
        glDeleteBuffers(1, &tab->scrollbarVAO);
    }

    if (tab->scrollThumbVAO != 0) {
        glDeleteVertexArrays(1, &tab->scrollThumbVAO);
    }

    if (tab->scrollThumbVAO != 0) {
        glDeleteBuffers(1, &tab->scrollThumbVAO);
    }

    for (auto cbIter = tab->contentBlocks.begin(); cbIter != tab->contentBlocks.end(); ++cbIter) {
        delete (*cbIter).rope;
    }

    auto itr = tabs.begin() + index;

    tabs.erase(itr, itr + 1);
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

            if (!tabs[activeTabIndex].contentBlocks.empty()) {
                reDrawAllContentBlocks();

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

            if (SCROLL_DATA.verticalScrollBarVBO != 0) {
                glDeleteBuffers(1, &SCROLL_DATA.verticalScrollBarVBO);
            }

            SETUP_BUFFERS(SCROLL_DATA.verticalScrollBarVAO, SCROLL_DATA.verticalScrollBarVBO, vertices);
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

            if (SCROLL_DATA.verticalScrollThumbVBO != 0) {
                glDeleteBuffers(1, &SCROLL_DATA.verticalScrollThumbVBO);
            }

            SETUP_BUFFERS(SCROLL_DATA.verticalScrollThumbVAO, SCROLL_DATA.verticalScrollThumbVBO, vertices);
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

            if (SCROLL_DATA.horizontalScrollBarVBO != 0) {
                glDeleteBuffers(1, &SCROLL_DATA.horizontalScrollBarVBO);
            }

            SETUP_BUFFERS(SCROLL_DATA.horizontalScrollBarVAO, SCROLL_DATA.horizontalScrollBarVBO, vertices);
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

            if (SCROLL_DATA.horizontalScrollThumbVBO != 0) {
                glDeleteBuffers(1, &SCROLL_DATA.horizontalScrollThumbVBO);
            }

            SETUP_BUFFERS(SCROLL_DATA.horizontalScrollThumbVAO, SCROLL_DATA.horizontalScrollThumbVBO, vertices);
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

// NOTE: only for UI_CONTENT_CONTAINER.
inline void renderCBScrollbar() {
    ContentTab* tab = &tabs[activeTabIndex];

    bool update = !tab->scrollbarUpdated;

    // WARNING: side effects!
    float top, left, bottom, right, thumbStart, thumbEnd;

    if (update) {
        // WARNING: side effects!
        getCBScrollbarDimensions(
            tab, top, right, bottom, left,
            thumbStart, thumbEnd
        );
    }

    #ifdef DEBUG_SCROLL_AREA
        cout << "\nShowing vertical scrollbar. Element width: " << elem->width;

        cout << "\n--> Vertical scrollbar dimensions. top: "
            << top << ", left: " << left << ", bottom: "
            << bottom << ", right:" << right
            << ", thumbStart: " << thumbStart << ", thumbEnd: " << thumbEnd;
    #endif

    glBindVertexArray(tab->scrollbarVAO);
    glBindBuffer(GL_ARRAY_BUFFER, tab->scrollbarVBO);

    if (update) {
        float vertices[] = {
            right, bottom,
            right, contentContainer->top,
            left, bottom,
            left, contentContainer->top,
        };

        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    }

    glUseProgram(shaders[SHADER_DEFAULT].programId);

    uint32_t colorU = glGetUniformLocation(shaders[SHADER_DEFAULT].programId, "solidColor");

    glm::vec4 color = SCROLL_BAR_TRACK_COLOR;

    if (darkTheme) {
        color = SCROLL_BAR_TRACK_COLOR_DARK;
    }

    glUniform4f(colorU, color.r, color.g, color.b, color.a);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glBindVertexArray(tab->scrollThumbVAO);
    glBindBuffer(GL_ARRAY_BUFFER, tab->scrollThumbVBO);

    if (update) {
        float vertices[] = {
            right, thumbEnd,
            right, thumbStart,
            left, thumbEnd,
            left, thumbStart,
        };

        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    }

    glUseProgram(shaders[SHADER_DEFAULT].programId);

    colorU = glGetUniformLocation(shaders[SHADER_DEFAULT].programId, "solidColor");
    glUniform4f(colorU, 0.6f, 0.6f, 0.6f, 1.0f);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    tab->scrollbarUpdated = true;
}

void updateCBCursor(ContentTab* tab, SelectionData sd) {
    tab->showCursor = true;
    tab->redrawCursor = false;

    if (
        tab->cursorPosition == sd.cursorPosition
        && tab->cursorRow == sd.row
        && tab->cursorCBIndex == sd.cursorCBIndex
    ) {
        return;
    }

    if (tab->updateCursorX) {
        tab->cursorX = static_cast<uint16_t>(sd.x);
        tab->updateCursorX = false;
    }

    tab->cursorY = static_cast<uint16_t>(sd.y) + (static_cast<uint16_t>(sd.height) >> 1);

    if (sd.y - tab->scrollOffsetY > contentContainer->height + contentContainer->top) {
        tab->scrollOffsetY += sd.y - contentContainer->height - contentContainer->top;
        tab->redrawCursor = true;
        highlightedContentBlock.show = false;
        tab->scrollbarUpdated = false;
    } else if (sd.y - sd.height < contentContainer->top && sd.y != 0 && sd.height != 0) {
        tab->scrollOffsetY -= contentContainer->top - sd.y + sd.height;
        tab->redrawCursor = true;
        tab->scrollbarUpdated = false;
    }

    tab->cursorPosition = sd.cursorPosition;
    tab->lastCursorTime = TIME();
    tab->cursorRow = sd.row;
    // tab->cursorX = static_cast<uint16_t>(sd.x);
    tab->cursorCBIndex = sd.cursorCBIndex;

    glBindVertexArray(tab->cursorVAO);
    glBindBuffer(GL_ARRAY_BUFFER, tab->cursorVBO);

    float vertices[] = {
        sd.x + 1.0f, sd.y - sd.height,
        sd.x + 1.0f, sd.y,
        sd.x - 1.0f, sd.y - sd.height,
        sd.x - 1.0f, sd.y,
    };

    cout << "\nSetting cursor dimensions: " << sd.x + 1 << ", "
        << sd.y - sd.height << "; " << sd.x + 1 << ", " << sd.y << "; " << sd.x
        << ", " << sd.y - sd.height << "; " << sd.x << ", " << sd.y;

    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
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

                SETUP_BUFFERS(elem.vao, elem.vbo, vertices);
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
            elem.reDraw = false;

            ContentTab* tab = CURRENT_TAB;

            float width = elem.width;
            float height = elem.height;

            bool showScrollbar = tab->maxHeight > elem.height;

            if (showScrollbar) {
                width -= SCROLL_BAR_THICKNESS;
            }

            glScissor(elem.left, resolution.y - elem.top - elem.height, width, height);
            glEnable(GL_SCISSOR_TEST);

            renderContentBlockHighlight();
            renderCBTextSelection(tab);

            for (
                auto itr = tab->contentBlocks.begin();
                itr != tab->contentBlocks.end();
                ++itr
            ) {
                renderContentBlock(tab, itr, resolution, cursor);
            }

            renderCBMoveMarker();

            if (tab->redrawCursor) {
                SelectionData sd = getCBTextCursor(resolution);

                if (tab->updateCursorX) {
                    tab->cursorX = static_cast<uint16_t>(sd.x);
                    tab->updateCursorX = false;
                }
                tab->cursorY = static_cast<uint16_t>(sd.y) + (static_cast<uint16_t>(sd.height) >> 1);
                tab->cursorRow = sd.row;

                cout << "\nSetting cursor dimensions (redraw): " << sd.x + 1 << ", "
                    << sd.y - sd.height << "; " << sd.x + 1 << ", " << sd.y << "; " << sd.x
                    << ", " << sd.y - sd.height << "; " << sd.x << ", " << sd.y;

                if (tab->showCursor && !tab->textSelected) {
                    float vertices[] = {
                        sd.x + 1.0f, sd.y - sd.height,
                        sd.x + 1.0f, sd.y,
                        sd.x - 1.0f, sd.y - sd.height,
                        sd.x - 1.0f, sd.y,
                    };

                    glBindVertexArray(tab->cursorVAO);
                    glBindBuffer(GL_ARRAY_BUFFER, tab->cursorVBO);
                    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
                }

                tab->redrawCursor = false;
            }

            if (tab->showCursor && !tab->textSelected) {
                glBindVertexArray(tab->cursorVAO);
                glBindBuffer(GL_ARRAY_BUFFER, tab->cursorVBO);

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
                glUniform1f(timeU, TIME() - tab->lastCursorTime);

                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            }

            glDisable(GL_SCISSOR_TEST);

            renderCBScrollbar();
            break;
        }

        case UI_LABEL: {
            glm::vec4 dim;

            if (elem.positioning == UIPositioning::Relative) {
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

                SETUP_BUFFERS(elem.vao, elem.vbo, vertices);
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

                SETUP_BUFFERS(elem.vao, elem.vbo, vertices);
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

                SETUP_BUFFERS(tid->cursorVAO, tid->cursorVBO, vertices);

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

                        SETUP_BUFFERS(tid->textSelectVAO, tid->textSelectVBO, vertices);

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

                SETUP_BUFFERS(elem.vao, elem.vbo, vertices);

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

                SETUP_BUFFERS(tid->cursorVAO, tid->cursorVBO, vertices);

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

inline glm::vec2 getActiveContentDimensions() {
    return glm::vec2 {
        windowWidth - SCROLL_BAR_THICKNESS - CONTENT_BLOCK_DRAGTAB_WIDTH,
        windowHeight - TAB_BAR_HEIGHT
    }; // width, height
}

void renderContentBlock(
    ContentTab* tab,
    vector<ContentBlock>::iterator cb,
    glm::vec2 &resolution,
    glm::vec2 &cursor
) {
    if (!((*cb).flags & CONTENT_BLOCK_VISIBLE)) {
        cout << "\nContent block not visible";
        return;
    }

    uint8_t fontSize = 16;
    float topMargin = 0.0f;

    switch ((*cb).type) {
        case ContentBlockType::Heading1: fontSize = 48; topMargin = CONTENT_BLOCK_MARGIN; break;
        case ContentBlockType::Heading2: fontSize = 40; topMargin = CONTENT_BLOCK_MARGIN; break;
        case ContentBlockType::Heading3: fontSize = 32; topMargin = CONTENT_BLOCK_MARGIN; break;
        case ContentBlockType::Heading4: fontSize = 24; topMargin = CONTENT_BLOCK_MARGIN; break;
        case ContentBlockType::Heading5: fontSize = 16; topMargin = CONTENT_BLOCK_MARGIN; break;
        case ContentBlockType::Heading6: fontSize = 12; topMargin = CONTENT_BLOCK_MARGIN; break;
        case ContentBlockType::Paragraph: fontSize = 16; break;
        case ContentBlockType::Title:  fontSize = 48; topMargin = 5.0f; break;
    }

    if (contentContainer->extraData == nullptr) {
        cout << "\nRender Error: UIElement's parent is NULL!";
        return;
    }

    if (((*cb).flags & CONTENT_BLOCK_REDRAW) != 0) {
        if (cb > tabs[activeTabIndex].contentBlocks.begin() && cb - 1 >= tabs[activeTabIndex].contentBlocks.begin()) {
            auto prev = cb - 1;

            (*cb).top = (*prev).top + (*prev).height + 8.0f;
            (*cb).left = (*prev).left;

            switch ((*prev).type) {
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

            (*cb).top += topMargin;
        } else {
            (*cb).top = contentContainer->top;
            (*cb).left = contentContainer->left;
        }
    }

    float top = (*cb).top - tab->scrollOffsetY;
    float left = (*cb).left;

    if (resolution.x >= CONTENT_BREAKPOINT) {
        left += contentLeft;
    } else {
        left += CONTENT_HORIZONTAL_MARGIN;
    }

    RopeRenderResult res = renderContentRope(
        (*cb).rope,
        glm::vec2(left, top),
        resolution,
        darkTheme ? WHITE : BLACK,
        fontSize,
        contentContainer
    );

    if (((*cb).flags & CONTENT_BLOCK_REDRAW) != 0) {
        (*cb).height = res.height;
        (*cb).rows = res.rows;
        (*cb).flags &= ~(CONTENT_BLOCK_REDRAW);
    }

    if (tab->maxHeight < (*cb).top + (*cb).height) {
        #ifdef DEBUG_SCROLL_AREA
            cout << "\nScroll height less than dim.z " << dim.z;
        #endif

        tab->maxHeight = (*cb).top + (*cb).height;
        tab->scrollbarUpdated = false;
        contentContainer->reDraw = true;
    }
}

void renderContentBlockHighlight() {
    if (!highlightedContentBlock.show) {
        return;
    }

    uint32_t colorU;

    glBindVertexArray(highlightedContentBlock.backgroundVAO);
    glBindBuffer(GL_ARRAY_BUFFER, highlightedContentBlock.backgroundVBO);

    if (highlightedContentBlock.updated) {
        float vertices[] = {
            highlightedContentBlock.right, highlightedContentBlock.top,
            highlightedContentBlock.right, highlightedContentBlock.bottom,
            highlightedContentBlock.left, highlightedContentBlock.top,
            highlightedContentBlock.left, highlightedContentBlock.bottom,
        };

        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    }

    glUseProgram(shaders[SHADER_DEFAULT].programId);

    colorU = glGetUniformLocation(shaders[SHADER_DEFAULT].programId, "solidColor");

    glm::vec4 color = CONTENT_HIGHLIGHT_BG;

    if (darkTheme) {
        color = CONTENT_HIGHLIGHT_BG_DARK;
    }

    glUniform4f(colorU, color.r, color.g, color.b, color.a);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glBindVertexArray(highlightedContentBlock.dragTabVAO);
    glBindBuffer(GL_ARRAY_BUFFER, highlightedContentBlock.dragTabVBO);

    if (highlightedContentBlock.updated) {
        float vertices[] = {
            highlightedContentBlock.left, highlightedContentBlock.top,
            highlightedContentBlock.left, highlightedContentBlock.bottom,
            highlightedContentBlock.left - 20.0f, highlightedContentBlock.top,
            highlightedContentBlock.left - 20.0f, highlightedContentBlock.bottom,
        };

        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    }

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

void renderCBTextSelection(ContentTab* tab) {
    if (!tab->textSelected) {
        return;
    }

    if (tab->redrawSel) {
        vector<float> vertices;

        auto firstIter = tab->contentBlocks.begin() + tab->selStartCBIndex;
        auto lastIter = tab->contentBlocks.begin() + tab->selEndCBIndex + 1;
        bool selectionStarted = false;
        bool selectionFinished = false;

        float startX = 0.0f;
        float startY = 0.0f;
        float startY_h = 0.0f;
        float endX = 0.0f;
        float endY = 0.0f;
        float endY_h = 0.0f;

        cout << "\nRedrawing selection (" << tab->selStartCBIndex << ", " << tab->selEndCBIndex << "): ";

        for (auto cbIter = firstIter; cbIter != lastIter; ++cbIter) {
            RopeTraverser rt((*cbIter).rope);

            float fontSize = static_cast<float>(getCBFontSize((*cbIter).type));
            float fontHeightRatio = fontSize / DEFAULT_FONT_SIZE;

            uint32_t row = 0;
            uint32_t cursorPosition = 0;

            float initialX = (*cbIter).left + CONTENT_BLOCK_DRAGTAB_WIDTH;
            float initialY = (*cbIter).top;

            float xpos = initialX;
            float yNet = initialY;

            while (rt.hasNext()) {
                Rope* rope = rt.next();

                for (char c: rope->text) {
                    Character* ch = &characters[(int)c];

                    float aspectRatio = static_cast<float>(ch->width) / static_cast<float>(ch->height);

                    float glyphHeight = static_cast<float>(ch->height) * fontHeightRatio;
                    float glyphWidth = glyphHeight * aspectRatio;

                    if (glyphWidth != glyphWidth) {
                        if (ch->width != 0) {
                            glyphWidth = static_cast<float>(ch->width) / fontSize;
                        } else {
                            glyphWidth = 0.0f;
                        }
                    }

                    float widthRatio = glyphWidth / ch->width;
                    float heightRatio = glyphHeight / ch->height;

                    float bearingTop = ch->bearingTop * heightRatio;
                    float bearingLeft = ch->bearingLeft * widthRatio;

                    if (bearingLeft != bearingLeft) {
                        bearingLeft = 0.0f;
                    }

                    if (bearingTop != bearingTop) {
                        bearingTop = 0.0f;
                    }

                    xpos += bearingLeft;

                    if (
                        selectionStarted
                        && cursorPosition == tab->selEndCursorPosition
                        && cbIter == lastIter - 1
                    ) {
                        selectionFinished = true;
                        endX = xpos;
                        endY_h = yNet;
                        endY = yNet + fontSize - fontSize * glyphDescender;

                        cout << "\n  (cb: "
                            << distance(tab->contentBlocks.begin(), cbIter)
                            << ", row: " << row << ", cur: " << cursorPosition
                            << ")\n    (" << startX << ", " << startY_h << "), ("
                            << endX << ", " << endY_h << "), ("
                            << endX << ", " << endY << "), ("
                            << startX << ", " << startY_h << ")";

                        vertices.push_back(startX);
                        vertices.push_back(startY_h);
                        vertices.push_back(endX);
                        vertices.push_back(endY_h);
                        vertices.push_back(endX);
                        vertices.push_back(endY);
                        vertices.push_back(endX);
                        vertices.push_back(endY);
                        vertices.push_back(startX);
                        vertices.push_back(startY);
                        vertices.push_back(startX);
                        vertices.push_back(startY_h);
                        break;
                    }

                    if (
                        xpos + glyphWidth > contentContainer->width - CONTENT_HORIZONTAL_MARGIN
                        || xpos + glyphWidth > contentRight
                    ) {
                        if (selectionStarted) {
                            endX = xpos;
                            endY_h = yNet;
                            endY = yNet + fontSize - fontSize * glyphDescender;

                            cout << "\n  (cb: "
                                << distance(tab->contentBlocks.begin(), cbIter)
                                << ", row: " << row << ", cur: " << cursorPosition
                                << ")\n    (" << startX << ", " << startY_h << "), ("
                                << endX << ", " << endY_h << "), ("
                                << endX << ", " << endY << "), ("
                                << startX << ", " << startY_h << ")";

                            vertices.push_back(startX);
                            vertices.push_back(startY_h);
                            vertices.push_back(endX);
                            vertices.push_back(endY_h);
                            vertices.push_back(endX);
                            vertices.push_back(endY);
                            vertices.push_back(endX);
                            vertices.push_back(endY);
                            vertices.push_back(startX);
                            vertices.push_back(startY);
                            vertices.push_back(startX);
                            vertices.push_back(startY_h);
                        }

                        yNet += fontSize - fontSize * glyphDescender;
                        xpos = initialX;
                        row++;

                        startX = xpos;
                        startY_h = yNet;
                        startY = yNet + fontSize - fontSize * glyphDescender;
                    }

                    float ypos = yNet + glyphHeight - bearingTop + fontSize;

                    if (
                        (
                            !selectionStarted
                            && cursorPosition == tab->selStartCursorPosition
                            && cbIter == firstIter
                        )
                        || selectionStarted && cbIter != firstIter && row == 0
                        && cursorPosition == 0
                    ) {
                        selectionStarted = true;

                        cout << "\n  Start (cb: "
                            << distance(tab->contentBlocks.begin(), cbIter)
                            << ", row: " << row << ", cur: " << cursorPosition
                            << "), ";

                        startX = xpos;
                        startY = yNet + fontSize - fontSize * glyphDescender;
                        startY_h = yNet;
                    }

                    xpos += (static_cast<uint32_t>(ch->advance * fontHeightRatio) >> 6) - bearingLeft;
                    ++cursorPosition;
                }

                if (selectionFinished) {
                    break;
                }
            }

            if (selectionFinished) {
                break;
            } else if (selectionStarted) {
                cout << "\n  (cb: "
                    << distance(tab->contentBlocks.begin(), cbIter)
                    << ", row: " << row << ", cur: " << cursorPosition
                    << "), ";

                endX = xpos;
                endY_h = yNet;
                endY = yNet + fontSize - fontSize * glyphDescender;

                vertices.push_back(startX);
                vertices.push_back(startY_h);
                vertices.push_back(endX);
                vertices.push_back(endY_h);
                vertices.push_back(endX);
                vertices.push_back(endY);
                vertices.push_back(endX);
                vertices.push_back(endY);
                vertices.push_back(startX);
                vertices.push_back(startY);
                vertices.push_back(startX);
                vertices.push_back(startY_h);
            }
        }

        if (vertices.empty()) {
            return;
        }

        selectionStarted = false;
        selectionFinished = false;

        if (vertices.size() > tab->selVertexCount) {
            glDeleteVertexArrays(1, &tab->selVAO);
            glDeleteBuffers(1, &tab->selVBO);

            glGenVertexArrays(1, &tab->selVAO);
            glBindVertexArray(tab->selVAO);
            glGenBuffers(1, &tab->selVBO);
            glBindBuffer(GL_ARRAY_BUFFER, tab->selVBO);
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);

            tab->selVertexCount = vertices.size();
        } else {
            for (int i=vertices.size(); i<tab->selVertexCount; ++i) {
                vertices.push_back(0.0f);
            }

            glBindVertexArray(tab->selVAO);
            glBindBuffer(GL_ARRAY_BUFFER, tab->selVBO);
            glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(float), &vertices[0]);
        }

        tab->redrawSel = false;
    }

    glBindVertexArray(tab->selVAO);
    glBindBuffer(GL_ARRAY_BUFFER, tab->selVBO);

    glUseProgram(shaders[SHADER_CB_TEXT_HIGHLIGHT].programId);

    uint32_t resolutionU = glGetUniformLocation(shaders[SHADER_CB_TEXT_HIGHLIGHT].programId, "resolution");
    uint32_t colorU = glGetUniformLocation(shaders[SHADER_CB_TEXT_HIGHLIGHT].programId, "solidColor");
    uint32_t scrollOffsetU = glGetUniformLocation(shaders[SHADER_CB_TEXT_HIGHLIGHT].programId, "scrollOffsetY");

    glUniform2f(resolutionU, resolution.x, resolution.y);
    glUniform1f(scrollOffsetU, tab->scrollOffsetY);

    glm::vec4 textSelColor = darkTheme ? CURSOR_COLOR_DARK : CURSOR_COLOR;

    glUniform4f(colorU, textSelColor.r, textSelColor.g, textSelColor.b, 0.25f);

    glDrawArrays(GL_TRIANGLES, 0, tab->selVertexCount/2);
}

void contentBlockMouseDownCallback(UIElement* elem, float mouseX, float mouseY, glm::vec2 res) {
    #ifdef DEBUG_CONTENT_BLOCKS
        cout << "\nDEBUG: Mouse down on content block!";
    #endif

    contentContainer->active = true;
    activeElement = contentContainer;

    activateCBFromScreenCoords(mouseX, mouseY);

    SelectionData sd = getCBTextCursorFromScreenCoords(mouseX, mouseY, res);

    #ifdef DEBUG_TEXT_SELECTION
        cout << "\nSelectionData: mouseX: " << mouseX << ", mouseY: " << mouseY
            << " cursorPosition: " << sd.cursorPosition << ", row: " << sd.row
            << ", col: " << sd.col << ", cursorCBIndex: " << sd.cursorCBIndex;
    #endif

    ContentTab* tab = &tabs[activeTabIndex];

    tab->updateCursorX = true;

    if (
        tab->textSelected
        && (
            sd.cursorCBIndex == tab->cursorCBIndex
            && (
                sd.cursorPosition < tab->selStartCursorPosition
                || sd.cursorPosition > tab->selEndCursorPosition
           )
           || sd.cursorCBIndex != tab->cursorCBIndex
       )
    ) {
        tab->textSelected = false;
        tab->selStartCBIndex = 0;
        tab->selStartCursorPosition = 0;
        tab->selEndCBIndex = 0;
        tab->selEndCursorPosition = 0;
        tab->redrawSel = true;
    }

    tab->initialCursorPosition = sd.cursorPosition;
    updateCBCursor(CURRENT_TAB, sd);

    tab->cbTextMouseDown = true;
}

void contentBlockMouseUpCallback(UIElement* elem, float mouseX, float mouseY, glm::vec2 res) {
    ContentTab* tab = &tabs[activeTabIndex];

    SelectionData sd = getCBTextCursorFromScreenCoords(mouseX, mouseY, res);

    cout << "\nDEBUG: contentBlockMouseUpCallback: " << "\n    selStartCBIndex: " << tab->selStartCBIndex
        << "\n    selEndCBIndex: " << tab->selEndCBIndex
        << "\n    selStartCursorPosition: " << tab->selStartCursorPosition
        << "\n    selEndCursorPosition: " << tab->selEndCursorPosition
        << "\n    cursorCBIndex: " << tab->cursorCBIndex
        << "\n    cursorPosition: " << tab->cursorPosition
        << "\n    textSelected: " << tab->textSelected
        << "\n    updateCursorX: " << tab->updateCursorX
        << "\n    redrawSel: " << tab->redrawSel
        << "\n    selVertexCount: " << tab->selVertexCount;

    tab->cbTextMouseDown = false;

    if (sd.cursorPosition == tab->initialCursorPosition) {
        tab->textSelected = false;
        tab->selStartCursorPosition = 0;
        tab->selStartCBIndex = 0;
        tab->selEndCursorPosition = 0;
        tab->selEndCBIndex = 0;
    }
}

void renderCBMoveMarker() {
    #define GET_TOP(block) block->top - 1.5f - contentContainer->extraData->scrollData.yOffset

    if (
        highlightedContentBlock.mouseDown
        && cbMove.show
    ) {
        if (cbMove.updated) {
            if (cbMove.colorU == 0) {
                cbMove.colorU = glGetUniformLocation(shaders[SHADER_DEFAULT].programId, "solidColor");
            }

            float top;

            if (cbMove.targetIndex == 0) {
                uint32_t index = tabs[activeTabIndex].contentBlocks.size() - 1;
                ContentBlock* last = &tabs[activeTabIndex].contentBlocks[index];

                top = GET_TOP(last) + last->height;
            } else {
                ContentBlock* last = &tabs[activeTabIndex].contentBlocks[cbMove.targetIndex];
                top = GET_TOP(last);
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
                if (cbMove.vbo) {
                    glDeleteBuffers(1, &cbMove.vbo);
                    cbMove.vbo = 0;
                }

                SETUP_BUFFERS(cbMove.vao, cbMove.vbo, vertices);
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
    cbMove.targetIndex = 0;
}

void reDrawAllContentBlocks() {
    for (
        auto itr = tabs[activeTabIndex].contentBlocks.begin();
        itr != tabs[activeTabIndex].contentBlocks.end();
        ++itr
    ) {
        (*itr).flags |= CONTENT_BLOCK_REDRAW;
    }
}

void reDrawCBsFrom(uint32_t i, uint32_t limit) {
    uint32_t j = 0;

    for (
        auto itr = tabs[activeTabIndex].contentBlocks.begin() + i;
        itr != tabs[activeTabIndex].contentBlocks.end() && j < limit;
        ++itr, ++j
    ) {
        (*itr).flags |= CONTENT_BLOCK_REDRAW;
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
        case CommandType::Type: {
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

        case CommandType::Delete: {
            element->text.erase(cmd->cursorPos, 1);
            CUR_POS = cmd->cursorPos;
            break;
        }

        case CommandType::Backspace: {
            element->text.erase(cmd->cursorPos-1, 1);
            CUR_POS = cmd->cursorPos - 1;
            break;
        }

        case CommandType::Replace: {
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
        case CommandType::Type: {
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
        case CommandType::Backspace: {
            element->text.insert(currentCmd->cursorPos-1, currentCmd->str);
            moveToPrevCommand(tid->cmdList);
            CUR_POS = currentCmd->cursorPos;
            break;
        }
        case CommandType::Delete: {
            element->text.insert(currentCmd->cursorPos, currentCmd->str);
            moveToPrevCommand(tid->cmdList);
            CUR_POS = currentCmd->cursorPos;
            break;
        }

        case CommandType::Replace: {
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

void processCBCommand(ContentCommand cmd) {
    ContentTab* tab = &tabs[activeTabIndex];
    ContentBlock* cb = &tab->contentBlocks[tab->cursorCBIndex];

    #ifdef DEBUG_TEXT_INPUT
        cout << "\nProcessing command \"";

        switch (cmd.type) {
            case CommandType::None: cout << "None"; break;
            case CommandType::Type: cout << "Type"; break;
            case CommandType::NewLine: cout << "NewLine"; break;
            case CommandType::Delete: cout << "Delete"; break;
            case CommandType::DeleteSelected: cout << "DeleteSelected"; break;
            case CommandType::DeleteMergeCB: cout << "DeleteMergeCB"; break;
            case CommandType::Backspace: cout << "Backspace"; break;
            case CommandType::Undo: cout << "Undo"; break;
            case CommandType::Redo: cout << "Redo"; break;
            case CommandType::Replace: cout << "Replace"; break;
        }

        cout << "\"\n  -> str: \"" << cmd.str
            << "\", startCBIndex: " << cmd.startCBIndex
            << ", startCursorPos: " << cmd.startCursorPos;

        if (cmd.ext) {
            cout << ", endCBIndex: " << tab->extCommands[cmd.extIndex].endCBIndex
                << ", endCursorPos: " << tab->extCommands[cmd.extIndex].endCursorPos;
        }
    #endif

    bool append = false;

    switch (cmd.type) {
        case CommandType::Type:
            if (cmd.str != "") {
                // if (tab->cursorPosition == cb->rope->length()) {
                //     cb->rope->append(cmd.str);
                // } else {
                    cb->rope->insert(tab->cursorPosition, cmd.str);
                // }

                vector<ContentBlock>::iterator itr = tab->contentBlocks.begin() + tab->cursorCBIndex;

                (*itr).flags = (*itr).flags | CONTENT_BLOCK_REDRAW;

                if (!tab->commands.empty()) {
                    if (tab->commands.back().type == CommandType::Type) {
                        auto itr = tab->commands.end() - 1;
                        (*itr).str += cmd.str;
                    } else {
                        append = true;
                    }
                } else {
                    append = true;
                }
            }
            break;

        case CommandType::NewLine:
            if (tab->cursorPosition == cb->rope->length() || tab->cursorPosition == 0) {
                if (tab->cursorPosition != 0) {
                    ++tab->cursorCBIndex;
                }

                tab->contentBlocks.insert(
                    tab->contentBlocks.begin() + tab->cursorCBIndex,
                    ContentBlock {
                        cb->type,
                        CONTENT_BLOCK_REDRAW | CONTENT_BLOCK_VISIBLE,
                        0.0f, 0.0f, 0.0f,
                        0,
                        new Rope()
                    }
                );
            } else if (cb->rope != nullptr) {
                Rope* result = cb->rope->split(tab->cursorPosition);

                if (result != nullptr) {
                    Rope* rope = result->right;

                    if (rope != nullptr) {
                        ++tab->cursorCBIndex;

                        tab->contentBlocks.insert(
                            tab->contentBlocks.begin() + tab->cursorCBIndex,
                            ContentBlock {
                                cb->type,
                                CONTENT_BLOCK_REDRAW | CONTENT_BLOCK_VISIBLE,
                                0.0f, 0.0f, 0.0f,
                                0,
                                rope
                        });

                        if (tab->cursorCBIndex == 0) {
                            (*tab->contentBlocks.begin()).rope->pick(rope);
                        } else {
                            (*(tab->contentBlocks.begin() + tab->cursorCBIndex - 1)).rope->pick(rope);
                        }
                    }
                }
            } else {
                tab->contentBlocks.insert(
                    tab->contentBlocks.begin() + tab->cursorCBIndex,
                    ContentBlock {
                        cb->type,
                        CONTENT_BLOCK_REDRAW | CONTENT_BLOCK_VISIBLE,
                        0.0f, 0.0f, 0.0f,
                        0,
                        nullptr
                    });
            }

            tab->cursorPosition = 0;

            for (
                auto itr = tab->contentBlocks.begin() + tab->cursorCBIndex - 1;
                itr != tab->contentBlocks.end();
                ++itr
            ) {
                (*itr).flags |= CONTENT_BLOCK_REDRAW;
            }
            append = true;
            break;

        case CommandType::Backspace: 
            if (tab->cursorPosition > 0) {
                cb->rope->remove(tab->cursorPosition - 1, 1);
            }

            if (
                !tab->commands.empty()
                && tab->commands.back().type == CommandType::Backspace
                && tab->commands.back().startCursorPos == tab->cursorPosition + 1
            ) {
                tab->commands.back().str = cmd.str + tab->commands.back().str;
                --tab->commands.back().startCursorPos;
            } else {
                append = true;
            }
            break;

        case CommandType::Delete:
            if (tab->cursorPosition < cb->rope->length()) {
                cb->rope->remove(tab->cursorPosition, 1);
            }

            if (
                !tab->commands.empty()
                && tab->commands.back().type == CommandType::Delete
                && tab->commands.back().startCursorPos == tab->cursorPosition
            ) {
                tab->commands.back().str += cmd.str;
            } else {
                append = true;
            }
            break;

        case CommandType::DeleteMergeCB:
            if (
                cmd.ext
                && cmd.extIndex < tab->extCommands.size()
                && !tab->extCommands[cmd.extIndex].deletedBlocks.empty()
                && cmd.startCBIndex != 0
                && cmd.startCBIndex + 1 < tab->contentBlocks.size()
            ) {
                Rope* nextRope = tab->contentBlocks[cmd.startCBIndex + 1].rope;

                tab->contentBlocks[cmd.startCBIndex].rope->append(nextRope->getString());

                tab->contentBlocks.erase(
                    tab->contentBlocks.begin() + cmd.startCBIndex + 1,
                    tab->contentBlocks.begin() + cmd.startCBIndex + 2
                );

                for (
                    auto cbIter = tab->contentBlocks.begin() + cmd.startCBIndex;
                    cbIter != tab->contentBlocks.end() && distance(tab->contentBlocks.begin(), cbIter) < 64;
                    ++cbIter
                ) {
                    (*cbIter).flags |= CONTENT_BLOCK_REDRAW;
                }

                append = true;
            }
            break;

        case CommandType::DeleteSelected: {
            #define START tab->contentBlocks.begin() + cmd.startCBIndex
            #define END   tab->contentBlocks.begin() + tab->extCommands.back().endCBIndex

            uint32_t endCBIndex = tab->extCommands.back().endCBIndex;
            uint32_t endCursorPos = tab->extCommands.back().endCursorPos;

            vector<ContentBlock>::iterator cbDeleteStart;
            vector<ContentBlock>::iterator cbDeleteEnd;

            bool deleteLastCB = false;

            for (auto cbIter = START; cbIter != END + 1; ++cbIter) {
                if (cbIter == START) {
                    if (
                        cmd.startCursorPos == 0
                        && (
                            cmd.startCBIndex != endCBIndex
                            || endCursorPos == (*cbIter).rope->length()
                       )
                    ) {
                        tab->extCommands.back().deletedBlocks.push_back(*cbIter);
                    } else {
                        tab->extCommands.back().deletedBlocks.push_back(ContentBlock {
                            (*cbIter).type,
                            0, 0.0f, 0.0f, 0.0f, 0,
                            new Rope(
                                (*cbIter).rope->getString()
                                    .substr(0, cmd.startCursorPos)
                            ),
                        });
                        (*cbIter).rope->remove(cmd.startCursorPos, (*cbIter).rope->length() - cmd.startCursorPos - 1);
                    }

                    cbDeleteStart = cbIter;
                    continue;
                }

                if (cbIter == END) {
                    if (endCursorPos == (*cbIter).rope->length() && cmd.startCBIndex != endCBIndex) {
                        deleteLastCB = true;
                        tab->extCommands.back().deletedBlocks.push_back(*cbIter);
                        tab->contentBlocks.erase(cbIter);
                    } else {
                        tab->extCommands.back().deletedBlocks.push_back(ContentBlock {
                            (*cbIter).type,
                            0, 0.0f, 0.0f, 0.0f, 0,
                            new Rope(
                                (*cbIter).rope->getString()
                                    .substr(
                                        cmd.startCursorPos,
                                        (*cbIter).rope->length() - cmd.startCursorPos
                                    )
                            ),
                        });
                        (*cbIter).rope->remove(0, endCursorPos);
                    }

                    cbDeleteEnd = cbIter;
                    continue;
                }

                tab->extCommands.back().deletedBlocks.push_back(*cbIter);
            }

            if (cmd.startCBIndex != endCBIndex && cmd.startCursorPos == 0 && deleteLastCB) {
                tab->contentBlocks.erase(cbDeleteStart, cbDeleteEnd + 1);
                tab->contentBlocks.insert(tab->contentBlocks.begin() + cmd.startCBIndex, ContentBlock {
                    ContentBlockType::Paragraph,
                    CONTENT_BLOCK_VISIBLE | CONTENT_BLOCK_REDRAW, 0.0f, 0.0f, 0.0f, 0, new Rope()
                });
            } else {
                (*cbDeleteStart).rope->append((*cbDeleteEnd).rope->getString());
                tab->contentBlocks.erase(cbDeleteStart + 1, cbDeleteEnd + 1);
            }

            reDrawCBsFrom(cmd.startCBIndex, 100);

            #undef START
            #undef END
            break;
        }
    }

    if (append) {
        if (!tab->commands.empty()) {
            ++tab->currentCommand;
        }

        tab->commands.push_back(cmd);
    }

    #ifdef DEBUG_COMMANDS
        cout << "\nDEBUG: Commands:";

        for (ContentCommand cmd : tab->commands) {
            cout << "\n  -> ";
            printContentCommand(&cmd);
        }
    #endif
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
            case CommandType::None: break;
            case CommandType::Type: {
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

            case CommandType::NewLine: {
                break;
            }

            case CommandType::Delete: {
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

            case CommandType::Backspace: {
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

            case CommandType::Undo:
                processUndo(elem);
                break;

            case CommandType::Redo:
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

void getCBScrollbarDimensions(
    ContentTab* tab,
    float& top, float& right, float& bottom, float& left,
    float& thumbStart, float& thumbEnd
) {
    float height = contentContainer->height;
    top = contentContainer->top;
    right = contentContainer->left + contentContainer->width;
    left = right - SCROLL_BAR_THICKNESS;
    bottom = top + height;

    thumbStart = top + tab->scrollOffsetY / tab->maxHeight * height;
    thumbEnd = thumbStart + contentContainer->height / tab->maxHeight * height;

    if (thumbEnd > top + height) {
        float barWidth = thumbEnd - thumbStart;

        thumbEnd = top + height;
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

void printContentBlocks(uint32_t tabIndex) {
    if (tabIndex >= tabs.size()) {
        cerr << "\nError: Invalid tab index.";
        return;
    }

    cout << "\nContent Blocks: ";

    for (auto itr = tabs[tabIndex].contentBlocks.begin(); itr != tabs[tabIndex].contentBlocks.end(); ++itr) {
        cout << "\n  ";
        switch ((*itr).type) {
            case ContentBlockType::Title: cout << "Title"; break;
            case ContentBlockType::Heading1: cout << "Heading1"; break;
            case ContentBlockType::Heading2: cout << "Heading2"; break;
            case ContentBlockType::Heading3: cout << "Heading3"; break;
            case ContentBlockType::Heading4: cout << "Heading4"; break;
            case ContentBlockType::Heading5: cout << "Heading5"; break;
            case ContentBlockType::Heading6: cout << "Heading6"; break;
            case ContentBlockType::Paragraph: cout << "Paragraph"; break;
        }

        cout << " -> " << (*itr).rope->text.substr(0, 10);
    }
}

#endif

