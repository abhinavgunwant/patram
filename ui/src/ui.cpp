#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "include/ui.hpp"
#include "include/shader.hpp"
#include "include/text.hpp"
#include "../lib/stb_image.h"

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

void defaultCallback(UIElement* _elem) {
    cout << "\nDefault callback executed!";
}

void initElement(UIElement& elem, UIElementType type, float top, float left, float width, float height) {
    glm::vec4 backgroundColor;
    glm::vec4 hoverColor;
    glm::vec4 color;
    uint8_t borderRadius = 0;

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
            color = BLACK;
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

        defaultCallback,
        defaultCallback,

        "",

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

        ICON_NONE,

        16,
        borderRadius,
        pref,
    };
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

void addChild(UIElement& parent, UIElement * child) {
    if (child->parent != nullptr) {
        cerr << "\nError: child already has a parent";
    }

    if (parent.capacity == 0) {
        parent.children = new UIElement*[INITIAL_PARENT_CAPACITY];
        parent.children[0] = child;
        ++parent.totalChildren;
        parent.capacity = INITIAL_PARENT_CAPACITY;
        child->parent = &parent;
        return;
    }

    if (parent.totalChildren + 1 > parent.capacity) {
        if (parent.capacity > GROWTH_THRESHOLD) {
            parent.capacity += INCR_POST_GROWTH_THRESHOLD;
        } else {
            parent.capacity *= GROWTH_FACTOR;
        }

        UIElement** newlist = new UIElement*[parent.capacity];

        for (uint32_t i=0; i<parent.totalChildren; ++i) {
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

    if (elem->preferences != nullptr) {
        switch (elem->preferences->widthPreference) {
            case UI_FULL:
                if (elem->parent != nullptr) {
                    cout << "\nparent element!";
                    elem->width = elem->parent->width;
                }
            default:
                break;
        }

        switch (elem->preferences->heightPreference) {
            case UI_FULL:
                if (elem->parent != nullptr) {
                    elem->height = elem->parent->height;
                }
            default:
                break;
        }
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

                cout << "\nRe-drawing Container";
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
            renderText(elem.text, glm::vec2(elem.left, elem.top), resolution, elem.color, elem.fontSize);
            break;
        }

        case UI_BUTTON: {
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

            glUniform1f(timeU, glfwGetTime());
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
                    cout << "\nRedrawing icon button";
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
            glUniform1f(timeU, glfwGetTime());
            glUniform1f(widthU, elem.width);
            glUniform1f(heightU, elem.height);

            if (elem.icon != ICON_NONE) {
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
                        cout << "\nTexture " << location << " loaded.";
                        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
                        glGenerateMipmap(GL_TEXTURE_2D);
                    } else {
                        cerr << "\nFailed to load texture.";
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

        case UI_TEXT_AREA: {
            break;
        }

        case UI_INPUT: {
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

string getIconLocation(Icon icon) {
    string path = "assets/images/";

    switch (icon) {
        case ICON_NONE: return "";
        case ICON_PLUS: path += "plus.png"; break;
        case ICON_FILE: path += "file.png"; break;
    }

    return path;
}

