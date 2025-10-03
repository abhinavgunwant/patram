#include <iostream>
#include <thread>
#include <chrono>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "include/setup-ui.hpp"
#include "include/window-utils.hpp"

#define WHITE               glm::vec4(1.0, 1.0, 1.0, 1.0)
#define BLACK               glm::vec4(0.0, 0.0, 0.0, 0.0)
#define ROOT_BG             glm::vec4(1.0, 1.0, 1.0, 1.0)
#define TOOL_BAR_BG         glm::vec4(0.93, 0.84, 0.85, 1.0)
#define ICON_BUTTON_BG      glm::vec4(0.84, 0.80, 0.81, 1.0)
#define ROOT_BG_DARK        glm::vec4(0.10, 0.09, 0.09, 1.0)
#define TOOL_BAR_BG_DARK    glm::vec4(0.19, 0.17, 0.17, 1.0)
#define ICON_BUTTON_BG_DARK glm::vec4(0.22, 0.19, 0.20, 1.0)

using namespace std;

GLFWwindow* window;
UIElement rootContainer;
bool minimized = false;
bool focused = true;

UIElement elements[16];

uint16_t windowHeight = WINDOW_HEIGHT;
uint16_t windowWidth = WINDOW_WIDTH;

void resizeCallback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
    rootContainer.width = width;
    rootContainer.height = height;

    windowWidth = width;
    windowHeight = height;

    recalculateLayout(&rootContainer);

    renderElement(rootContainer, glm::vec2(width, height));
    glfwSwapBuffers(window);
}

void cursorPosCallback(GLFWwindow * window, double xpos, double ypos) {
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

void setupUI(bool darkTheme) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    UIElement* menuBar = &elements[0];
    UIElement* newButton = &elements[1];

    initElement(rootContainer, UI_CONTAINER, 0.0f, 0.0f, WINDOW_WIDTH, WINDOW_HEIGHT);
    initElement(*menuBar, UI_CONTAINER, 0.0f, 0.0f, WINDOW_WIDTH, 64.0f);
    initElement(*newButton, UI_ICON_BUTTON, 8.0f, 8.0f, WINDOW_WIDTH, 48.0f);

    if (darkTheme) {
        rootContainer.backgroundColor = ROOT_BG_DARK;
        menuBar->backgroundColor = TOOL_BAR_BG_DARK;
        menuBar->color = WHITE;
        newButton->backgroundColor = ICON_BUTTON_BG_DARK;
        // newButton->backgroundColor = glm::vec4(1.0, 0.0, 0.0, 1.0);
        newButton->color = WHITE;
    } else {
        rootContainer.backgroundColor = ROOT_BG;
        menuBar->backgroundColor = TOOL_BAR_BG;
        menuBar->color = BLACK;
        newButton->backgroundColor = ICON_BUTTON_BG;
        newButton->color = BLACK;
    }

    menuBar->text = "test";
    menuBar->widthPreference = UI_FULL;
    menuBar->heightPreference = UI_FIXED;

    newButton->text = "New";

    addChild(rootContainer, menuBar);
    addChild(rootContainer, newButton);
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

        renderElement(rootContainer, resolution);

        glfwSwapBuffers(window);
		glfwPollEvents();
        glfwSwapInterval(1);
    }
}

