#include <iostream>
#include <thread>
#include <chrono>
#include <stack>

#include "include/setup-ui.hpp"
#include "GLFW/glfw3.h"
#include "include/window-utils.hpp"
#include "text.hpp"

#define ROOT_BG             glm::vec4(1.0, 1.0, 1.0, 1.0)
#define TOOL_BAR_BG         glm::vec4(0.93, 0.84, 0.85, 1.0)
#define ROOT_BG_DARK        glm::vec4(0.10, 0.09, 0.09, 1.0)
#define TOOL_BAR_BG_DARK    glm::vec4(0.19, 0.17, 0.17, 1.0)

using namespace std;

GLFWwindow* window;
UIElement rootContainer;
bool minimized = false;
bool focused = true;
bool mouse1Down = false;
bool cursorShouldUpdate = false;

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
        } else if (action == GLFW_RELEASE) {
            #ifdef DEBUG_MOUSE_BUTTON
                cout << "\nMouse button 1 released";
            #endif
            mouse1Down = false;

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
    // cout << "\nKeyboard event. Key: " << key << ", Scan code:" << scanCode
    //     << ", Action: " << action << ", Modifier Keys: " << mods;

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
    rootContainer.width = width;
    rootContainer.height = height;

    windowWidth = width;
    windowHeight = height;

    recalculateLayout(&rootContainer);

    resolution = glm::vec2(width, height);
    glm::vec2 cursor = glm::vec2(mouseX, mouseY);

    renderElement(rootContainer, resolution, cursor);
    glfwSwapBuffers(window);
}

void cursorPosCallback(GLFWwindow * window, double xpos, double ypos) {
    mouseX = xpos;
    mouseY = ypos;

    if (
        activeElement != nullptr
        && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1) == GLFW_PRESS
    ) {
        activeElement->onMouseDrag(activeElement, mouseX, mouseY, resolution);
    }

    throttleUpdateCursorShape();
}

void scrollCallback(GLFWwindow * window, double xoffset, double yoffset) {
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

    UIElement* menuBar = &elements[0];
    UIElement* newButton = &elements[1];
    UIElement* button2 = &elements[2];
    UIElement* button3 = &elements[3];
    UIElement* label2 = &elements[4];
    UIElement* label3 = &elements[5];
    UIElement* button4 = &elements[6];
    UIElement* input = &elements[7];
    UIElement* inputArea = &elements[8];

    initElement(rootContainer, UI_CONTAINER, 0.0f, 0.0f, WINDOW_WIDTH, WINDOW_HEIGHT);
    initElement(*menuBar, UI_CONTAINER, 0.0f, 0.0f, WINDOW_WIDTH, 64.0f);
    initElement(*newButton, UI_ICON_BUTTON, 8.0f, 8.0f, 48.0f, 48.0f);
    initElement(*button2, UI_ICON_BUTTON, 8.0f, 64.0f, 48.0f, 48.0f);
    initElement(*button3, UI_ICON_BUTTON, 8.0f, 120.0f, 48.0f, 48.0f);
    initElement(*button4, UI_BUTTON, 72.0f, 8.0f, 48.0f, 48.0f);
    initElement(*label2, UI_LABEL, 100.0f, 8.0f, 48.0f, 48.0f);
    initElement(*label3, UI_LABEL, 128.0f, 8.0f, 48.0f, 48.0f);
    initElement(*input, UI_TEXT_INPUT, 200.0f, 8.0f, 128.0f, 32.0f);
    initElement(*inputArea, UI_TEXT_AREA, 250.0f, 8.0f, 256.0f, 108.0f);

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
    newButton->icon = ICON_PLUS;
    button2->icon = ICON_FILE;
    button2->text = "Button2";
    button4->text = "Test Button";
    button4->onClick = testOnClick;
    button4->borderRadius = 10;
    label2->text = "Made by:";
    label2->fontSize = 32;
    label3->text = "Abhinav Gunwant";
    label3->fontSize = 48;
    input->borderRadius = 10;
    input->text = "Input Test";
    inputArea->text = "Input Area\nTest line 1\nTest line 2\nTest line 3\nTest line 4";
    // input->onClick = inputClickCallback;

    addChild(rootContainer, menuBar);
    addChild(rootContainer, newButton);
    addChild(rootContainer, button2);
    addChild(rootContainer, button3);
    addChild(rootContainer, button4);
    addChild(rootContainer, label2);
    addChild(rootContainer, label3);
    addChild(rootContainer, input);
    addChild(rootContainer, inputArea);

    textCursor = glfwCreateStandardCursor(GLFW_IBEAM_CURSOR);
}

void enterRenderLoop() {
	while (!glfwWindowShouldClose(window)) {
        if (minimized) {
            glfwWaitEventsTimeout(4000.0f);
        }

        if (!focused) {
            glfwWaitEventsTimeout(80.0f);
        }

        glm::vec2 cursor(mouseX, mouseY);

        renderElement(rootContainer, resolution, cursor);

        glfwSwapBuffers(window);
		glfwPollEvents();

        #ifdef DEBUG_TEXT
        debugTextFirstRenderPrint = false;
        #endif
        // glfwSwapInterval(1);
    }
}

