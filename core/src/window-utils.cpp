#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "include/window-utils.hpp"
#include "include/setup-ui.hpp"
#include "ui.hpp"
#include "note.hpp"

using namespace std;

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

    glfwSetFramebufferSizeCallback(window, resizeCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetScrollCallback(window, scrollCallback);
    glfwSetWindowIconifyCallback(window, minimizedCallback);
    glfwSetWindowMaximizeCallback(window, maximizedCallback);
    glfwSetWindowFocusCallback(window, windowFocusCallback);

	glClearColor(0, 0, 0, 1.0);
}

