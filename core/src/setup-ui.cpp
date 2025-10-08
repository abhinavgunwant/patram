#include <iostream>
#include <thread>
#include <chrono>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "include/setup-ui.hpp"
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

UIElement elements[16];

uint16_t windowHeight = WINDOW_HEIGHT;
uint16_t windowWidth = WINDOW_WIDTH;
float cursorX = 25000;
float cursorY = 25000;

void resizeCallback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
    rootContainer.width = width;
    rootContainer.height = height;

    windowWidth = width;
    windowHeight = height;

    recalculateLayout(&rootContainer);

    renderElement(rootContainer, glm::vec2(width, height), glm::vec2(cursorX, cursorY));
    glfwSwapBuffers(window);
}

void cursorPosCallback(GLFWwindow * window, double xpos, double ypos) {
    cursorX = xpos;
    cursorY = ypos;
}

void scrollCallback(GLFWwindow * window, double xoffset, double yoffset) {
}

void minimizedCallback(GLFWwindow* window, int isMinimized) {
    cout << "\n!!!! minimizedcallback";
    if (isMinimized) {
        minimized = true;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        cout << "\nMinimized!";
        return;
    }

    minimized = false;
}

void maximizedCallback(GLFWwindow* window, int maximized) {
    cout << "\nMaximized!";
    if (maximized) {
        minimized = false;
        return;
    }
}

void windowFocusCallback(GLFWwindow *window, int isFocused) {
    if (isFocused) {
        cout << "\nFocused";
        focused = true;
    } else {
        focused = false;
        cout << "\nOut of focus";
    }
}

void processInput(GLFWwindow * win) {
}

void setupUI(bool isDarkTheme) {
    initTextRendering(windowWidth, windowHeight);

    glEnable(GL_BLEND);
    glEnable(GL_SAMPLE_SHADING);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    UIElement* menuBar = &elements[0];
    UIElement* newButton = &elements[1];
    UIElement* button2 = &elements[2];
    UIElement* button3 = &elements[3];
    UIElement* label = &elements[4];
    UIElement* label2 = &elements[5];
    UIElement* label3 = &elements[6];
    UIElement* label4 = &elements[7];
    UIElement* label5 = &elements[8];
    UIElement* label6 = &elements[9];

    initElement(rootContainer, UI_CONTAINER, 0.0f, 0.0f, WINDOW_WIDTH, WINDOW_HEIGHT);
    initElement(*menuBar, UI_CONTAINER, 0.0f, 0.0f, WINDOW_WIDTH, 64.0f);
    initElement(*newButton, UI_ICON_BUTTON, 8.0f, 8.0f, 48.0f, 48.0f);
    initElement(*button2, UI_ICON_BUTTON, 8.0f, 64.0f, 48.0f, 48.0f);
    initElement(*button3, UI_ICON_BUTTON, 8.0f, 120.0f, 48.0f, 48.0f);
    initElement(*label, UI_LABEL, 80.0f, 8.0f, 48.0f, 48.0f);
    initElement(*label2, UI_LABEL, 100.0f, 8.0f, 48.0f, 48.0f);
    initElement(*label3, UI_LABEL, 128.0f, 8.0f, 48.0f, 48.0f);
    initElement(*label4, UI_LABEL, 180.0f, 8.0f, 48.0f, 48.0f);
    initElement(*label5, UI_LABEL, 230.0f, 8.0f, 48.0f, 48.0f);
    initElement(*label6, UI_LABEL, 290.0f, 8.0f, 48.0f, 48.0f);

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
    menuBar->widthPreference = UI_FULL;
    menuBar->heightPreference = UI_FIXED;
    menuBar->verticalAlign = UI_ALIGN_CENTER;
    menuBar->horizontalAlign = UI_ALIGN_CENTER;

    newButton->text = "New";
    newButton->icon = ICON_PLUS;
    button2->icon = ICON_FILE;
    button2->text = "Button2";
    label->text = "Test Label jxyz";
    label2->text = "Made by:";
    label2->fontSize = 32;
    label3->text = "Abhinav Gunwant";
    label3->fontSize = 48;

    label4->text = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    label4->fontSize = 48;

    label5->text = "abcdefghijklmnopqrstuvwxyz";
    label5->fontSize = 48;

    label6->text = "0123456789";
    label6->fontSize = 48;

    addChild(rootContainer, menuBar);
    addChild(rootContainer, newButton);
    addChild(rootContainer, button2);
    addChild(rootContainer, button3);
    addChild(rootContainer, label);
    addChild(rootContainer, label2);
    addChild(rootContainer, label3);
    addChild(rootContainer, label4);
    addChild(rootContainer, label5);
    addChild(rootContainer, label6);
}

void enterRenderLoop() {
	while (!glfwWindowShouldClose(window)) {
        if (minimized) {
            glfwWaitEventsTimeout(4000.0f);
        }

        if (!focused) {
            glfwWaitEventsTimeout(80.0f);
        }

        processInput(window);

        glm::vec2 resolution(windowWidth, windowHeight);
        glm::vec2 cursor(cursorX, cursorY);

        renderElement(rootContainer, resolution, cursor);

        glfwSwapBuffers(window);
		glfwPollEvents();

        #ifdef DEBUG_TEXT
        debugTextFirstRenderPrint = false;
        #endif
        // glfwSwapInterval(1);
    }
}

