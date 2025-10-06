#ifndef PATRAM_CORE_SETUP_UI
#define PATRAM_CORE_SETUP_UI

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define MENU_BAR 0;

#include "ui.hpp"

extern GLFWwindow* window;
extern UIElement rootContainer;
extern bool minimized;
extern bool focused;
extern uint16_t windowHeight;
extern uint16_t windowWidth;

void resizeCallback(GLFWwindow* window, int width, int height);
void cursorPosCallback(GLFWwindow * window, double xpos, double ypos);
void scrollCallback(GLFWwindow * window, double xoffset, double yoffset);
void minimizedCallback(GLFWwindow* window, int minimized);
void maximizedCallback(GLFWwindow* window, int maximized);
void windowFocusCallback(GLFWwindow *window, int focused);

void setupUI(bool darkTheme);
void enterRenderLoop();

#endif // PATRAM_CORE_SETUP_UI

