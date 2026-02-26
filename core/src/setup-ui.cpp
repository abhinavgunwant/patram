#include <iostream>
#include <thread>
#include <chrono>
#include <stack>

#include "include/setup-ui.hpp"
#include "GLFW/glfw3.h"
#include "include/window-utils.hpp"
#include "text.hpp"
#include "files.hpp"

#define ROOT_BG             glm::vec4(1.0, 1.0, 1.0, 1.0)
#define TOOL_BAR_BG         glm::vec4(0.93, 0.84, 0.85, 1.0)
#define ROOT_BG_DARK        glm::vec4(0.10, 0.09, 0.09, 1.0)
#define TOOL_BAR_BG_DARK    glm::vec4(0.19, 0.17, 0.17, 1.0)

#define DEBUG_SCROLL

using namespace std;

GLFWwindow* window;
UIElement rootContainer;
bool minimized = false;
bool focused = true;
bool mouse1Down = false;
bool cursorShouldUpdate = false;
bool showFps = false;

GLFWcursor* textCursor;

UIElement elements[16];
UIElement* activeElement = nullptr;

uint16_t windowHeight = WINDOW_HEIGHT;
uint16_t windowWidth = WINDOW_WIDTH;
glm::vec2 resolution = glm::vec2(windowWidth, windowHeight);
float mouseX = 25000;
float mouseY = 25000;
float mouseDownX = 25000;
float mouseDownY = 25000;
float cursorUpdateTime = 0.0;

void updateCursorShape() {
    if (cursorShouldUpdate) {
        cursorUpdateTime = glfwGetTime() + THROTTLE_FAST;

        while (glfwGetTime() < cursorUpdateTime) {}

        UIElement* elem = getElementFromScreenCoords(&rootContainer, mouseX, mouseY);

        if (elem != nullptr && (elem->type == UI_TEXT_AREA || elem->type == UI_TEXT_INPUT)) {
            glfwSetCursor(window, textCursor);
        } else {
            glfwSetCursor(window, nullptr);
        }

        cursorShouldUpdate = false;
    }
}

void throttleUpdateCursorShape() {
    if (!cursorShouldUpdate) {
        cursorShouldUpdate = true;
        thread t(updateCursorShape);
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

                elem->onMouseDown(elem, mouseX, mouseY, resolution);
            } else if (activeElement != nullptr) {
                activeElement->active = false;
                activeElement = nullptr;
            }

            if (highlightedContentBlock.show) {
                if (
                    mouseX >= highlightedContentBlock.left - 20.0f
                    && mouseX <= highlightedContentBlock.left
                    && mouseY >= highlightedContentBlock.top
                    && mouseY <= highlightedContentBlock.bottom
                    && highlightedContentBlock.cbIndex >= tabs[activeTabIndex].contentBlocks.size()
                ) {
                    tabs[activeTabIndex].contentBlocks[highlightedContentBlock.cbIndex].flags |= CONTENT_BLOCK_DRAGGING;
                    highlightedContentBlock.mouseDown = true;
                } else {
                    tabs[activeTabIndex].contentBlocks[highlightedContentBlock.cbIndex].flags &= ~(CONTENT_BLOCK_DRAGGING);
                    highlightedContentBlock.mouseDown = false;
                    cbMove.targetIndex = 0;
                }
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
            }

            UIElement* elem = getElementFromScreenCoords(&rootContainer, mouseX, mouseY);

            if (elem != nullptr) {
                if (elem == activeElement) {
                    elem->onMouseUp(elem, mouseX, mouseY, resolution);
                }

                if (mouseDownX == mouseX && mouseDownY == mouseY) {
                    elem->onClick(elem);
                }
            }
        }
    }
}

