#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "ui.hpp"
#include "include/ui.hpp"
#include "include/shader.hpp"

using namespace std;

void defaultCallback(UIElement _elem) {}

void initElement(UIElement& elem, UIElementType type, float top, float left, float width, float height) {
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

        glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),

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

    if (oldChildrenCount == 0) {
        parent.children = new UIElement*[8];
        parent.children[0] = child;
        ++parent.totalChildren;
        child->parent = &parent;
        return;
    }

    if (parent.totalChildren > parent.capacity) {
        if (parent.capacity > 1000) {
            parent.capacity += 128;
        } else {
            parent.capacity *= 2;
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

void renderElement(UIElement& elem, glm::vec2 &resolution) {
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

                float vertices[] = {
                    elem.left + elem.width, elem.top + elem.height,
                    elem.left + elem.width, elem.top,
                    elem.left, elem.top + elem.height,
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
                    renderElement(*elem.children[i], resolution);
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
    }
}

