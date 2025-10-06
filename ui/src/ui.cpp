#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "include/ui.hpp"
#include "include/shader.hpp"
#include "include/text.hpp"

using namespace std;

float iconButtonVertices[8] = {
    -1.0, -1.0,
    -1.0, -1.0,
    -1.0, -1.0,
    -1.0, -1.0,
};

bool darkTheme = false;

uint32_t iconButtonVAO;
uint32_t iconButtonVBO;

void defaultCallback(UIElement _elem) {}

void initElement(UIElement& elem, UIElementType type, float top, float left, float width, float height) {
    glm::vec4 backgroundColor;
    glm::vec4 hoverColor;
    glm::vec4 color;

    switch (type) {
        case UI_ICON_BUTTON:
            if (darkTheme) {
                backgroundColor = ICON_BUTTON_BG_DARK;
                hoverColor = ICON_BUTTON_HOVER_BG_DARK;
                color = WHITE;
            } else {
                backgroundColor = ICON_BUTTON_BG;
                hoverColor = ICON_BUTTON_HOVER_BG;
                color = WHITE;
            }
            break;

        default:
            backgroundColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
            hoverColor = glm::vec4(0.5f, 0.5f, 0.5f, 0.5f);
            color = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
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
        true,

        defaultCallback,
        defaultCallback,

        "",

        nullptr,

        UI_ALIGN_START,
        UI_ALIGN_START,

        0,
        0,
        nullptr,

        backgroundColor,
        hoverColor,
        color,

        0,
        0,

        UI_FIXED,
        UI_FIXED,
    };
}

void addChild(UIElement& parent, UIElement * child) {
    if (child->parent != nullptr) {
        cerr << "\nError: child already has a parent";
    }

    uint32_t oldChildrenCount = parent.totalChildren;
    uint32_t oldCapacity = parent.capacity;

    if (oldCapacity == 0) {
        parent.children = new UIElement*[INITIAL_PARENT_CAPACITY];
        parent.children[0] = child;
        ++parent.totalChildren;
        parent.capacity = INITIAL_PARENT_CAPACITY;
        child->parent = &parent;
        return;
    }

    if (parent.totalChildren > parent.capacity) {
        if (parent.capacity > GROWTH_THRESHOLD) {
            parent.capacity += INCR_POST_GROWTH_THRESHOLD;
        } else {
            parent.capacity *= GROWTH_FACTOR;
        }

        UIElement** newlist = new UIElement*[parent.capacity];

        for (uint32_t i=0; i<oldChildrenCount; ++i) {
            newlist[i] = parent.children[i];
        }

        delete [] parent.children;

        parent.children = newlist;
    }

    parent.children[parent.totalChildren] = child;
    child->parent = &parent;

    ++parent.totalChildren;
}

void recalculateLayout(UIElement * elem) {
    elem->reDraw = true;

    switch (elem->widthPreference) {
        case UI_FULL:
            if (elem->parent != nullptr) {
                cout << "\nparent element!";
                elem->width = elem->parent->width;
            }
        default:
            break;
    }

    switch (elem->heightPreference) {
        case UI_FULL:
            if (elem->parent != nullptr) {
                elem->height = elem->parent->height;
            }
        default:
            break;
    }

    for (int i=0; i<elem->totalChildren; ++i) {
        recalculateLayout(elem->children[i]);
    }
}

void renderElement(UIElement& elem, glm::vec2 &resolution, glm::vec2 &cursor) {
    // cout << "resolution: " << resolution.x << "x" << resolution.y;
    switch (elem.type) {
        case UI_CONTAINER: {
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

            glUniform1f(timeU, glfwGetTime());

            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

            if (elem.totalChildren) {
                for (int i=0; i<elem.totalChildren; ++i) {
                    renderElement(*elem.children[i], resolution, cursor);
                }
            }

            elem.reDraw = false;

            break;
        }

        case UI_LABEL: {
            break;
        }

        case UI_BUTTON: {
            break;
        }

        case UI_ICON_BUTTON: {
            if (iconButtonVertices[0] == -1.0) {
                cout << "\nInitializing icon button vertices";
                iconButtonVertices[0] = 48.0f;
                iconButtonVertices[1] = 48.0f;
                iconButtonVertices[2] = 48.0f;
                iconButtonVertices[3] = 0.0f;
                iconButtonVertices[4] = 0.0f;
                iconButtonVertices[5] = 48.0f;
                iconButtonVertices[6] = 0.0f;
                iconButtonVertices[7] = 0.0f;

                if (elem.reDraw) {
                    cout << "\nRedrawing icon button";
                    glGenVertexArrays(1, &iconButtonVAO);
                    glBindVertexArray(iconButtonVAO);
                    glGenBuffers(1, &iconButtonVBO);
                    glBindBuffer(GL_ARRAY_BUFFER, iconButtonVBO);
                    glBufferData(GL_ARRAY_BUFFER, sizeof(iconButtonVertices), iconButtonVertices, GL_STATIC_DRAW);
                    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
                    glEnableVertexAttribArray(0);
                }
            }

            glBindVertexArray(iconButtonVAO);
            glUseProgram(shaders[SHADER_ICON_BUTTON].programId);

            int resU = glGetUniformLocation(shaders[SHADER_ICON_BUTTON].programId, "resolution");
            int colorU = glGetUniformLocation(shaders[SHADER_ICON_BUTTON].programId, "solidColor");
            int hoverColorU = glGetUniformLocation(shaders[SHADER_ICON_BUTTON].programId, "hoverColor");
            int timeU = glGetUniformLocation(shaders[SHADER_ICON_BUTTON].programId, "time");
            int topLeftU = glGetUniformLocation(shaders[SHADER_ICON_BUTTON].programId, "topLeft");
            int widthU = glGetUniformLocation(shaders[SHADER_ICON_BUTTON].programId, "width");
            int heightU = glGetUniformLocation(shaders[SHADER_ICON_BUTTON].programId, "height");
            int cursorU = glGetUniformLocation(shaders[SHADER_ICON_BUTTON].programId, "cursor");

            glUniform2f(resU, resolution.x, resolution.y);
            glUniform4f(
                colorU,
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
            glUniform2f(topLeftU, elem.left, elem.top);
            glUniform2f(cursorU, cursor.x, cursor.y);
            glUniform1f(timeU, glfwGetTime());
            glUniform1f(widthU, elem.width);
            glUniform1f(heightU, elem.height);

            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

            elem.reDraw = false;
            break;
        }

        default: break;
    }
}