void keyCallback(GLFWwindow* window, int key, int scanCode, int action, int mods) {
    #ifdef DEBUG_TEXT_INPUT
        cout << "\nKeyboard event. Key: " << key << ", Scan code:" << scanCode
            << ", Action: " << action << ", Modifier Keys: " << mods;
    #endif

    if (action == GLFW_PRESS) {
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

    if (activeElement != nullptr) {
        activeElement->onKey(activeElement, key, action, mods);

        if (!activeElement->active) {
            activeElement = nullptr;
        }
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

    renderElement(rootContainer, resolution, cursor);
    glfwSwapBuffers(window);
}

void cursorPosCallback(GLFWwindow * window, double xpos, double ypos) {
    mouseX = xpos;
    mouseY = ypos;

    if (!highlightedContentBlock.mouseDown) {
        float top;
        highlightedContentBlock.show = false;

        for (
            auto itr = tabs[activeTabIndex].contentBlocks.begin();
            itr != tabs[activeTabIndex].contentBlocks.end();
            ++itr
        ) {
            top = (*itr).top - contentContainer->extraData->scrollData.yOffset;

            if (ypos > top && ypos < top + (*itr).height) {
                if ((*itr).type == ContentBlockType::Title) {
                    highlightedContentBlock.show = false;
                    break;
                }

                // TODO: Make sure the cursor is not over a scrollbar.
                highlightedContentBlock.show = true;
                highlightedContentBlock.left = contentLeft;
                highlightedContentBlock.top = top;

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
        }
    }

    if (highlightedContentBlock.show && highlightedContentBlock.mouseDown) {
        #define SCROLL_DATA contentContainer->extraData->scrollData

        for (
            auto itr = tabs[activeTabIndex].contentBlocks.begin();
            itr != tabs[activeTabIndex].contentBlocks.end();
            ++itr
        ) {
            float top = (*itr).top - SCROLL_DATA.yOffset;

            uint32_t index = std::distance(tabs[activeTabIndex].contentBlocks.begin(), itr);

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
        activeElement->onMouseDrag(activeElement, mouseX, mouseY, resolution);
    }

    throttleUpdateCursorShape();
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

    bool shiftPressed = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS
        || glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;

    bool horizontalScroll = xOffset != 0 || shiftPressed && yOffset != 0;
    bool verticalScroll = !shiftPressed && yOffset != 0;

    if (elem != nullptr && (elem->type == UI_SCROLL_AREA || elem->type == UI_CONTENT_CONTAINER)) {
        bool scrolled = false;

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

    #undef Y_OFFSET
    #undef X_OFFSET
}

void minimizedCallback(GLFWwindow* window, int isMinimized) {
    // cout << "\n!!!! minimizedcallback";
    if (isMinimized) {
        minimized = true;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        // cout << "\nMinimized!";
        return;
    }

    minimized = false;
}

void maximizedCallback(GLFWwindow* window, int maximized) {
    // cout << "\nMaximized!";
    if (maximized) {
        minimized = false;
        return;
    }
}

void windowFocusCallback(GLFWwindow *window, int isFocused) {
    if (isFocused) {
        // cout << "\nFocused";
        focused = true;
    } else {
        focused = false;
        // cout << "\nOut of focus";
    }
}

/**
 * Just a function to test the button click behaviour.
 *
 * TODO: remove
 */
void testOnClick(UIElement* elem) {
    elem->text += " (clicked)";
    elements[5].text += " (who else?)";
}

/**
 * Sets up the startup ui.
 *
 * **Note:** For now this contains test values. This will change.
 */
void setupUI(bool isDarkTheme) {
    initTextRendering(windowWidth, windowHeight);

    glEnable(GL_BLEND);
    glEnable(GL_SAMPLE_SHADING);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // UIElement* menuBar = &elements[0];
    // UIElement* newButton = &elements[1];
    // UIElement* button2 = &elements[2];
    // UIElement* button3 = &elements[3];
    // UIElement* label2 = &elements[4];
    // UIElement* label3 = &elements[5];
    // UIElement* button4 = &elements[6];
    // UIElement* input = &elements[7];
    // UIElement* inputArea = &elements[8];
    // UIElement* scrollArea = &elements[9];
    // UIElement* label4 = &elements[10];
    // UIElement* label5 = &elements[11];
    // UIElement* label6 = &elements[12];

    // initElement(rootContainer, UI_CONTAINER, 0.0f, 0.0f, WINDOW_WIDTH, WINDOW_HEIGHT);
    // initElement(*menuBar, UI_CONTAINER, 0.0f, 0.0f, WINDOW_WIDTH, 64.0f);
    // initElement(*newButton, UI_ICON_BUTTON, 8.0f, 8.0f, 48.0f, 48.0f);
    // initElement(*button2, UI_ICON_BUTTON, 8.0f, 64.0f, 48.0f, 48.0f);
    // initElement(*button3, UI_ICON_BUTTON, 8.0f, 120.0f, 48.0f, 48.0f);
    // initElement(*button4, UI_BUTTON, 72.0f, 8.0f, 48.0f, 48.0f);
    // initElement(*label2, UI_LABEL, 100.0f, 8.0f, 48.0f, 48.0f);
    // initElement(*label3, UI_LABEL, 128.0f, 8.0f, 48.0f, 48.0f);
    // initElement(*input, UI_TEXT_INPUT, 200.0f, 8.0f, 128.0f, 32.0f);
    // initElement(*inputArea, UI_TEXT_AREA, 250.0f, 8.0f, 256.0f, 108.0f);
    // initElement(*label4, UI_LABEL, 10.0f, 8.0f, 48.0f, 48.0f);
    // initElement(*label5, UI_LABEL, 70.0f, 8.0f, 48.0f, 48.0f);
    // initElement(*label6, UI_LABEL, 140.0f, 8.0f, 48.0f, 48.0f);
    // initElement(*scrollArea, UI_SCROLL_AREA, 370.0f, 8.0f, 256.0f, 128.0f);

    // if (darkTheme) {
    //     rootContainer.backgroundColor = ROOT_BG_DARK;
    //     menuBar->backgroundColor = TOOL_BAR_BG_DARK;
    //     menuBar->color = WHITE;
    // } else {
    //     rootContainer.backgroundColor = ROOT_BG;
    //     menuBar->backgroundColor = TOOL_BAR_BG;
    //     menuBar->color = BLACK;
    // }

    // menuBar->text = "test";
    // 
    // menuBar->preferences->widthPreference = UI_FULL;
    // menuBar->preferences->heightPreference = UI_FIXED;
    // menuBar->preferences->verticalAlign = UI_ALIGN_CENTER;
    // menuBar->preferences->horizontalAlign = UI_ALIGN_CENTER;

    // newButton->text = "New";
    // newButton->icon = ICON_PLUS;
    // button2->icon = ICON_FILE;
    // button2->text = "Button2";
    // button4->text = "Test Button";
    // button4->onClick = testOnClick;
    // button4->borderRadius = 10;

    // label2->text = "Made by:";
    // label2->fontSize = 32;
    // label3->text = "Abhinav Gunwant";
    // label3->fontSize = 48;
    // label4->text = "This is test label for scroll Area";
    // label4->fontSize = 48;
    // label4->positioning = UI_POSITIONING_RELATIVE;
    // label5->text = "This is another test label for scroll Area";
    // label5->fontSize = 32;
    // label5->positioning = UI_POSITIONING_RELATIVE;
    // label6->text = "This is yet another test label for scroll area!";
    // label6->fontSize = 16;
    // label6->positioning = UI_POSITIONING_RELATIVE;

    // input->borderRadius = 10;
    // input->text = "Input Test";
    // inputArea->text = "Input Area\nTest line 1\nTest line 2\nTest line 3\nTest line 4";

    // scrollArea->extraData->scrollData.width = 300;
    // scrollArea->extraData->scrollData.height = 200;

    // addChild(&rootContainer, menuBar);
    // addChild(&rootContainer, newButton);
    // addChild(&rootContainer, button2);
    // addChild(&rootContainer, button3);
    // addChild(&rootContainer, button4);
    // addChild(&rootContainer, label2);
    // addChild(&rootContainer, label3);
    // addChild(&rootContainer, input);
    // addChild(&rootContainer, inputArea);

    // addChild(scrollArea, label4);
    // addChild(scrollArea, label5);
    // addChild(scrollArea, label6);
    // addChild(&rootContainer, scrollArea);

    UIElement* menuBar = &elements[0];
    UIElement* newButton = &elements[1];
    UIElement* openButton = &elements[2];
    contentContainer = &elements[3];
    // UIElement* cbTitle = &elements[4];
    // UIElement* para1 = &elements[5];
    // UIElement* para2 = &elements[6];
    // UIElement* para3 = &elements[7];
    // UIElement* para4 = &elements[8];
    // UIElement* head1 = &elements[9];
    // UIElement* head2 = &elements[10];
    // UIElement* head3 = &elements[11];

    initElement(rootContainer, UI_CONTAINER, 0.0f, 0.0f, WINDOW_WIDTH, WINDOW_HEIGHT);
    initElement(*menuBar, UI_CONTAINER, 0.0f, 0.0f, WINDOW_WIDTH, 64.0f);
    initElement(*newButton, UI_ICON_BUTTON, 8.0f, 8.0f, 48.0f, 48.0f);
    initElement(*openButton, UI_ICON_BUTTON, 8.0f, 64.0f, 48.0f, 48.0f);
    initElement(*contentContainer, UI_CONTENT_CONTAINER, 64.0f, 0.0f, WINDOW_WIDTH, WINDOW_HEIGHT - 64.0f);

    // initElement(*testLabel, UI_LABEL, 1200.0f, 8.0f, 256.0f, 48.0f);

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

    vector<ContentBlock> content;

    tabs.push_back(ContentTab { 0, 0, 0, 0, content });
    activeTabIndex = 0;

    ContentBlock* temp;
    appendContentBlock(0, ContentBlockType::Title, "This is a test note title!");
    appendContentBlock(0, ContentBlockType::Paragraph, "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Suspendisse accumsan lobortis sapien a facilisis. Fusce volutpat odio at purus ullamcorper, a feugiat leo vulputate. Etiam et tortor odio.");
    appendContentBlock(0, ContentBlockType::Heading1, "Vivamus scelerisque ac lorem quis lacinia.");
    appendContentBlock(0, ContentBlockType::Paragraph, "Etiam at erat vel ex placerat auctor at quis arcu. Nullam tempus arcu facilisis nisl sodales molestie. Curabitur enim nisi, aliquam sed risus id, ultricies fringilla nisl. Integer accumsan tincidunt pulvinar. Lorem ipsum dolor sit amet, consectetur adipiscing elit. Cras lacinia convallis velit, eu malesuada metus. Proin posuere gravida eros, a iaculis magna porta a.");
    appendContentBlock(0, ContentBlockType::Heading2, "Praesent rutrum odio in neque commodo pretium.");
    appendContentBlock(0, ContentBlockType::Paragraph, "Etiam auctor est sed sagittis ullamcorper. Cras suscipit eu ligula eu feugiat. Nulla vitae nibh justo. Curabitur pretium urna quis nisi ornare imperdiet. Donec diam diam, rutrum sed massa eu, tristique volutpat felis. Morbi pulvinar ante leo, eu tristique velit imperdiet sit amet. Praesent venenatis bibendum erat quis lacinia. Interdum et malesuada fames ac ante ipsum primis in faucibus. Pellentesque porta convallis rutrum.");
    appendContentBlock(0, ContentBlockType::Heading3, "Aenean porta tortor in tellus blandit consectetur.");
    appendContentBlock(0, ContentBlockType::Paragraph, "Pellentesque ac tellus maximus, scelerisque risus sit amet, consequat justo. Phasellus lobortis massa quis ipsum eleifend imperdiet. Aliquam in convallis tellus, sit amet pharetra diam. Praesent eu sagittis nunc, scelerisque maximus enim. Vivamus varius, elit sed rutrum dapibus, ante purus sodales lorem, vitae malesuada risus dolor quis leo. Proin porta ex orci, ac vulputate purus pulvinar sed. Curabitur non porta sapien. Mauris tortor magna, congue ac volutpat vel, aliquam eget elit. Nulla ullamcorper tempus felis, et fermentum dolor condimentum a. Nullam rhoncus cursus consequat.");

    // testLabel->text = "Testing the scroll area for full width";
    // testLabel->fontSize = 48.0f;
    // testLabel->positioning = UI_POSITIONING_RELATIVE;

    addChild(menuBar, newButton);
    addChild(menuBar, openButton);

    addChild(&rootContainer, menuBar);
    addChild(&rootContainer, contentContainer);

    textCursor = glfwCreateStandardCursor(GLFW_IBEAM_CURSOR);

    initFontTexture(getFontAtlasPath(0).string());

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
        // renderText("Test", glm::vec2(0.0f, 500.0f), resolution, BLACK, 48);

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

