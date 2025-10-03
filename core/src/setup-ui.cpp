#include <iostream>
#include <thread>
#include <chrono>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "include/setup-ui.hpp"
#include "include/window-utils.hpp"

#define WHITE               glm::vec4(1.0, 1.0, 1.0, 1.0);
#define BLACK               glm::vec4(0.0, 0.0, 0.0, 0.0);
#define ROOT_BG_DARK        glm::vec4(0.10, 0.09, 0.09, 1.0);
#define TOOL_BAR_BG_DARK    glm::vec4(0.19, 0.17, 0.17, 1.0);
#define ROOT_BG             glm::vec4(1.0, 1.0, 1.0, 1.0);
#define TOOL_BAR_BG         glm::vec4(0.93, 0.84, 0.85, 1.0);

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
    initElement(rootContainer, UI_CONTAINER, 0.0f, 0.0f, WINDOW_WIDTH, WINDOW_HEIGHT);
    UIElement* a = &elements[0];
    initElement(*a, UI_CONTAINER, 0.0f, 0.0f, WINDOW_WIDTH, 40.0f);

    if (darkTheme) {
        rootContainer.backgroundColor = ROOT_BG_DARK;
        a->backgroundColor = TOOL_BAR_BG_DARK;
        a->color = WHITE;
    } else {
        rootContainer.backgroundColor = ROOT_BG;
        a->backgroundColor = TOOL_BAR_BG;
        a->color = BLACK;
    }

    a->text = "test";
    a->widthPreference = UI_FULL;
    a->heightPreference = UI_FIXED;

    addChild(rootContainer, a);
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

