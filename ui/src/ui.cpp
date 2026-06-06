#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <stack>
#include <thread>
#include <chrono>
#include <filesystem>

#ifdef DEBUG_TEXT_INPUT
    #include <string_view>
#endif

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "include/ui.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "include/command.hpp"
#include "include/shader.hpp"
#include "include/text.hpp"
#include "files.hpp"
#include "utils.hpp"
#include "../lib/stb_image.h"

#define TIME() static_cast<float>(glfwGetTime())
#define LAST_IN_LIST(item) while (item->next != nullptr) { item = item->next; }

#define SETUP_BUFFERS(vao, vbo, vertices) do {\
        glGenVertexArrays(1, &vao);\
        glBindVertexArray(vao);\
        glGenBuffers(1, &vbo);\
        glBindBuffer(GL_ARRAY_BUFFER, vbo);\
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);\
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);\
        glEnableVertexAttribArray(0);\
    } while (false)

#define SETUP_BUFFERS_DYNAMIC(vao, vbo, vertices) do {\
        glGenVertexArrays(1, &vao);\
        glBindVertexArray(vao);\
        glGenBuffers(1, &vbo);\
        glBindBuffer(GL_ARRAY_BUFFER, vbo);\
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);\
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);\
        glEnableVertexAttribArray(0);\
    } while (false)

#define MOUSE_ON_EXPLORER_LINE(x, y) x >= EXPLORER.width - 3\
    && x <= EXPLORER.width + 2 && y > EXPLORER.top

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

float contentLeft = CONTENT_HORIZONTAL_MARGIN;
float contentRight = 530.0f;

HighlightedContentBlock highlightedContentBlock = {
    .cbIndex = 0,
    .show =  false,
    .updated =  false,
    .mouseDown = false,
    .top =  0.0f,
    .left =  0.0f,
    .bottom =  0.0f,
    .right =  0.0f,
    .backgroundVAO = 0,
    .backgroundVBO = 0,
    .dragTabVAO = 0,
    .dragTabVBO = 0,
};

ContentBlockMoveMarker cbMove = { 0, 0, 0, false, true };

vector<ContentTab> tabs;
ContentTabDisplay tabsDisplay[100];
uint8_t activeTabIndex;
// UIElement* contentContainer;
UIElement* activeElement = nullptr;
uint16_t windowHeight;
uint16_t windowWidth;
glm::vec2 resolution;
GLFWwindow* window;
bool minimized = false;
bool focused = true;
bool mouse1Down = false;
bool cursorShouldUpdate = true;
bool showFps = false;
bool vaultSelected = false;
bool cbTextDragging = false;
CursorShape currentCursorShape = CursorShape::Default;

string selectedVault = "";

GLFWcursor* textCursor;
GLFWcursor* pointerCursor;
GLFWcursor* hResizeCursor;

UIElement elements[16];

float mouseX = 25000;
float mouseY = 25000;
float mouseDownX = 25000;
float mouseDownY = 25000;
float cursorUpdateTime = 0.0;

uint32_t untitledNotes = 0;

Explorer explorer = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    false, false, false, false,
    vector<ExplorerNode>()
};

#ifdef DEBUG_TEXT
    float prevTextTime = 0.0f;
    float textTime = 0.0f;
    float cumulativeTextTime = 0.0f;
    uint32_t textTimeCalculated = 0;
#endif

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
        exit(1);
    }

    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

    glfwSetKeyCallback(window, keyCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetFramebufferSizeCallback(window, resizeCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetScrollCallback(window, scrollCallback);
    glfwSetWindowIconifyCallback(window, minimizedCallback);
    glfwSetWindowMaximizeCallback(window, maximizedCallback);
    glfwSetWindowFocusCallback(window, windowFocusCallback);

    glfwSetInputMode(window, GLFW_LOCK_KEY_MODS, GLFW_TRUE);

    glClearColor(0, 0, 0, 1.0);
}

void updateCursorShape(CursorShape shape) {
    if (shape == currentCursorShape && shape != CursorShape::Auto) {
        cursorShouldUpdate = true;
        return;
    }

    switch (shape) {
        case CursorShape::Auto: {
            UIElement* elem = getElementFromScreenCoords(&ROOT_CONTAINER, mouseX, mouseY);

            if (elem != nullptr && (elem->type == UIElementType::TextArea || elem->type == UIElementType::TextInput)) {
                #ifdef DEBUG_CURSOR_SHAPE
                cout << "\nDEBUG: Cursor shape: Text";
                #endif
                glfwSetCursor(window, textCursor);
                currentCursorShape = CursorShape::Text;
            } else {
                #ifdef DEBUG_CURSOR_SHAPE
                cout << "\nDEBUG: Cursor shape: Default";
                #endif
                glfwSetCursor(window, nullptr);
                currentCursorShape = CursorShape::Default;
            }

            break;
        }

        case CursorShape::Default:
            #ifdef DEBUG_CURSOR_SHAPE
            cout << "\nDEBUG: Cursor shape: Default";
            #endif
            glfwSetCursor(window, nullptr);
            currentCursorShape = CursorShape::Default;
            break;

        case CursorShape::Text:
            #ifdef DEBUG_CURSOR_SHAPE
            cout << "\nDEBUG: Cursor shape: Text";
            #endif
            glfwSetCursor(window, textCursor);
            currentCursorShape = CursorShape::Text;
            break;
        case CursorShape::Pointer:
            #ifdef DEBUG_CURSOR_SHAPE
            cout << "\nDEBUG: Cursor shape: Pointer";
            #endif
            glfwSetCursor(window, pointerCursor);
            currentCursorShape = CursorShape::Pointer;
            break;
        case CursorShape::HorizontalResize:
            #ifdef DEBUG_CURSOR_SHAPE
            cout << "\nDEBUG: Cursor shape: HorizontalResize";
            #endif
            glfwSetCursor(window, hResizeCursor);
            currentCursorShape = CursorShape::HorizontalResize;
            break;
    }

    while (glfwGetTime() < cursorUpdateTime) {}

    cursorUpdateTime = glfwGetTime() + THROTTLE_FAST;
    cursorShouldUpdate = true;
}

void throttleUpdateCursorShape(CursorShape shape) {
    if (cursorShouldUpdate) {
        cursorShouldUpdate = false;
        thread t(updateCursorShape, shape);
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

    if (elem->textInputData != nullptr) {
        elem->textInputData->lastActiveTime = glfwGetTime();
        // selectTextWithMouse(elem, mouseX, mouseY, mouseDownX, mouseDownY, true, resolution);
    }
}

void setupWelcomeScreen();
void destroyWelcomeScreen();
void onVaultBackClicked();

void onVaultSetupCreateVaultClicked() {
    cout << "\nCreating vault...";
    string vaultName;
    string vaultPassword;

    bool nameInit = false;
    bool passwordInit = false;

    for (int i=0; i<WELCOME_SCREEN.totalChildren && (!nameInit || !passwordInit); ++i) {
        if (WELCOME_SCREEN.children[i]->id == VAULT_SETUP_VAULT_NAME_TEXT_INPUT_ID) {
            vaultName = WELCOME_SCREEN.children[i]->text;
            nameInit = true;
        } else if (WELCOME_SCREEN.children[i]->id == VAULT_SETUP_VAULT_PASSWORD_TEXT_INPUT_ID) {
            vaultPassword = WELCOME_SCREEN.children[i]->text;
            passwordInit = true;
        }
    }

    if (nameInit) {
        createVaultInfoFile(vaultName, vaultPassword);
        createVaultIndexFile(vaultName, vaultPassword);
        setDefaultVault(vaultName);

        vaultSelected = true;
        selectedVault = vaultName;

        onVaultBackClicked();
        return;
    }

    // TODO: display some error like "Vault name cannot be empty!"
}

void onVaultBackClicked() {
    destroyElementChildren(WELCOME_SCREEN);
    setupWelcomeScreen();
}

void onCreateANewVaultClicked() {
    #define ELEM_COUNT  8

    UIElement** elems = new UIElement*[ELEM_COUNT];

    for (int i=0; i<ELEM_COUNT; ++i) {
        elems[i] = new UIElement;
    }

    initElement(*elems[0], UIElementType::Label, 0.0f, 0.0f, 0.0f, 0.0f);
    initElement(*elems[1], UIElementType::Label, 60.0f, 0.0f, 0.0f, 0.0f);
    initElement(*elems[2], UIElementType::Label, 120.0f, 0.0f, 0.0f, 0.0f);
    initElement(*elems[3], UIElementType::Label, 200.0f, 0.0f, 0.0f, 0.0f);
    initElement(*elems[4], UIElementType::TextInput, 150.0f, 0.0f, 240.0f, 30.0f);
    initElement(*elems[5], UIElementType::TextInput, 230.0f, 0.0f, 240.0f, 30.0f);
    initElement(*elems[6], UIElementType::Button, 290.0f, 0.0f, 0.0f, 0.0f);
    initElement(*elems[7], UIElementType::Button, 340.0f, 0.0f, 0.0f, 0.0f);

    float vertices[] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };

    SETUP_BUFFERS((*elems[4]).vao, (*elems[4]).vbo, vertices);
    SETUP_BUFFERS((*elems[5]).vao, (*elems[5]).vbo, vertices);
    SETUP_BUFFERS((*elems[6]).vao, (*elems[6]).vbo, vertices);
    SETUP_BUFFERS((*elems[7]).vao, (*elems[7]).vbo, vertices);

    for (int i=0; i<ELEM_COUNT; ++i) {
        (*elems[i]).preferences->horizontalAlign = UIAlignment::Center;
        (*elems[i]).preferences->widthPreference = UISizePreference::Grow;
        (*elems[i]).parent = &WELCOME_SCREEN;

        if (i != 0) {
            (*elems[i]).fontSize = 16;
        }
    }

    (*elems[2]).text = "Vault Name:";
    (*elems[3]).text = "Vault Password:";
    (*elems[4]).text = "Untitled Vault";
    (*elems[4]).textInputData->alignment = TextInputAlignment::Center;
    (*elems[4]).id = VAULT_SETUP_VAULT_NAME_TEXT_INPUT_ID;
    (*elems[5]).text = "password";
    (*elems[5]).textInputData->alignment = TextInputAlignment::Center;
    (*elems[5]).textInputData->password = true;
    (*elems[5]).id = VAULT_SETUP_VAULT_PASSWORD_TEXT_INPUT_ID;

    (*elems[6]).text = "Create Vault";
    (*elems[6]).preferences->heightPreference = UISizePreference::Grow;
    (*elems[6]).id = VAULT_SETUP_CREATE_VAULT_BUTTON_ID;

    (*elems[7]).text = "Back";
    (*elems[7]).preferences->heightPreference = UISizePreference::Grow;
    (*elems[7]).id = VAULT_BACK_BUTTON_ID;

    (*elems[0]).text = "patram";
    (*elems[0]).fontSize = 48;

    (*elems[1]).text = "Vault Setup";
    (*elems[1]).fontSize = 16;

    WELCOME_SCREEN.children = elems;
    WELCOME_SCREEN.totalChildren = ELEM_COUNT;
    WELCOME_SCREEN.capacity = ELEM_COUNT;

    destroyWelcomeScreen();

    #undef ELEM_COUNT
}

void onSwitchVaultClicked() {
    #define ELEM_COUNT  4

    vector<string> vaults = getVaults();

    UIElement** elems = new UIElement*[ELEM_COUNT];
    UIElement** vaultElems = new UIElement*[vaults.size()];

    for (int i=0; i<ELEM_COUNT; ++i) {
        elems[i] = new UIElement;
    }

    for (int i=0; i<vaults.size(); ++i) {
        vaultElems[i] = new UIElement;
        (*vaultElems[i]).positioning = UIPositioning::Relative;
    }

    initElement(*elems[0], UIElementType::Label, 0.0f, 0.0f, 0.0f, 0.0f);
    initElement(*elems[1], UIElementType::Label, 60.0f, 0.0f, 0.0f, 0.0f);
    initElement(*elems[2], UIElementType::ScrollArea, 120.0f, 0.0f, 300.0f, 200.0f);
    initElement(*elems[3], UIElementType::Button, 330.0f, 0.0f, 0.0f, 0.0f);

    float vertices[] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };

    SETUP_BUFFERS((*elems[2]).vao, (*elems[2]).vbo, vertices);
    SETUP_BUFFERS(
        (*elems[2]).scrollData->verticalScrollBarVAO,
        (*elems[2]).scrollData->verticalScrollBarVBO,
        vertices
    );
    SETUP_BUFFERS(
        (*elems[2]).scrollData->verticalScrollThumbVAO,
        (*elems[2]).scrollData->verticalScrollThumbVBO,
        vertices
    );
    SETUP_BUFFERS((*elems[3]).vao, (*elems[3]).vbo, vertices);

    for (int i=0; i<ELEM_COUNT; ++i) {
        (*elems[i]).preferences->horizontalAlign = UIAlignment::Center;
        (*elems[i]).preferences->widthPreference = UISizePreference::Grow;
        (*elems[i]).parent = &WELCOME_SCREEN;

        if (i != 0) {
            (*elems[i]).fontSize = 16;
        }
    }

    float top = 120.0f;

    for (int i=0; i<vaults.size(); ++i, top += 40.0f) {
        initElement(*vaultElems[i], UIElementType::Button, top, 0.0f, 0.0f, 35.0f);

        SETUP_BUFFERS((*vaultElems[i]).vao, (*vaultElems[i]).vbo, vertices);
        (*vaultElems[i]).preferences->widthPreference = UISizePreference::Full;
        (*vaultElems[i]).parent = elems[2];
        (*vaultElems[i]).borderRadius = 0;
        (*vaultElems[i]).backgroundColor = TRANSPARENT_COLOR;
        (*vaultElems[i]).text = vaults[i];
        (*vaultElems[i]).id = SWITCH_VAULT_OPTION_ID;
    }

    (*elems[2]).text = "SWITCH_VLT_SCR";
    (*elems[2]).scrollData->showVerticalScrollbar = true;

    (*elems[3]).text = "Back";
    (*elems[3]).preferences->heightPreference = UISizePreference::Grow;
    (*elems[3]).id = VAULT_BACK_BUTTON_ID;

    (*elems[0]).text = "patram";
    (*elems[0]).fontSize = 48;

    (*elems[1]).text = "Switch Vault";
    (*elems[1]).fontSize = 16;

    (*elems[2]).capacity = vaults.size();
    (*elems[2]).totalChildren = vaults.size();
    (*elems[2]).children = vaultElems;

    WELCOME_SCREEN.children = elems;
    WELCOME_SCREEN.totalChildren = ELEM_COUNT;
    WELCOME_SCREEN.capacity = ELEM_COUNT;

    destroyWelcomeScreen();

    #undef ELEM_COUNT
}

tuple<ExplorerNodeType, filesystem::path> getClickedExplorerNodePath(
    vector<ExplorerNode>& nodes,
    float& y,
    float mouseY
) {
    filesystem::path p;
    ExplorerNodeType nodeType;

    for (auto itr = nodes.begin(); itr != nodes.end(); ++itr) {
        if (mouseY > (*itr).top && mouseY < (*itr).top + EXPLORER_ELEMENT_HEIGHT) {
            ExplorerNodeType ent = (*itr).type;

            if (ent == ExplorerNodeType::Folder) {
                if (GET_FLAG((*itr).flags, EXPLORER_FLAG_EXPANDED)) {
                    UNSET_FLAG((*itr).flags, EXPLORER_FLAG_EXPANDED);
                    #ifdef DEBUG_EXPLORER
                    cout << "\nDEBUG: explorer flag " << (int)((*itr).flags) << " unset for "
                        << (*itr).name << ", vertices:\n";
                    #endif
                } else {
                    SET_FLAG((*itr).flags, EXPLORER_FLAG_EXPANDED);
                    #ifdef DEBUG_EXPLORER
                    cout << "\nDEBUG: explorer flag " << (int)((*itr).flags) << " set for "
                        << (*itr).name << ", vertices:\n";
                    #endif
                }
                EXPLORER.reDraw = true;
            }

            return make_tuple(ent, (*itr).name);
        }

        y += EXPLORER_ELEMENT_HEIGHT;

        if (GET_FLAG((*itr).flags, EXPLORER_FLAG_EXPANDED) && !(*itr).children.empty()) {
            auto node_tuple = getClickedExplorerNodePath((*itr).children, y, mouseY);
            ExplorerNodeType t = get<0>(node_tuple);
            filesystem::path _p = get<1>(node_tuple);

            if (!_p.empty()) {
                p /= (*itr).name / _p;

                return make_tuple(t, p);
            }
        }
    }

    #ifdef DEBUG_EXPLORER
        cout << "\nDEBUG: Clicked explorer path: " << p;
    #endif

    return make_tuple(ExplorerNodeType::Note, p);
}

void unsetExplorerTreeFlag(vector<ExplorerNode>& nodes, uint8_t flag) {
    for (auto itr = nodes.begin(); itr != nodes.end(); ++itr) {
        UNSET_FLAG((*itr).flags, flag);

        if (!(*itr).children.empty()) {
            unsetExplorerTreeFlag((*itr).children, flag);
        }
    }
}

void setExplorerTreeFlag(
    vector<ExplorerNode>& nodes,
    float& y,
    float mouseY,
    float yOffset,
    uint8_t flag
) {
    for (auto itr = nodes.begin(); itr != nodes.end(); ++itr) {
        float y1 = y + EXPLORER_ELEMENT_HEIGHT;
        float top = (*itr).top;

        if (mouseY > (*itr).top && mouseY < (*itr).top + EXPLORER_ELEMENT_HEIGHT) {
            SET_FLAG((*itr).flags, flag);
            explorer.elementHover = true;

            float vertices[8] = {
                0,              (*itr).top - 4.0f,
                0,              (*itr).top + EXPLORER_ELEMENT_HEIGHT - 4.0f,
                EXPLORER.width, (*itr).top - 4.0f,
                EXPLORER.width, (*itr).top + EXPLORER_ELEMENT_HEIGHT - 4.0f,
            };

            glBindVertexArray(explorer.hoverBgVao);
            glBindBuffer(GL_ARRAY_BUFFER, explorer.hoverBgVbo);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        } else {
            UNSET_FLAG((*itr).flags, flag);
        }

        y = y1;

        if (GET_FLAG((*itr).flags, EXPLORER_FLAG_EXPANDED) && !(*itr).children.empty()) {
            setExplorerTreeFlag((*itr).children, y, mouseY, yOffset, flag);
        }
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

            if (highlightedContentBlock.show && !tabs.empty()) {
                if (
                    mouseX >= highlightedContentBlock.left - 20.0f
                    && mouseX <= highlightedContentBlock.left
                    && mouseY >= highlightedContentBlock.top
                    && mouseY <= highlightedContentBlock.bottom
                    && highlightedContentBlock.cbIndex < tabs[activeTabIndex].contentBlocks.size()
                ) {
                    tabs[activeTabIndex].contentBlocks[highlightedContentBlock.cbIndex].flags |= CONTENT_BLOCK_DRAGGING;
                    highlightedContentBlock.mouseDown = true;
                    return;
                } else {
                    tabs[activeTabIndex].contentBlocks[highlightedContentBlock.cbIndex].flags &= ~(CONTENT_BLOCK_DRAGGING);
                    highlightedContentBlock.mouseDown = false;
                    cbMove.targetIndex = 0;
                }
            }

            if (explorer.lineHover) {
                explorer.lineMouseDown = true;
                return;
            }

            UIElement* elem = getElementFromScreenCoords(&ROOT_CONTAINER, mouseX, mouseY);

            if (elem != nullptr) {
                #ifdef DEBUG_MOUSE_BUTTON
                    cout << " for " << elementType(elem);
                #endif

                if (elem->id > 0 && elem->type == UIElementType::Button) {
                    switch(elem->id) {
                        case CREATE_VAULT_BUTTON_ID: onCreateANewVaultClicked(); return;
                        case SWITCH_VAULT_BUTTON_ID: onSwitchVaultClicked(); return;
                        case VAULT_SETUP_CREATE_VAULT_BUTTON_ID: onVaultSetupCreateVaultClicked(); return;
                        case VAULT_BACK_BUTTON_ID: onVaultBackClicked(); return;
                        case NEW_NOTE_BUTTON_ID: newNote(); return;
                        case SWITCH_VAULT_OPTION_ID: {
                            setDefaultVault(elem->text);
                            vaultSelected = true;
                            selectedVault = elem->text;
                            onVaultBackClicked();
                            return;
                        };
                    }
                }

                UIElement* prevActiveElement = activeElement;

                if (activeElement != nullptr) {
                    activeElement->active = false;
                }

                activeElement = elem;
                elem->active = true;

                switch (activeElement->type) {
                    case UIElementType::ContentContainer:
                        if (!tabs.empty()) {
                            contentBlockMouseDownCallback(elem, mouseX, mouseY, resolution);
                        } else {
                            activeElement = prevActiveElement;
                        }
                        break;
                    case UIElementType::ScrollArea:
                        scrollAreaMouseDownCallback(elem, mouseX, mouseY, resolution);
                        break;
                    case UIElementType::TextArea:
                    case UIElementType::TextInput:
                        textInputMouseDownCallback(elem, mouseX, mouseY, resolution);
                        break;
                    case UIElementType::TabBar:
                        if (!tabs.empty()) {
                            tabBarMouseDownCallback(elem, mouseX, mouseY, resolution);
                        } else {
                            activeElement = prevActiveElement;
                        }
                        break;
                    case UIElementType::Explorer:
                        float y = EXPLORER.top + elem->scrollData->yOffset;
                        float time = glfwGetTime();

                        if (
                            elem->scrollData->showVerticalScrollbar
                            && mouseX > elem->width - SCROLL_BAR_THICKNESS
                        ) {
                            elem->scrollData->mouseDown = true;
                            elem->scrollData->mouseDownX = mouseX;
                            elem->scrollData->mouseDownY = mouseY;
                            break;
                        }

                        auto node_tuple = getClickedExplorerNodePath(explorer.nodes, y, mouseY);
                        ExplorerNodeType t = get<0>(node_tuple);
                        filesystem::path p = get<1>(node_tuple);

                        if (t == ExplorerNodeType::Note) {
                            if (time - explorer.lastClickedTime < 0.4f) {
                                explorer.lastClickedTime = 0.0f;

                                filesystem::path nodePath = getVaultPath(selectedVault) / p;
                                openFile(false, nodePath);
                            } else {
                                explorer.lastClickedTime = time;
                            }
                        }
                        break;
                }

                // elem->onMouseDown(elem, mouseX, mouseY, resolution);
            } else if (activeElement != nullptr) {
                activeElement->active = false;
                activeElement = nullptr;
            }
        } else if (action == GLFW_RELEASE) {
            #ifdef DEBUG_MOUSE_BUTTON
                cout << "\nMouse button 1 released";
            #endif
            mouse1Down = false;
            explorer.lineMouseDown = false;
            EXPLORER.scrollData->mouseDown = false;

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
                return;
            }

            UIElement* elem = getElementFromScreenCoords(&ROOT_CONTAINER, mouseX, mouseY);

            if (elem != nullptr && activeElement != nullptr && elem->id == 0) {
                switch (activeElement->type) {
                    case UIElementType::TextArea:
                    case UIElementType::TextInput:
                        textInputMouseUpCallback(elem, mouseX, mouseY, resolution);
                        break;
                    case UIElementType::ContentContainer:
                        if (!tabs.empty()) {
                            contentBlockMouseUpCallback(elem, mouseX, mouseY, resolution);
                        }
                        break;
                    case UIElementType::IconButton:
                        if (elem->id == OPEN_BUTTON_ID) {
                            openFile(true);
                        }
                        break;
                }

                // if (mouseDownX == mouseX && mouseDownY == mouseY) {
                //     elem->onClick(elem);
                // }
            }
        }
    }
}

void keyCallback(GLFWwindow* window, int key, int scanCode, int action, int mods) {
    #ifdef DEBUG_TEXT_INPUT
        #ifdef DEBUG_KEYBOARD_EVENTS
        cout << "\nKeyboard event. Key: " << key << ", Scan code: " << scanCode
            << ", Action: " << action << ", Modifier Keys: " << mods;
        #endif
    #endif

    if (activeElement == nullptr) {
        defaultKeyCallback(nullptr, key, action, mods);
        return;
    }

    if (action == GLFW_PRESS && activeElement->type != UIElementType::ContentContainer) {
        switch (key) {
            case GLFW_KEY_ESCAPE: {
                if (activeElement != nullptr && activeElement->type != UIElementType::TextInput) {
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
        switch (activeElement->type) {
            case UIElementType::ContentContainer: cbKeyCallback(key, action, mods); break;
            case UIElementType::TextInput:
            case UIElementType::TextArea: textInputKeyCallback(activeElement, key, action, mods); break;
            defalt: defaultKeyCallback(activeElement, key, action, mods);
        }
    }

    if (activeElement != nullptr && !activeElement->active) {
        activeElement = nullptr;
    }
}

void resizeCallback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);

    ROOT_CONTAINER.width = width;
    ROOT_CONTAINER.height = height;

    windowWidth = width;
    windowHeight = height;

    if (!minimized) {
        #ifdef DEBUG_LAYOUT
            cout << "\nRecalculating layout";
        #endif

        recalculateLayout(&ROOT_CONTAINER);
    }

    highlightedContentBlock.show = false;
    highlightedContentBlock.updated = true;

    resolution = glm::vec2(width, height);
    glm::vec2 cursor = glm::vec2(mouseX, mouseY);

    if (CONTENT_CONTAINER.show && !tabs.empty()) {
        recalculateContentWidth();

        ContentTab* tab = CURRENT_TAB;

        tab->redrawCursor = true;

        reDrawCBsFrom(tab->firstVisibleCBIndex, tab->visibleCBLength + 16);

        if (tab->showCursor) {
            tab->redrawCursor = true;
        }

        tab->scrollbarUpdated = false;

        TAB_BAR.reDraw = true;

        if (tabs.size() > 1) {
            for (auto tabIter = tabs.begin(); tabIter != tabs.end(); ++tabIter) {
                (*tabIter).scrollbarUpdated = false;
            }
        }
    }

    float _ = 0.0f;

    renderElement(ROOT_CONTAINER, resolution, cursor, 0, _);

    glfwSwapBuffers(window);
}

void cursorPosCallback(GLFWwindow * window, double xpos, double ypos) {
    mouseX = xpos;
    mouseY = ypos;

    CursorShape cShape = CursorShape::Default;

    if (!highlightedContentBlock.mouseDown && !tabs.empty()) {
        ContentTab* tab = CURRENT_TAB;

        float top;
        highlightedContentBlock.show = false;

        for (
            auto itr = tab->contentBlocks.begin();
            itr != tab->contentBlocks.end();
            ++itr
        ) {
            top = (*itr).top - tab->scrollOffsetY;

            if (ypos > top && ypos < top + (*itr).height) {
                cShape = CursorShape::Text;
                if ((*itr).type == ContentBlockType::Title) {
                    highlightedContentBlock.show = false;
                    break;
                }

                // TODO: Make sure the cursor is not over a scrollbar.
                highlightedContentBlock.show = true;
                highlightedContentBlock.left = contentLeft;
                highlightedContentBlock.top = top;

                tab->activeCBIndex = std::distance(tab->contentBlocks.begin(), itr);

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

            // TODO: remove this when 1 is completed
            if (top > windowHeight) {
                break;
            }
        }

        if (tab->cbTextMouseDown) {
            SelectionData sd = getCBTextCursorFromScreenCoords(mouseX, mouseY, resolution);

            if (sd.cursorCBIndex != tab->cursorCBIndex) {
                cbTextDragging = true;

                if (tab->textSelected) {
                    if (sd.cursorCBIndex < tab->initialCBIndex) {
                        tab->selStartCBIndex = sd.cursorCBIndex;
                        tab->selStartCursorPosition = sd.cursorPosition;
                        tab->selEndCBIndex = tab->initialCBIndex;
                    } else {
                        tab->selStartCBIndex = tab->initialCBIndex;
                        tab->selEndCBIndex = sd.cursorCBIndex;
                        tab->selEndCursorPosition = sd.cursorPosition;
                    }
                } else {
                    tab->textSelected = true;

                    if (sd.cursorCBIndex < tab->cursorCBIndex) {
                        tab->selStartCBIndex = sd.cursorCBIndex;
                        tab->selStartCursorPosition = sd.cursorPosition;
                        tab->selEndCBIndex = tab->cursorCBIndex;
                        tab->selEndCursorPosition = tab->cursorPosition;
                    } else {
                        tab->selStartCBIndex = tab->cursorCBIndex;
                        tab->selStartCursorPosition = tab->cursorPosition;
                        tab->selEndCBIndex = sd.cursorCBIndex;
                        tab->selEndCursorPosition = sd.cursorPosition;
                    }
                }

                tab->cursorCBIndex = sd.cursorCBIndex;
                tab->cursorPosition = sd.cursorPosition;
            } else if (sd.cursorPosition != tab->cursorPosition) {
                cbTextDragging = true;

                if (tab->textSelected) {
                    if (tab->selStartCBIndex == tab->selEndCBIndex) {
                        if (sd.cursorPosition == tab->initialCursorPosition) {
                            tab->textSelected = false;
                        } else if (sd.cursorPosition > tab->initialCursorPosition) {
                            tab->selEndCursorPosition = sd.cursorPosition;
                            tab->selStartCursorPosition = tab->initialCursorPosition;
                        } else {
                            tab->selStartCursorPosition = sd.cursorPosition;
                            tab->selEndCursorPosition = tab->initialCursorPosition;
                        }
                    } else {
                        if (tab->selStartCBIndex == sd.cursorCBIndex) {
                            tab->selStartCursorPosition = sd.cursorPosition;
                        } else if (tab->selEndCBIndex == sd.cursorCBIndex) {
                            tab->selEndCursorPosition = sd.cursorPosition;
                        } else {
                            // do nothing
                        }
                    }
                } else {
                    tab->textSelected = true;
                    tab->initialCursorPosition = tab->cursorPosition;
                    tab->initialCBIndex = tab->cursorCBIndex;

                    if (sd.cursorPosition > tab->cursorPosition) {
                        tab->selStartCBIndex = tab->cursorCBIndex;
                        tab->selEndCBIndex = sd.cursorCBIndex;
                        tab->selStartCursorPosition = tab->cursorPosition;
                        tab->selEndCursorPosition = sd.cursorPosition;
                    } else {
                        tab->selStartCBIndex = sd.cursorCBIndex;
                        tab->selEndCBIndex = tab->cursorCBIndex;
                        tab->selStartCursorPosition = sd.cursorPosition;
                        tab->selEndCursorPosition = tab->cursorPosition;
                    }
                }

                tab->cursorCBIndex = sd.cursorCBIndex;
                tab->cursorPosition = sd.cursorPosition;
                tab->updateCursorX = true;
                tab->redrawSel = true;
                updateCBCursor(tab, sd);
            }
        }
    }

    if (highlightedContentBlock.show && highlightedContentBlock.mouseDown && !tabs.empty()) {
        #define SCROLL_DATA CONTENT_CONTAINER.scrollData
        ContentTab* tab = CURRENT_TAB;

        for (
            auto itr = tab->contentBlocks.begin();
            itr != tab->contentBlocks.end();
            ++itr
        ) {
            float top = (*itr).top - SCROLL_DATA->yOffset;

            uint32_t index = std::distance(tab->contentBlocks.begin(), itr);

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
            float newScrollOffset = (50.0f - resolution.y + mouseY)/10 + SCROLL_DATA->yOffset;

            if (newScrollOffset > SCROLL_DATA->height - resolution.y) {
                SCROLL_DATA->yOffset = SCROLL_DATA->height - resolution.y + 50.0f;
            } else {
                SCROLL_DATA->yOffset = newScrollOffset;
            }
        } else if (mouseY < 114.0f) {
            float newScrollOffset = SCROLL_DATA->yOffset - (114.0f - mouseY)/10;

            if (newScrollOffset < 0) {
                SCROLL_DATA->yOffset = 0;
            } else {
                SCROLL_DATA->yOffset = newScrollOffset;
            }
        }

        #undef SCROLL_DATA
    }

    if (explorer.lineMouseDown) {
        if (mouseX > 100.0f && mouseX < 500.0f) {
            float diff = mouseX - EXPLORER.width;

            EXPLORER.width += diff;
            WELCOME_SCREEN.width -= diff;
            CONTENT_CONTAINER.width -= diff;

            MAIN_CONTAINER.reDraw = true;
            EXPLORER.reDraw = true;
            WELCOME_SCREEN.reDraw = true;
            CONTENT_CONTAINER.reDraw = true;
            explorer.redrawLine = true;
        }
        return;
    }

    if (
        activeElement != nullptr
        && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1) == GLFW_PRESS
    ) {
        switch (activeElement->type) {
            case UIElementType::ScrollArea:
                cShape = CursorShape::Text;
                scrollAreaMouseDragCallback(activeElement, mouseX, mouseY, resolution);
                throttleUpdateCursorShape(cShape);
                return;
            case UIElementType::TextArea:
            case UIElementType::TextInput:
                cShape = CursorShape::Text;
                textInputMouseDragCallback(activeElement, mouseX, mouseY, resolution);
                throttleUpdateCursorShape(cShape);
                return;
        }
    }

    if (MOUSE_ON_EXPLORER_LINE(mouseX, mouseY)) {
        if (!explorer.lineHover) {
            explorer.lineHover = true;
            explorer.redrawLine = true;
        }
        cShape = CursorShape::HorizontalResize;
        throttleUpdateCursorShape(cShape);
        return;
    } else {
        explorer.lineHover = false;
    }

    UIElement* elem = getElementFromScreenCoords(&ROOT_CONTAINER, mouseX, mouseY);

    if (
        elem != nullptr
        && elem->type == UIElementType::Explorer
        && elem->show
        && mouseX < elem->width
        && (TAB_BAR.show && mouseY > TAB_BAR.height || !TAB_BAR.show)
    ) {
        if (
            elem->scrollData->showVerticalScrollbar
            && mouseX > elem->width - SCROLL_BAR_THICKNESS
        ) {
            if (elem->scrollData->mouseDown) {
                float oldOffset = elem->scrollData->yOffset;
                elem->scrollData->yOffset += mouseY - elem->scrollData->mouseDownY;

                if (elem->scrollData->yOffset < 0) {
                    elem->scrollData->yOffset = 0;
                } else if (elem->scrollData->yOffset + elem->height > elem->scrollData->height) {
                    elem->scrollData->yOffset = elem->scrollData->height - elem->height;
                }

                if (elem->scrollData->yOffset != oldOffset) {
                    elem->reDraw = true;
                    elem->scrollData->renderedUpdate = false;
                    cout << "\nScroll offset: " << elem->scrollData->yOffset;
                }
                elem->scrollData->mouseDownY = mouseY;
            }
            throttleUpdateCursorShape(CursorShape::Default);
            return;
        } else {
            float y = EXPLORER.top + elem->scrollData->yOffset;

            setExplorerTreeFlag(explorer.nodes, y, mouseY, elem->scrollData->yOffset, EXPLORER_FLAG_HOVER);
            cShape = CursorShape::Default;
        }
    } else if (explorer.elementHover) {
        unsetExplorerTreeFlag(explorer.nodes, EXPLORER_FLAG_HOVER);
        explorer.elementHover = false;
    }

    throttleUpdateCursorShape(cShape);
}

void scrollCallback(GLFWwindow * window, double xOffset, double yOffset) {
    #define Y_OFFSET elem->scrollData->yOffset
    #define X_OFFSET elem->scrollData->xOffset

    #ifdef DEBUG_SCROLL
        cout << "\nScroll callback: xOffset: " << xOffset << ", yOffset: " << yOffset;
    #endif

    highlightedContentBlock.show = false;
    highlightedContentBlock.updated = true;
    UIElement* elem = getElementFromScreenCoords(&ROOT_CONTAINER, mouseX, mouseY);

    if (elem != nullptr) {
        if (elem->parent->type == UIElementType::ScrollArea) {
            elem = elem->parent;
        }

        switch (elem->type) {
            case UIElementType::Explorer:
            case UIElementType::ScrollArea: {
                bool scrolled = false;
                bool shiftPressed = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS
                    || glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
                bool horizontalScroll = xOffset != 0 || shiftPressed && yOffset != 0;
                bool verticalScroll = !shiftPressed && yOffset != 0;

                if (verticalScroll) {
                    if (elem->height + Y_OFFSET < elem->scrollData->height && yOffset < 0) {
                        Y_OFFSET += SCROLL_AMOUNT;
                    } else if (yOffset > 0) {
                        Y_OFFSET -= SCROLL_AMOUNT;
                    }

                    if (Y_OFFSET < 0) {
                        Y_OFFSET = 0;
                    } else if (Y_OFFSET > elem->scrollData->height) {
                        Y_OFFSET = elem->scrollData->height - elem->height;
                    }

                    scrolled = true;
                }

                if (horizontalScroll) {
                    if (
                        elem->width + X_OFFSET < elem->scrollData->width
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
                    elem->reDraw = true;
                    elem->scrollData->lastScrolled = glfwGetTime();
                    elem->scrollData->renderedUpdate = false;

                    for (int i=0; i<elem->totalChildren; ++i) {
                        elem->children[i]->reDraw = true;
                        elem->children[i]->show = true;
                    }
                }

                break;
            }

            case UIElementType::ContentContainer: {
                ContentTab* tab = &tabs[activeTabIndex];

                if (elem->height + tab->scrollOffsetY < tab->maxHeight && yOffset < 0) {
                    tab->scrollOffsetY += SCROLL_AMOUNT;
                } else if (yOffset > 0) {
                    tab->scrollOffsetY -= SCROLL_AMOUNT;
                }

                if (tab->scrollOffsetY < 0) {
                    tab->scrollOffsetY = 0;
                } else if (tab->scrollOffsetY > tab->maxHeight) {
                    tab->scrollOffsetY = tab->maxHeight - CONTENT_CONTAINER.height;
                }

                if (tab->showCursor) {
                    tab->redrawCursor = true;
                }

                tab->scrollbarUpdated = false;

                reDrawCBsFrom(tab->firstVisibleCBIndex, tab->visibleCBLength);

                cout << "\nDEBUG: ON SCROLL: tab->scrollOffsetY: " << tab->scrollOffsetY;
                break;
            }
        }
    }

    #undef Y_OFFSET
    #undef X_OFFSET
}

void minimizedCallback(GLFWwindow* window, int isMinimized) {
    minimized = isMinimized != 0;
}

void maximizedCallback(GLFWwindow* window, int min) {
    // minimized = min == 0;
}

void windowFocusCallback(GLFWwindow *window, int isFocused) {
    focused = isFocused != 0;
}

vector<ExplorerNode> createExplorerNodesFromIndex(vector<VaultIndexEntry>& index) {
    vector<ExplorerNode> nodes;

    if (index.empty()) {
        return nodes;
    }

    nodes.reserve(index.size());

    for (VaultIndexEntry entry: index) {
        vector<ExplorerNode> children;

        if (!entry.children.empty()) {
            children = createExplorerNodesFromIndex(entry.children);
        }

        ExplorerNodeType nodeType;

        if (entry.type == VaultIndexEntryType::Folder) {
            nodeType = ExplorerNodeType::Folder;
        } else {
            nodeType = ExplorerNodeType::Note;
        }

        nodes.push_back({ nodeType, 0, 0, 0, entry.title, children });
    }

    return nodes;
}

void populateExplorerFromIndex() {
    #ifdef DEBUG_EXPLORER
        auto t0 = std::chrono::high_resolution_clock::now();
    #endif

    vector<VaultIndexEntry> index = getVaultIndex(selectedVault);

    #ifdef DEBUG_EXPLORER
        auto t1 = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0);

        cout << "\nDEBUG: Time taken to get entries from index: " << duration.count() << " microseconds.";

        cout << "\nDEBUG: VaultIndex Entries:";

        for (VaultIndexEntry entry : index) {
            cout << "\n\t" << entry.title;
        }
    #endif

    if (!index.empty()) {
        explorer.nodes = createExplorerNodesFromIndex(index);
    }
}

void populateExplorerFromContent() {
    #ifdef DEBUG_EXPLORER
        auto t1 = std::chrono::high_resolution_clock::now();
    #endif

    vector<VaultIndexEntry> index = getVaultContent(selectedVault);

    if (!index.empty()) {
        #ifdef DEBUG_EXPLORER
            auto t2 = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1);

            cout << "\nDEBUG: Time taken to get entries from content: " << duration.count() << " microseconds.";

            cout << "\nDEBUG: VaultIndex content Entries:";

            for (VaultIndexEntry entry : index) {
                cout << "\n\t" << entry.title;
            }
        #endif

        vector<ExplorerNode> newExplorerNodes = createExplorerNodesFromIndex(index);

        if (newExplorerNodes != explorer.nodes) {
            setVaultIndex(selectedVault, index);
            explorer.nodes = newExplorerNodes;
            EXPLORER.reDraw = true;
#ifdef DEBUG_EXPLORER
        } else {
            cout << "\nDEBUG: entries in index and contents in vault are same. No changes made!";
#endif
        }

        #ifdef DEBUG_EXPLORER
            auto t3 = std::chrono::high_resolution_clock::now();
            duration = std::chrono::duration_cast<std::chrono::microseconds>(t3 - t2);

            cout << "\nDEBUG: Time taken to get compare (and maybe also to assign) entries: " << duration.count();
        #endif
    }

    #ifdef DEBUG_EXPLORER
        auto t4 = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(t4 - t1);
        cout << "\nDEBUG: Total Time taken in " << __func__ << ": " << duration.count();
    #endif
}

void setupWelcomeScreen() {
    float vertices[] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };

    UIElement* title = &elements[7];
    UIElement* version = &elements[8];
    UIElement* line1 = &elements[9];
    UIElement* line2 = &elements[10];
    UIElement* line3 = &elements[11];
    UIElement* line4 = &elements[12];
    UIElement* line5 = &elements[13];
    UIElement* createVaultButton = &elements[13];
    UIElement* switchVaultButton = &elements[14];

    initElement(*title, UIElementType::Label, 0.0f, 0.0f, 0.0f, 0.0f);
    initElement(*version, UIElementType::Label, 0.0f, 0.0f, 0.0f, 0.0f);
    initElement(*line1, UIElementType::Label, 0.0f, 0.0f, 0.0f, 0.0f);

    if (darkTheme) {
        title->color = WHITE;
    } else {
        title->color = BLACK;
    }

    if (vaultSelected) {
        initElement(*line2, UIElementType::Button, 32.0f, 0.0f, 0.0f, 0.0f);
        initElement(*line3, UIElementType::Button, 4.0f, 0.0f, 0.0f, 0.0f);
        initElement(*line4, UIElementType::Button, 4.0f, 0.0f, 0.0f, 0.0f);
        initElement(*line5, UIElementType::Button, 4.0f, 0.0f, 0.0f, 0.0f);

        SETUP_BUFFERS(line2->vao, line2->vbo, vertices);
        SETUP_BUFFERS(line3->vao, line3->vbo, vertices);
        SETUP_BUFFERS(line4->vao, line4->vbo, vertices);
        SETUP_BUFFERS(line5->vao, line5->vbo, vertices);
    } else {
        initElement(*line2, UIElementType::Label, 0.0f, 0.0f, 0.0f, 0.0f);
        initElement(*line3, UIElementType::Label, 0.0f, 0.0f, 0.0f, 0.0f);
        initElement(*line4, UIElementType::Label, 0.0f, 0.0f, 0.0f, 0.0f);
        initElement(*line5, UIElementType::Label, 0.0f, 0.0f, 0.0f, 0.0f);
    }

    initElement(*createVaultButton, UIElementType::Button, 24.0f, 0.0f, 0.0f, 0.0f);
    initElement(*switchVaultButton, UIElementType::Button, 4.0f, 0.0f, 0.0f, 0.0f);

    SETUP_BUFFERS(createVaultButton->vao, createVaultButton->vbo, vertices);
    SETUP_BUFFERS(switchVaultButton->vao, switchVaultButton->vbo, vertices);

    title->text = "patram";
    title->fontSize = 48;
    title->preferences->horizontalAlign = UIAlignment::Center;
    title->preferences->widthPreference = UISizePreference::Grow;
    title->positioning = UIPositioning::Relative_sibling;

    version->text = "v0.1.0";
    version->fontSize = 16;
    version->preferences->horizontalAlign = UIAlignment::Center;
    version->preferences->widthPreference = UISizePreference::Grow;
    version->positioning = UIPositioning::Relative_sibling;

    if (vaultSelected) {
        line1->text = getDefaultVault();
        line1->fontSize = 24;
        line1->preferences->heightPreference = UISizePreference::Grow;
    } else {
        line1->text = "Welcome to patram - your lightweight notes manager!";
        line1->fontSize = 16;
    }

    line1->preferences->horizontalAlign = UIAlignment::Center;
    line1->preferences->widthPreference = UISizePreference::Grow;
    line1->positioning = UIPositioning::Relative_sibling;

    if (vaultSelected) {
        line2->text = "New note (Ctrl + N)";
        line2->preferences->heightPreference = UISizePreference::Grow;
        line2->id = NEW_NOTE_BUTTON_ID;
    } else {
        line2->text = "You begin by creating a note vault.";
    }

    line2->fontSize = 16;
    line2->preferences->horizontalAlign = UIAlignment::Center;
    line2->preferences->widthPreference = UISizePreference::Grow;
    line2->positioning = UIPositioning::Relative_sibling;

    if (vaultSelected) {
        line3->text = "New Folder (Ctrl + Shift + N)";
        line3->preferences->heightPreference = UISizePreference::Grow;
    } else {
        line3->text = "Note vaults contain notes and folders.";
    }
    line3->fontSize = 16;
    line3->preferences->horizontalAlign = UIAlignment::Center;
    line3->preferences->widthPreference = UISizePreference::Grow;
    line3->positioning = UIPositioning::Relative_sibling;

    if (vaultSelected) {
        line4->text = "Save Note (Ctrl + S)";
        line4->preferences->heightPreference = UISizePreference::Grow;
    } else {
        line4->text = "Folders can have other notes and folders.";
    }
    line4->fontSize = 16;
    line4->preferences->horizontalAlign = UIAlignment::Center;
    line4->preferences->widthPreference = UISizePreference::Grow;
    line4->positioning = UIPositioning::Relative_sibling;

    if (vaultSelected) {
        line5->text = "Toggle Explorer (Ctrl + E)";
        line5->preferences->heightPreference = UISizePreference::Grow;
    } else {
        line5->text = "You can password-protect a note vault - this encrypts all the notes in it.";
    }
    line5->fontSize = 16;
    line5->preferences->horizontalAlign = UIAlignment::Center;
    line5->preferences->widthPreference = UISizePreference::Grow;
    line5->positioning = UIPositioning::Relative_sibling;

    createVaultButton->text = "Create a note vault";
    createVaultButton->fontSize = 16;
    createVaultButton->preferences->horizontalAlign = UIAlignment::Center;
    createVaultButton->preferences->widthPreference = UISizePreference::Grow;
    createVaultButton->preferences->heightPreference = UISizePreference::Grow;
    createVaultButton->id = CREATE_VAULT_BUTTON_ID;
    createVaultButton->positioning = UIPositioning::Relative_sibling;

    switchVaultButton->text = "Switch note vault";
    switchVaultButton->fontSize = 16;
    switchVaultButton->preferences->horizontalAlign = UIAlignment::Center;
    switchVaultButton->preferences->widthPreference = UISizePreference::Grow;
    switchVaultButton->preferences->heightPreference = UISizePreference::Grow;
    switchVaultButton->id = SWITCH_VAULT_BUTTON_ID;
    switchVaultButton->positioning = UIPositioning::Relative_sibling;

    addChild(&WELCOME_MAIN_CONTAINER, title);
    addChild(&WELCOME_MAIN_CONTAINER, version);
    addChild(&WELCOME_MAIN_CONTAINER, line1);
    addChild(&WELCOME_MAIN_CONTAINER, line2);
    addChild(&WELCOME_MAIN_CONTAINER, line3);
    addChild(&WELCOME_MAIN_CONTAINER, line4);
    addChild(&WELCOME_MAIN_CONTAINER, line5);
    addChild(&WELCOME_MAIN_CONTAINER, createVaultButton);

    if (vaultSelected) {
        addChild(&WELCOME_MAIN_CONTAINER, switchVaultButton);
    }
}

void destroyWelcomeScreen() {
    UIElement* title = &elements[7];
    UIElement* version = &elements[8];
    UIElement* line1 = &elements[9];
    UIElement* line2 = &elements[10];
    UIElement* line3 = &elements[11];
    UIElement* line4 = &elements[12];
    UIElement* line5 = &elements[13];
    UIElement* createVaultButton = &elements[14];
    UIElement* switchVaultButton = &elements[15];

    for (int i=5; i<14; ++i) {
        if (elements[i].vao != 0) {
            glDeleteVertexArrays(1, &elements[i].vao);
            elements[i].vao = 0;
        }

        if (elements[i].vbo != 0) {
            glDeleteBuffers(1, &elements[i].vbo);
            elements[i].vbo = 0;
        }

        if (elements[i].textVAO != 0) {
            glDeleteVertexArrays(1, &elements[i].textVAO);
            elements[i].textVAO = 0;
        }

        if (elements[i].textVBO != 0) {
            glDeleteBuffers(1, &elements[i].textVBO);
            elements[i].textVBO = 0;
        }

        if (elements[i].preferences != nullptr) {
            delete elements[i].preferences;
            elements[i].preferences = nullptr;
        }

        if (elements[i].type == UIElementType::TextInput) {
            TextInputData* tid = elements[i].textInputData;

            glDeleteVertexArrays(1, &tid->cursorVAO);
            glDeleteBuffers(1, &tid->cursorVBO);
            glDeleteVertexArrays(1, &tid->textSelectVAO);
            glDeleteBuffers(1, &tid->textSelectVBO);
        }

        elements[i].parent = nullptr;
    }
}

inline void appendTextVerticesForExplorerNode(
    float x,
    float& y,
    vector<ExplorerNode>& nodes,
    vector<float>& vertices
) {
    float initialY = y;

    for (
        auto itr = nodes.begin();
        itr != nodes.end();
        ++itr, y += EXPLORER_ELEMENT_HEIGHT
    ) {
        appendTextVertices(x, y, (*itr).name, vertices);
        (*itr).top = y;
        bool expanded = GET_FLAG((*itr).flags, EXPLORER_FLAG_EXPANDED);
        bool empty = (*itr).children.empty();

        if (
            (*itr).type == ExplorerNodeType::Folder
            && GET_FLAG((*itr).flags, EXPLORER_FLAG_EXPANDED)
            && !(*itr).children.empty()
        ) {
            y += EXPLORER_ELEMENT_HEIGHT;
            appendTextVerticesForExplorerNode(x + 16.0f, y, (*itr).children, vertices);
        }
    }

    if (y != initialY) {
        y -= EXPLORER_ELEMENT_HEIGHT;
    }
}

void setupExplorer() {
    const float y0 = EXPLORER.top + 4.0f - EXPLORER.scrollData->yOffset;
    float y = y0;
    vector<float> vertices;

    vertices.reserve(64000);

    appendTextVerticesForExplorerNode(16.0f, y, explorer.nodes, vertices);
    float scrollHeight = y - y0 + 32.0f;

    EXPLORER.scrollData->height = scrollHeight;
    EXPLORER.scrollData->showVerticalScrollbar = scrollHeight > EXPLORER.height;

    glBindVertexArray(explorer.vao);
    glBindBuffer(GL_ARRAY_BUFFER, explorer.vbo);

    uint32_t currentSize = vertices.size();

    if (currentSize <= explorer.textVerCap) {
        vector<float> zeros(explorer.textVerCap - vertices.size(), 0);
        move(zeros.begin(), zeros.end(), back_inserter(vertices));

        glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(float), &vertices[0]);
    } else {
        while (currentSize > explorer.textVerCap) { explorer.textVerCap *= 1.5; }

        vector<float> zeros(explorer.textVerCap - currentSize, 0);
        move(zeros.begin(), zeros.end(), back_inserter(vertices));

        glBufferData(GL_ARRAY_BUFFER, explorer.textVerCap * sizeof(float), &vertices[0], GL_DYNAMIC_DRAW);
        explorer.textVerCap = vertices.size();
    }

    vertices.reserve(0);

    explorer.textVertices = vertices.size();
}

void setupUI(bool isDarkTheme) {
    windowHeight = WINDOW_HEIGHT;
    windowWidth = WINDOW_WIDTH;
    resolution = glm::vec2(windowWidth, windowHeight);

    textCursor = glfwCreateStandardCursor(GLFW_IBEAM_CURSOR);
    pointerCursor = glfwCreateStandardCursor(GLFW_HAND_CURSOR);
    hResizeCursor = glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR);

    initShaders();
    initTextRendering(windowWidth, windowHeight);

    glEnable(GL_BLEND);
    glEnable(GL_SAMPLE_SHADING);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    float vertices[] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };

    SETUP_BUFFERS(highlightedContentBlock.backgroundVAO, highlightedContentBlock.backgroundVBO, vertices);
    SETUP_BUFFERS(highlightedContentBlock.dragTabVAO, highlightedContentBlock.dragTabVBO, vertices);

    initElement(ROOT_CONTAINER, UIElementType::Container, 0.0f, 0.0f, WINDOW_WIDTH, WINDOW_HEIGHT);
    initElement(CONTENT_CONTAINER, UIElementType::ContentContainer, TAB_BAR_HEIGHT, 0.0f, WINDOW_WIDTH, WINDOW_HEIGHT - 64.0f);
    initElement(TAB_BAR, UIElementType::TabBar, 0.0f, 0.0f, WINDOW_WIDTH, TAB_BAR_HEIGHT);
    initElement(WELCOME_SCREEN, UIElementType::Container, 0.0f, 0.0f, WINDOW_WIDTH, WINDOW_HEIGHT);
    initElement(MAIN_CONTAINER, UIElementType::Container, TAB_BAR_HEIGHT, 0.0f, WINDOW_WIDTH, WINDOW_HEIGHT - TAB_BAR_HEIGHT);
    initElement(WELCOME_MAIN_CONTAINER, UIElementType::Container, 100.0f, 100.0f, WINDOW_WIDTH - 100.0f, WINDOW_HEIGHT - 100.0f);
    initElement(EXPLORER, UIElementType::Explorer, TAB_BAR_HEIGHT, 0.0f, 200.0f, WINDOW_HEIGHT - TAB_BAR_HEIGHT);

    TAB_BAR.show = false;
    CONTENT_CONTAINER.show = false;

    SETUP_BUFFERS(ROOT_CONTAINER.vao, ROOT_CONTAINER.vbo, vertices);
    SETUP_BUFFERS(CONTENT_CONTAINER.vao, CONTENT_CONTAINER.vbo, vertices);
    SETUP_BUFFERS(TAB_BAR.vao, TAB_BAR.vbo, vertices);
    SETUP_BUFFERS(EXPLORER.vao, EXPLORER.vbo, vertices);

    float verts[2400];
    memset(verts, 0, sizeof(verts));
    glGenVertexArrays(1, &explorer.vao);
    glBindVertexArray(explorer.vao);
    glGenBuffers(1, &explorer.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, explorer.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    explorer.textVerCap = 2400;

    SETUP_BUFFERS_DYNAMIC(explorer.lineVAO, explorer.lineVBO, vertices);
    SETUP_BUFFERS_DYNAMIC(explorer.hoverBgVao, explorer.hoverBgVbo, vertices);
    SETUP_BUFFERS_DYNAMIC(explorer.activeBgVao, explorer.activeBgVbo, vertices);
    SETUP_BUFFERS_DYNAMIC(explorer.verticalScrollVao, explorer.verticalScrollVbo, vertices);
    SETUP_BUFFERS_DYNAMIC(explorer.horizontalScrollVao, explorer.horizontalScrollVao, vertices);

    for (int i=0; i<100; ++i) {
        tabsDisplay[i].vao = 0;
        tabsDisplay[i].vbo = 0;
        tabsDisplay[i].activeBgVao = 0;
        tabsDisplay[i].activeBgVbo = 0;
        tabsDisplay[i].vbo = 0;
        tabsDisplay[i].bytes = 0;
        tabsDisplay[i].numVertices = 0;
        tabsDisplay[i].width = 0.0f;
    }

    if (darkTheme) {
        ROOT_CONTAINER.backgroundColor = ROOT_BG_DARK;
        TAB_BAR.backgroundColor = TOOL_BAR_BG_DARK;
        WELCOME_SCREEN.backgroundColor = TOOL_BAR_BG_DARK;
    } else {
        ROOT_CONTAINER.backgroundColor = ROOT_BG;
        TAB_BAR.backgroundColor = TOOL_BAR_BG;
        WELCOME_SCREEN.backgroundColor = TOOL_BAR_BG;
    }

    ROOT_CONTAINER.text = "Root Container";
    ROOT_CONTAINER.preferences->widthPreference = UISizePreference::Full;
    ROOT_CONTAINER.preferences->heightPreference = UISizePreference::Full;
    ROOT_CONTAINER.preferences->direction = false;

    TAB_BAR.text = "Tab Bar";
    TAB_BAR.preferences->widthPreference = UISizePreference::Full;
    TAB_BAR.preferences->heightPreference = UISizePreference::Fixed;
    TAB_BAR.preferences->verticalAlign = UIAlignment::Start;
    TAB_BAR.preferences->horizontalAlign = UIAlignment::Center;

    MAIN_CONTAINER.text = "MAIN_CONTAINER";
    MAIN_CONTAINER.preferences->widthPreference = UISizePreference::Full;
    MAIN_CONTAINER.preferences->heightPreference = UISizePreference::Stretch;
    MAIN_CONTAINER.preferences->verticalAlign = UIAlignment::End;

    EXPLORER.text = "EXPLORER";
    EXPLORER.show = false;

    CONTENT_CONTAINER.text = "CONT_CONTAINER";
    CONTENT_CONTAINER.scrollData->width = WINDOW_WIDTH;
    CONTENT_CONTAINER.scrollData->height = WINDOW_HEIGHT - 64;
    CONTENT_CONTAINER.preferences->widthPreference = UISizePreference::Stretch;
    CONTENT_CONTAINER.preferences->heightPreference = UISizePreference::Stretch;
    CONTENT_CONTAINER.preferences->verticalAlign = UIAlignment::End;
    CONTENT_CONTAINER.preferences->horizontalAlign = UIAlignment::End;
    CONTENT_CONTAINER.active = true;

    WELCOME_SCREEN.text = "Welcome Screen";
    WELCOME_SCREEN.preferences->widthPreference = UISizePreference::Stretch;
    WELCOME_SCREEN.preferences->heightPreference = UISizePreference::Full;
    WELCOME_SCREEN.preferences->horizontalAlign = UIAlignment::End;
    WELCOME_SCREEN.preferences->direction = false;

    WELCOME_MAIN_CONTAINER.preferences->verticalAlign = UIAlignment::Center;
    WELCOME_MAIN_CONTAINER.preferences->widthPreference = UISizePreference::Full;
    WELCOME_MAIN_CONTAINER.preferences->heightPreference = UISizePreference::Grow;
    WELCOME_MAIN_CONTAINER.preferences->direction = false;
    WELCOME_MAIN_CONTAINER.text = "WEL_MAIN_CONT";

    #ifdef DEBUG_TEST_NOTE_TAB
    newTab("Test note title");

    updateWindowTitle();

    appendContentBlock(0, ContentBlockType::Paragraph, "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Suspendisse accumsan lobortis sapien a facilisis. Fusce volutpat odio at purus ullamcorper, a feugiat leo vulputate. Etiam et tortor odio.");
    appendContentBlock(0, ContentBlockType::Heading1, "Vivamus scelerisque ac lorem quis lacinia.");
    appendContentBlock(0, ContentBlockType::Paragraph, "Etiam at erat vel ex placerat auctor at quis arcu. Nullam tempus arcu facilisis nisl sodales molestie. Curabitur enim nisi, aliquam sed risus id, ultricies fringilla nisl. Integer accumsan tincidunt pulvinar. Lorem ipsum dolor sit amet, consectetur adipiscing elit. Cras lacinia convallis velit, eu malesuada metus. Proin posuere gravida eros, a iaculis magna porta a.");
    appendContentBlock(0, ContentBlockType::Heading2, "Praesent rutrum odio in neque commodo pretium.");
    appendContentBlock(0, ContentBlockType::Paragraph, "Etiam auctor est sed sagittis ullamcorper. Cras suscipit eu ligula eu feugiat. Nulla vitae nibh justo. Curabitur pretium urna quis nisi ornare imperdiet. Donec diam diam, rutrum sed massa eu, tristique volutpat felis. Morbi pulvinar ante leo, eu tristique velit imperdiet sit amet. Praesent venenatis bibendum erat quis lacinia. Interdum et malesuada fames ac ante ipsum primis in faucibus. Pellentesque porta convallis rutrum.");
    appendContentBlock(0, ContentBlockType::Heading3, "Aenean porta tortor in tellus blandit consectetur.");
    appendContentBlock(0, ContentBlockType::Paragraph, "Pellentesque ac tellus maximus, scelerisque risus sit amet, consequat justo. Phasellus lobortis massa quis ipsum eleifend imperdiet. Aliquam in convallis tellus, sit amet pharetra diam. Praesent eu sagittis nunc, scelerisque maximus enim. Vivamus varius, elit sed rutrum dapibus, ante purus sodales lorem, vitae malesuada risus dolor quis leo. Proin porta ex orci, ac vulputate purus pulvinar sed. Curabitur non porta sapien. Mauris tortor magna, congue ac volutpat vel, aliquam eget elit. Nulla ullamcorper tempus felis, et fermentum dolor condimentum a. Nullam rhoncus cursus consequat.");
    activeElement = &CONTENT_CONTAINER;
    #endif

    if (defaultVaultExists()) {
        vaultSelected = true;
        selectedVault = getDefaultVault();
        setupWelcomeScreen();
    } else {
        if (!getVaults().empty()) {
            onSwitchVaultClicked();
        } else {
            setupWelcomeScreen();
        }
    }

    populateExplorerFromIndex();
    thread explorerLoadThread(populateExplorerFromContent);

    EXPLORER.backgroundColor = darkTheme ? EXPLORER_BG_DARK : EXPLORER_BG;
    EXPLORER.preferences->widthPreference = UISizePreference::Fixed;
    EXPLORER.preferences->heightPreference = UISizePreference::Full;

    addChild(&WELCOME_SCREEN, &WELCOME_MAIN_CONTAINER);

    addChild(&MAIN_CONTAINER, &EXPLORER);
    addChild(&MAIN_CONTAINER, &WELCOME_SCREEN);
    addChild(&MAIN_CONTAINER, &CONTENT_CONTAINER);

    addChild(&ROOT_CONTAINER, &TAB_BAR);
    addChild(&ROOT_CONTAINER, &MAIN_CONTAINER);

    initFontTexture(getFontAtlasPath(0).string());

    #ifdef DEBUG_CONTENT_BLOCKS
        printContentBlocks(0);
    #endif

    explorerLoadThread.join();
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

        float _ = 0.0f;

        renderElement(ROOT_CONTAINER, resolution, cursor, 0, _);

        #ifdef DEBUG_TEXT
            if (glfwGetTime() - prevTextTime >= 1.0f) {
                cout << "\nDEBUG: Average text render time per second: " << cumulativeTextTime / textTimeCalculated << " milliseconds";

                textTime = 0.0f;
                cumulativeTextTime = 0.0f;
                textTimeCalculated = 0;
                prevTextTime = glfwGetTime();
            }
        #endif

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

void defaultCallback(UIElement* _elem) {
    cout << "\nDefault callback executed!";
}

void defaultKeyCallback(UIElement* elem, int key, int action, int mod) {
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        switch (key) {
            case GLFW_KEY_N:
                if (mod & GLFW_MOD_CONTROL) {
                    if (tabs.size() < 100) {
                        newNote();
                    } else {
                        // TODO: display a warning here...
                    }
                }
                break;

            case GLFW_KEY_E:
                if (mod & GLFW_MOD_CONTROL) {
                    toggleExplorer();
                }
                break;
        }
    }
}

void defaultOnMouseUpDownCallback(UIElement*, float, float, glm::vec2) {
    cout << "\nDefault mouse up/down callback executed!";
}

void cbKeyCallback(int key, int action, int mod) {
    if (tabs.empty()) {
        return;
    }
    ContentTab* tab = &tabs[activeTabIndex];
    ContentBlock* cb = &tab->contentBlocks[tab->cursorCBIndex];

    bool alt = mod & GLFW_MOD_ALT;
    bool shift = mod & GLFW_MOD_SHIFT;
    bool ctrl = mod & GLFW_MOD_CONTROL;
    bool capsLck = mod & GLFW_MOD_CAPS_LOCK;
    bool numLck = mod & GLFW_MOD_NUM_LOCK;

    #ifdef DEBUG_TEXT_INPUT
    #ifdef DEBUG_KEYBOARD_EVENTS
        cout << "\n Key: ";

        if (alt) {
            cout << "Alt + ";
        }

        if (shift) {
            cout << "Shift + ";
        }

        if (ctrl) {
            cout << "Control + ";
        }

        if (capsLck) {
            cout << "Caps Lock + ";
        }

        if (numLck) {
            cout << "Num Lock + ";
        }

        cout << key;
    #endif
    #endif

    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        #define TYPE(c) cbKeypressType(tab, c)

        #define BASIC_CMD(cmdType) ContentCommand {\
                cmdType, false, 0, tab->cursorCBIndex, tab->cursorPosition, ""\
            }
        #define INCR_CURSOR() if (tab->cursorPosition < cb->rope->length()) { tab->cursorPosition++; }
        #define DECR_CURSOR() if (tab->cursorPosition > 0) { tab->cursorPosition--; }
        #define CUR_POS tab->cursorPosition
        #define UPDATE() tab->lastCursorTime = static_cast<float>(glfwGetTime()); tab->redrawCursor = true
        #define UNSELECT() tab->textSelected = false;\
            tab->selStartCBIndex = 0;\
            tab->selStartCursorPosition = 0;\
            tab->selEndCBIndex = 0;\
            tab->selEndCursorPosition = 0

        switch (key) {
            case GLFW_KEY_ESCAPE:
                if (tab->textSelected) {
                    UNSELECT();
                } else {
                    tab->showCursor = false;
                }
                break;

            case GLFW_KEY_LEFT:
                if (ctrl) {
                    uint32_t i = cb->rope->length();
                    uint32_t cumulativeLen = i;
                    --i;

                    ReverseRopeTraverser rt(cb->rope);

                    bool found = false;

                    while (rt.hasNext()) {
                        Rope* rope = rt.next();

                        if (cumulativeLen - rope->length() > tab->cursorPosition) {
                            cumulativeLen -= rope->length();
                            continue;
                        }

                        if (cumulativeLen >= 0) {
                            i = cumulativeLen - 1;
                        }

                        for (
                            string::reverse_iterator c = rope->text.rbegin();
                            c != rope->text.rend();
                            ++c
                        ) {
                            if (i >= tab->cursorPosition) {
                                --i;
                                continue;
                            }

                            if (*c == ' ') {
                                found = true;
                                break;
                            }

                            if (!isalnum(*c)) {
                                found = true;
                                break;
                            }

                            --i;
                        }

                        if (found) {
                            tab->cursorPosition = i;
                            #ifdef DEBUG_TEXT_INPUT
                                cout << "\nCTRL + LEFT: cursorPosition = " << i;
                            #endif
                            break;
                        }
                    }

                    if (!found) {
                        if (tab->cursorPosition > 0) {
                            tab->cursorPosition = 0;
                        } else if (tab->cursorPosition == 0) {
                            UNSELECT();

                            if (tab->cursorPosition == 0 && tab->cursorCBIndex > 0) {
                                --tab->cursorCBIndex;
                                ContentBlock* newCB = &tab->contentBlocks[tab->cursorCBIndex];

                                tab->cursorPosition = newCB->rope->length();
                                tab->cursorRow = newCB->rows - 1;
                            } else {
                                DECR_CURSOR();
                            }
                        }
                    }
                } else if (shift) {
                    if (tab->textSelected) {
                        if (
                            tab->cursorPosition - 1 == tab->initialCursorPosition
                            && tab->cursorCBIndex == tab->selStartCBIndex
                            && tab->cursorCBIndex == tab->selEndCBIndex
                        ) {
                            tab->textSelected = false;
                            --tab->cursorPosition;
                        } else if (tab->selStartCBIndex != tab->selEndCBIndex) {
                            if (tab->cursorCBIndex > tab->initialCBIndex) {
                                if (tab->selEndCursorPosition == 0 && tab->cursorCBIndex > 0) {
                                    --tab->cursorCBIndex;
                                    --tab->selEndCBIndex;
                                    tab->selEndCursorPosition = tab->contentBlocks[tab->cursorCBIndex].rope->length();
                                    tab->cursorPosition = tab->selEndCursorPosition;
                                } else if (tab->selEndCursorPosition > 0) {
                                    --tab->selEndCursorPosition;
                                    --tab->cursorPosition;
                                }
                            } else {
                                if (tab->selStartCursorPosition == 0 && tab->cursorCBIndex > 0) {
                                    --tab->cursorCBIndex;
                                    --tab->selStartCBIndex;
                                    tab->selStartCursorPosition = tab->contentBlocks[tab->cursorCBIndex].rope->length();
                                    tab->cursorPosition = tab->selEndCursorPosition;
                                } else if (tab->selStartCursorPosition > 0) {
                                    --tab->selStartCursorPosition;
                                    --tab->cursorPosition;
                                }
                            }
                        } else {
                            if (tab->cursorPosition > tab->initialCursorPosition) {
                                if (tab->selEndCursorPosition == 0 && tab->cursorCBIndex > 0) {
                                    --tab->cursorCBIndex;
                                    --tab->selEndCBIndex;
                                    tab->selEndCursorPosition = tab->contentBlocks[tab->cursorCBIndex].rope->length();
                                    tab->cursorPosition = tab->selEndCursorPosition;
                                } else if (tab->selEndCursorPosition > 0) {
                                    --tab->selEndCursorPosition;
                                    --tab->cursorPosition;
                                }
                            } else {
                                if (tab->selStartCursorPosition == 0 && tab->cursorCBIndex > 0) {
                                    --tab->cursorCBIndex;
                                    --tab->selStartCBIndex;
                                    tab->selStartCursorPosition = tab->contentBlocks[tab->cursorCBIndex].rope->length();
                                    tab->cursorPosition = tab->selEndCursorPosition;
                                } else if (tab->selStartCursorPosition > 0) {
                                    --tab->selStartCursorPosition;
                                    --tab->cursorPosition;
                                }
                            }
                        }
                    } else {
                        tab->textSelected = true;
                        tab->initialCursorPosition = tab->cursorPosition;
                        tab->selStartCursorPosition = tab->cursorPosition - 1;
                        tab->selEndCursorPosition = tab->cursorPosition;
                        tab->selStartCBIndex = tab->cursorCBIndex;
                        tab->selEndCBIndex = tab->cursorCBIndex;
                        tab->initialCBIndex = tab->cursorCBIndex;

                        if (tab->cursorPosition > 0) {
                            --tab->cursorPosition;
                        }
                    }

                    tab->redrawSel = true;
                } else {
                    UNSELECT();

                    if (tab->cursorPosition == 0 && tab->cursorCBIndex > 0) {
                        --tab->cursorCBIndex;
                        ContentBlock* newCB = &tab->contentBlocks[tab->cursorCBIndex];

                        tab->cursorPosition = newCB->rope->length();
                        tab->cursorRow = newCB->rows - 1;
                    } else {
                        DECR_CURSOR();
                    }
                }

                tab->updateCursorX = true;
                UPDATE();
                break;

            case GLFW_KEY_RIGHT:
                if (ctrl) {
                    uint32_t i = 0;
                    uint32_t cumulativeLen = 0;

                    RopeTraverser rt(cb->rope);

                    bool found = false;

                    while (rt.hasNext()) {
                        Rope* rope = rt.next();

                        if (cumulativeLen + rope->length() < tab->cursorPosition) {
                            cumulativeLen += rope->length();
                            continue;
                        }

                        if (cumulativeLen > 0) {
                            i = cumulativeLen - 1;
                        }

                        for (char c: rope->text) {
                            if (i <= tab->cursorPosition) {
                                ++i;
                                continue;
                            }

                            if (c == ' ') {
                                ++i;
                                found = true;
                                break;
                            }

                            if (!isalnum(c)) {
                                // --i;
                                found = true;
                                break;
                            }

                            ++i;
                        }

                        if (found) {
                            tab->cursorPosition = i;
                            #ifdef DEBUG_TEXT_INPUT
                                cout << "\nCTRL + RIGHT: cursorPosition = " << i;
                            #endif
                            break;
                        }
                    }

                    if (!found && tab->cursorPosition <= cb->rope->length()) {
                        UNSELECT();

                        if (
                            tab->cursorPosition == cb->rope->length()
                            && tab->cursorCBIndex < tab->contentBlocks.size() - 1
                        ) {
                            tab->cursorPosition = 0;
                            tab->cursorRow = 0;
                            ++tab->cursorCBIndex;
                        } else {
                            INCR_CURSOR();
                        }
                    }
                } else if (shift) {
                    if (tab->textSelected) {
                        if (
                            tab->cursorPosition + 1 == tab->initialCursorPosition
                            && tab->cursorCBIndex == tab->selStartCBIndex
                            && tab->cursorCBIndex == tab->selEndCBIndex
                        ) {
                            tab->textSelected = false;
                            ++tab->cursorPosition;
                        } else if (tab->selStartCBIndex != tab->selEndCBIndex) {
                            if (tab->cursorCBIndex > tab->initialCBIndex) {
                                if (
                                    tab->selEndCursorPosition == cb->rope->length()
                                    && tab->cursorCBIndex < tabs.size() - 1
                                ) {
                                    ++tab->cursorCBIndex;
                                    ++tab->selEndCBIndex;
                                    tab->selEndCursorPosition = 0;
                                    tab->cursorPosition = 0;
                                } else if (tab->selEndCursorPosition < cb->rope->length()) {
                                    ++tab->selEndCursorPosition;
                                    ++tab->cursorPosition;
                                }
                            } else {
                                if (
                                    tab->selStartCursorPosition == cb->rope->length()
                                    && tab->cursorCBIndex < tabs.size() - 1
                                ) {
                                    ++tab->cursorCBIndex;
                                    ++tab->selStartCBIndex;
                                    tab->selStartCursorPosition = 0;
                                    tab->cursorPosition = 0;
                                } else if (tab->selStartCursorPosition < cb->rope->length()) {
                                    ++tab->selStartCursorPosition;
                                    ++tab->cursorPosition;
                                }
                            }
                        } else {
                            if (tab->cursorPosition > tab->initialCursorPosition) {
                                if (
                                    tab->selEndCursorPosition == cb->rope->length()
                                    && tab->cursorCBIndex > tabs.size() - 1
                                ) {
                                    ++tab->cursorCBIndex;
                                    ++tab->selEndCBIndex;
                                    tab->selEndCursorPosition = 0;
                                    tab->cursorPosition = 0;
                                } else if (tab->selEndCursorPosition < cb->rope->length()) {
                                    ++tab->selEndCursorPosition;
                                    ++tab->cursorPosition;
                                }
                            } else {
                                if (
                                    tab->selStartCursorPosition == cb->rope->length()
                                    && tab->cursorCBIndex < tabs.size() - 1
                                ) {
                                    ++tab->cursorCBIndex;
                                    ++tab->selStartCBIndex;
                                    tab->selStartCursorPosition = 0;
                                    tab->cursorPosition = 0;
                                } else if (tab->selStartCursorPosition < cb->rope->length()) {
                                    ++tab->selStartCursorPosition;
                                    ++tab->cursorPosition;
                                }
                            }
                        }
                    } else {
                        tab->textSelected = true;
                        tab->initialCursorPosition = tab->cursorPosition;
                        tab->selStartCursorPosition = tab->cursorPosition;
                        tab->selEndCursorPosition = ++tab->cursorPosition;
                        tab->selStartCBIndex = tab->cursorCBIndex;
                        tab->selEndCBIndex = tab->cursorCBIndex;
                        tab->initialCBIndex = tab->cursorCBIndex;
                    }

                    tab->redrawSel = true;
                } else {
                    UNSELECT();

                    if (
                        tab->cursorPosition == cb->rope->length()
                        && tab->cursorCBIndex < tab->contentBlocks.size() - 1
                    ) {
                        tab->cursorPosition = 0;
                        tab->cursorRow = 0;
                        ++tab->cursorCBIndex;
                    } else {
                        INCR_CURSOR();
                    }
                }

                tab->updateCursorX = true;
                UPDATE();
                break;

            case GLFW_KEY_UP: {
                    SelectionData sd = getCBTextCursorVertical(tab, false, true);

                    #ifdef DEBUG_TEXT_INPUT
                        cout << "\nSelectionData:  cursorPosition: " << sd.cursorPosition
                            << ", row: " << sd.row << ", col: " << sd.col;
                    #endif

                    if (shift) {
                        if (sd.cursorCBIndex == 0) {
                            tab->textSelected = false;
                            tab->cursorPosition = sd.cursorPosition;
                            tab->cursorCBIndex = 0;
                        } else if (sd.cursorCBIndex != tab->cursorCBIndex) {
                            if (tab->textSelected) {
                                if (sd.cursorCBIndex < tab->initialCBIndex) {
                                    tab->selStartCBIndex = sd.cursorCBIndex;
                                    tab->selStartCursorPosition = sd.cursorPosition;
                                    tab->selEndCBIndex = tab->initialCBIndex;
                                } else {
                                    tab->selStartCBIndex = tab->initialCBIndex;
                                    tab->selEndCBIndex = sd.cursorCBIndex;
                                    tab->selEndCursorPosition = sd.cursorPosition;
                                }
                            } else {
                                tab->textSelected = true;

                                tab->selStartCBIndex = sd.cursorCBIndex;
                                tab->selStartCursorPosition = sd.cursorPosition;
                                tab->selEndCBIndex = tab->cursorCBIndex;
                                tab->selEndCursorPosition = tab->cursorPosition;
                                tab->initialCBIndex = tab->cursorCBIndex;
                                tab->initialCursorPosition = tab->cursorPosition;
                            }
                        } else if (sd.cursorPosition != tab->cursorPosition) {
                            if (tab->textSelected) {
                                if (tab->selStartCBIndex == tab->selEndCBIndex) {
                                    if (sd.cursorPosition == tab->initialCursorPosition) {
                                        tab->textSelected = false;
                                    } else {
                                        if (sd.cursorPosition < tab->initialCursorPosition) {
                                            tab->selStartCursorPosition = sd.cursorPosition;
                                            tab->selEndCursorPosition = tab->initialCursorPosition;
                                        } else {
                                            tab->selStartCursorPosition = tab->initialCursorPosition;
                                            tab->selEndCursorPosition = sd.cursorPosition;
                                        }
                                    }
                                } else {
                                    if (tab->selStartCBIndex == sd.cursorCBIndex) {
                                        tab->selStartCursorPosition = sd.cursorPosition;
                                    } else if (tab->selEndCBIndex == sd.cursorCBIndex) {
                                        tab->selEndCursorPosition = sd.cursorPosition;
                                    } else {
                                        #ifdef DEBUG_TEXT_INPUT
                                        cout << "\n\nBhai is condition ke bare me toh socha hi nahi thaa!!";
                                        #endif
                                    }
                                }
                            } else {
                                tab->textSelected = true;
                                tab->initialCursorPosition = tab->cursorPosition;
                                tab->initialCBIndex = tab->cursorCBIndex;

                                tab->selStartCBIndex = sd.cursorCBIndex;
                                tab->selEndCBIndex = tab->cursorCBIndex;
                                tab->selStartCursorPosition = sd.cursorPosition;
                                tab->selEndCursorPosition = tab->cursorPosition;
                                tab->initialCBIndex = tab->cursorCBIndex;
                            }
                        }

                        tab->redrawSel = true;
                    } else {
                        UNSELECT();
                    }

                    updateCBCursor(tab, sd);
                }
                break;

            case GLFW_KEY_DOWN: {
                    SelectionData sd = getCBTextCursorVertical(tab, false, false);

                    #ifdef DEBUG_TEXT_INPUT
                    cout << "\nSelectionData:  cursorPosition: " << sd.cursorPosition
                        << ", row: " << sd.row << ", col: " << sd.col;
                    #endif

                    if (shift) {
                        if (sd.cursorCBIndex != tab->cursorCBIndex) {
                            cbTextDragging = true;

                            if (tab->textSelected) {
                                if (sd.cursorCBIndex == tab->initialCBIndex) {
                                    tab->selStartCBIndex = sd.cursorCBIndex;
                                    tab->selEndCBIndex = tab->initialCBIndex;
                                    tab->selStartCursorPosition = sd.cursorPosition;
                                } else if (sd.cursorCBIndex < tab->initialCBIndex) {
                                    tab->selStartCBIndex = sd.cursorCBIndex;
                                    tab->selEndCBIndex = tab->initialCBIndex;
                                    tab->selStartCursorPosition = sd.cursorPosition;
                                } else {
                                    tab->selStartCBIndex = tab->initialCBIndex;
                                    tab->selEndCBIndex = sd.cursorCBIndex;
                                    tab->selEndCursorPosition = sd.cursorPosition;
                                }
                            } else {
                                tab->textSelected = true;

                                tab->selStartCBIndex = tab->cursorCBIndex;
                                tab->selStartCursorPosition = tab->cursorPosition;
                                tab->selEndCBIndex = sd.cursorCBIndex;
                                tab->selEndCursorPosition = sd.cursorPosition;
                                tab->initialCBIndex = tab->cursorCBIndex;
                                tab->initialCursorPosition = tab->cursorPosition;
                            }

                            tab->cursorCBIndex = sd.cursorCBIndex;
                            tab->cursorPosition = sd.cursorPosition;
                        } else if (sd.cursorPosition != tab->cursorPosition) {
                            cbTextDragging = true;

                            if (tab->textSelected) {
                                if (tab->selStartCBIndex == tab->selEndCBIndex) {
                                    if (sd.cursorPosition == tab->initialCursorPosition) {
                                        tab->textSelected = false;
                                    } else {
                                        if (sd.cursorPosition < tab->initialCursorPosition) {
                                            tab->selStartCursorPosition = sd.cursorPosition;
                                            tab->selEndCursorPosition = tab->initialCursorPosition;
                                        } else {
                                            tab->selStartCursorPosition = tab->initialCursorPosition;
                                            tab->selEndCursorPosition = sd.cursorPosition;
                                        }
                                    }
                                } else {
                                    if (tab->selStartCBIndex == sd.cursorCBIndex) {
                                        tab->selStartCursorPosition = sd.cursorPosition;
                                    } else if (tab->selEndCBIndex == sd.cursorCBIndex) {
                                        tab->selEndCursorPosition = sd.cursorPosition;
                                    } else {
                                        #ifdef DEBUG_TEXT_INPUT
                                        cout << "\n\nBhai is condition ke bare me toh socha hi nahi thaa!!";
                                        #endif
                                    }
                                }
                            } else {
                                tab->textSelected = true;
                                tab->initialCursorPosition = tab->cursorPosition;
                                tab->initialCBIndex = tab->cursorCBIndex;

                                tab->selStartCBIndex = tab->cursorCBIndex;
                                tab->selEndCBIndex = sd.cursorCBIndex;
                                tab->selStartCursorPosition = tab->cursorPosition;
                                tab->selEndCursorPosition = sd.cursorPosition;
                                tab->initialCBIndex = tab->cursorCBIndex;
                            }

                            tab->cursorCBIndex = sd.cursorCBIndex;
                            tab->cursorPosition = sd.cursorPosition;
                            tab->updateCursorX = true;
                            tab->redrawSel = true;
                        }

                        tab->redrawSel = true;
                    } else {
                        UNSELECT();
                    }

                    updateCBCursor(tab, sd);
                }
                break;

            case GLFW_KEY_TAB:
                if (ctrl && shift) {
                    if (activeTabIndex > 0) {
                        --activeTabIndex;
                    } else {
                        activeTabIndex = tabs.size() - 1;
                    }
                } else if (ctrl) {
                    activeTabIndex = (activeTabIndex + 1) % tabs.size();
                }

                updateWindowTitle();
                break;

            case GLFW_KEY_ENTER:
                #ifdef DEBUG_TEXT_INPUT
                #ifdef DEBUG_KEYBOARD_EVENTS
                    cout << "\nEnter pressed";
                #endif
                #endif

                processCBCommand(BASIC_CMD(CommandType::NewLine));
                tab->redrawCursor = true;
                break;

            case GLFW_KEY_END:
                CUR_POS = static_cast<uint32_t>(cb->rope->length());
                tab->updateCursorX = true;
                UPDATE();
                break;

            case GLFW_KEY_HOME:
                CUR_POS = 0;
                tab->updateCursorX = true;
                UPDATE();
                break;

            case GLFW_KEY_PAGE_UP: {
                    glm::vec2 dim = getActiveContentDimensions();

                    if (tab->scrollOffsetY - dim.y > 0) {
                        tab->scrollOffsetY -= dim.y;
                    } else {
                        tab->scrollOffsetY = 0;
                    }

                    vector<ContentBlock>::iterator fvcb = getFirstVisibleCB(tab);

                    if (fvcb != tab->contentBlocks.begin()) {
                        int64_t dist = distance(tab->contentBlocks.begin(), fvcb) - 1;

                        if (dist > -1) {
                            tab->cursorCBIndex = static_cast<uint32_t>(dist);
                        } else {
                            tab->cursorCBIndex = 0;
                        }
                    }

                    SelectionData sd = getCBTextCursorVertical(tab, true, true);
                    updateCBCursor(tab, sd);

                    tab->scrollbarUpdated = false;
                    highlightedContentBlock.show = false;
                    UPDATE();
                }
                break;

            case GLFW_KEY_PAGE_DOWN: {
                    glm::vec2 dim = getActiveContentDimensions();

                    if (tab->scrollOffsetY + dim.y < tab->maxHeight - dim.y) {
                        tab->scrollOffsetY += dim.y;
                    } else {
                        tab->scrollOffsetY = tab->maxHeight - CONTENT_CONTAINER.height;
                    }

                    vector<ContentBlock>::iterator fvcb = getLastVisibleCB(tab);

                    if (fvcb != tab->contentBlocks.begin()) {
                        int64_t dist = distance(tab->contentBlocks.begin(), fvcb) + 1;

                        if (dist < tab->contentBlocks.size()) {
                            tab->cursorCBIndex = static_cast<uint32_t>(dist);
                        } else {
                            tab->cursorCBIndex = tab->contentBlocks.size() - 1;
                        }
                    }

                    SelectionData sd = getCBTextCursorVertical(tab, true, false);
                    updateCBCursor(tab, sd);

                    tab->scrollbarUpdated = false;
                    highlightedContentBlock.show = false;
                    UPDATE();
                }
                break;

            case GLFW_KEY_BACKSPACE:
                if (
                    CUR_POS <= cb->rope->length() && CUR_POS > 0
                    || tab->textSelected
                ) {
                    if (tab->textSelected){
                        tab->extCommands.push_back(ContentCommandExt {
                            ContentDeletionType::None,
                            tab->selEndCBIndex,
                            tab->selEndCursorPosition,
                            vector<ContentBlock>(),
                            vector<ContentBlock>(),
                        });

                        processCBCommand(ContentCommand {
                            CommandType::DeleteSelected,
                            true,
                            static_cast<uint16_t>(tab->extCommands.size() - 1),
                            tab->selStartCBIndex,
                            tab->selStartCursorPosition,
                            ""
                        });

                        tab->cursorPosition = tab->selStartCursorPosition;
                        tab->cursorCBIndex = tab->selStartCBIndex;
                        UNSELECT();
                    } else {
                        processCBCommand(ContentCommand {
                            CommandType::Backspace,
                            false,
                            0,
                            tab->cursorCBIndex, CUR_POS,
                            string(1, (*cb->rope)[CUR_POS - 1])
                        });

                        if (CUR_POS != 0) {
                            DECR_CURSOR();
                        }
                    }

                    UPDATE();
                } else if (CUR_POS == 0) {
                    if (tab->textSelected) {
                        tab->extCommands.push_back(ContentCommandExt {
                            ContentDeletionType::None,
                            tab->selEndCBIndex,
                            tab->selEndCursorPosition,
                            vector<ContentBlock>(),
                            vector<ContentBlock>(),
                        });

                        processCBCommand(ContentCommand {
                            CommandType::DeleteSelected,
                            true,
                            static_cast<uint16_t>(tab->extCommands.size() - 1),
                            tab->selStartCBIndex,
                            tab->selStartCursorPosition,
                            ""
                        });

                        tab->cursorPosition = tab->selStartCursorPosition;
                        tab->cursorCBIndex = tab->selStartCBIndex;
                        UNSELECT();
                    } else {
                        if (tab->cursorCBIndex > 1) {
                            vector<ContentBlock> cbs;
                            ContentBlock cb = tab->contentBlocks[tab->cursorCBIndex];
                            cbs.push_back(cb);

                            tab->extCommands.push_back(ContentCommandExt {
                                ContentDeletionType::None,
                                tab->cursorCBIndex,
                                0,
                                vector<ContentBlock>(),
                                cbs,
                            });

                            uint32_t newCursorPos = tab->contentBlocks[tab->cursorCBIndex - 1].rope->length();

                            processCBCommand(ContentCommand {
                                CommandType::DeleteMergeCB,
                                true,
                                static_cast<uint16_t>(tab->extCommands.size() - 1),
                                tab->cursorCBIndex - 1,
                                0,
                                "",
                            });

                            --tab->cursorCBIndex;
                            tab->cursorPosition = newCursorPos;
                            UPDATE();
                        }
                    }
                }
                break;

            case GLFW_KEY_DELETE:
                if (
                    CUR_POS < cb->rope->length() && CUR_POS >= 0
                    || tab->textSelected
                ) {
                    if (tab->textSelected) {
                        tab->extCommands.push_back(ContentCommandExt {
                            ContentDeletionType::None,
                            tab->selEndCBIndex,
                            tab->selEndCursorPosition,
                            vector<ContentBlock>(),
                            vector<ContentBlock>(),
                        });

                        processCBCommand(ContentCommand {
                            CommandType::DeleteSelected,
                            true,
                            static_cast<uint16_t>(tab->extCommands.size() - 1),
                            tab->selStartCBIndex,
                            tab->selStartCursorPosition,
                            ""
                        });

                        tab->cursorPosition = tab->selStartCursorPosition;
                        tab->cursorCBIndex = tab->selStartCBIndex;
                        UNSELECT();
                    } else {
                        processCBCommand(ContentCommand {
                            CommandType::Delete,
                            false,
                            0,
                            tab->cursorCBIndex, CUR_POS,
                            string(1, (*cb->rope)[CUR_POS])
                        });
                    }

                    UPDATE();
                } else if (CUR_POS == cb->rope->length()) {
                    if (tab->textSelected) {
                        tab->extCommands.push_back(ContentCommandExt {
                            ContentDeletionType::None,
                            tab->selEndCBIndex,
                            tab->selEndCursorPosition,
                            vector<ContentBlock>(),
                            vector<ContentBlock>(),
                        });

                        processCBCommand(ContentCommand {
                            CommandType::DeleteSelected,
                            true,
                            static_cast<uint16_t>(tab->extCommands.size() - 1),
                            tab->selStartCBIndex,
                            tab->selStartCursorPosition,
                            ""
                        });

                        tab->cursorPosition = tab->selStartCursorPosition;
                        tab->cursorCBIndex = tab->selStartCBIndex;
                        UNSELECT();
                    } else {
                        if (
                            tab->cursorCBIndex != 0
                            && tab->cursorCBIndex + 1 < tab->contentBlocks.size()
                        ) {
                            vector<ContentBlock> cbs;
                            ContentBlock cb = tab->contentBlocks[tab->cursorCBIndex + 1];
                            cbs.push_back(cb);

                            tab->extCommands.push_back(ContentCommandExt {
                                ContentDeletionType::None,
                                tab->cursorCBIndex + 1,
                                0,
                                cbs,
                                vector<ContentBlock>(),
                            });

                            processCBCommand(ContentCommand {
                                CommandType::DeleteMergeCB,
                                true,
                                static_cast<uint16_t>(tab->extCommands.size() - 1),
                                tab->cursorCBIndex,
                                0,
                                "",
                            });
                        }
                    }
                }
                break;

            case GLFW_KEY_A:
                if (ctrl) {
                    const uint32_t len = cb->rope->length();
                    tab->textSelected = true;
                    tab->initialCursorPosition = tab->cursorPosition;
                    // tab->selStartCBIndex = 0;
                    // tab->selEndCol = len;
                    tab->redrawSel = true;
                    tab->cursorPosition = len;
                } else if (capsLck || shift) { TYPE("A"); } else { TYPE("a"); }

                break;

            case GLFW_KEY_B: if (capsLck || shift) { TYPE("B"); } else { TYPE("b"); } break;
            case GLFW_KEY_C:
                if (ctrl) {
                    if (tab->textSelected) {
                        string text = getSelectedTextFromCurrentTab();

                        if (!text.empty()) {
                            ClipboardResponse resp = copyToClipboard(text);

                            if (
                                resp.respType != ClipboardResponseType::Ok
                                && !resp.str.empty()
                            ) {
                                // TODO: log this in future
                                cout << "\n" << getClipboardError(resp);
                            }
                        }
                    }
                } else if (capsLck || shift) {
                    TYPE("C");
                } else {
                    TYPE("c");
                }
                break;
            case GLFW_KEY_D: if (capsLck || shift) { TYPE("D"); } else { TYPE("d"); } break;
            case GLFW_KEY_E: if (ctrl) { toggleExplorer(); } else if (capsLck || shift) { TYPE("E"); } else { TYPE("e"); } break;
            case GLFW_KEY_F: if (capsLck || shift) { TYPE("F"); } else { TYPE("f"); } break;
            case GLFW_KEY_G: if (capsLck || shift) { TYPE("G"); } else { TYPE("g"); } break;
            case GLFW_KEY_H: if (capsLck || shift) { TYPE("H"); } else { TYPE("h"); } break;
            case GLFW_KEY_I: if (capsLck || shift) { TYPE("I"); } else { TYPE("i"); } break;
            case GLFW_KEY_J: if (capsLck || shift) { TYPE("J"); } else { TYPE("j"); } break;
            case GLFW_KEY_K: if (capsLck || shift) { TYPE("K"); } else { TYPE("k"); } break;
            case GLFW_KEY_L: if (capsLck || shift) { TYPE("L"); } else { TYPE("l"); } break;
            case GLFW_KEY_M: if (capsLck || shift) { TYPE("M"); } else { TYPE("m"); } break;
            case GLFW_KEY_N: if (ctrl) { newNote(); } else if (capsLck || shift) { TYPE("N"); } else { TYPE("n"); } break;
            case GLFW_KEY_O: if (ctrl) { openFile(true); } else if (capsLck || shift) { TYPE("O"); } else { TYPE("o"); } break;
            case GLFW_KEY_P: if (capsLck || shift) { TYPE("P"); } else { TYPE("p"); } break;
            case GLFW_KEY_Q: if (capsLck || shift) { TYPE("Q"); } else { TYPE("q"); } break;
            case GLFW_KEY_R: if (capsLck || shift) { TYPE("R"); } else { TYPE("r"); } break;
            case GLFW_KEY_S: if (ctrl) { saveFile(); } else if (capsLck || shift) { TYPE("S"); } else { TYPE("s"); } break;
            case GLFW_KEY_T: if (capsLck || shift) { TYPE("T"); } else { TYPE("t"); } break;
            case GLFW_KEY_U: if (capsLck || shift) { TYPE("U"); } else { TYPE("u"); } break;
            case GLFW_KEY_V:
                if (ctrl) {
                    ClipboardResponse resp = pasteFromClipboard();

                    if (tab->textSelected) {
                        tab->extCommands.push_back(ContentCommandExt {
                            ContentDeletionType::None,
                            tab->selEndCBIndex,
                            tab->selEndCursorPosition,
                            vector<ContentBlock>(),
                            vector<ContentBlock>(),
                        });

                        processCBCommand(ContentCommand {
                            CommandType::Replace,
                            true,
                            static_cast<uint16_t>(tab->extCommands.size() - 1),
                            tab->selStartCBIndex,
                            tab->selStartCursorPosition,
                            resp.str,
                        });

                        UNSELECT();
                        UPDATE();
                    } else {
                        if (resp.respType == ClipboardResponseType::Ok) {
                            if (!resp.str.empty()) {
                                processCBCommand(ContentCommand {
                                    CommandType::Paste,
                                    false,
                                    0,
                                    tab->cursorCBIndex,
                                    tab->cursorPosition,
                                    resp.str
                                });

                                UPDATE();
                            }
                        } else {
                            cout << "\n" << getClipboardError(resp);
                        }
                    }
                } else if (capsLck || shift) {
                    TYPE("V");
                } else {
                    TYPE("v");
                }
                break;
            case GLFW_KEY_W: if (ctrl) { closeCurrentTab(); } else if (capsLck || shift) { TYPE("W"); } else { TYPE("w"); } break;
            case GLFW_KEY_X: if (capsLck || shift) { TYPE("X"); } else { TYPE("x"); } break;

            case GLFW_KEY_Y:
                if (ctrl) {
                    processCBCommand(BASIC_CMD(CommandType::Redo));
                } else {
                    if (capsLck || shift) { TYPE("Y"); } else { TYPE("y"); }
                }
                break;

            case GLFW_KEY_Z:
                if (ctrl) {
                    processCBCommand(BASIC_CMD(CommandType::Undo));
                } else {
                    if (capsLck || shift) { TYPE("Z"); } else { TYPE("z"); }
                }
                break;
            case GLFW_KEY_0: if (shift) { TYPE(")"); } else { TYPE("0"); } break;
            case GLFW_KEY_1: if (shift) { TYPE("!"); } else { TYPE("1"); } break;
            case GLFW_KEY_2: if (shift) { TYPE("@"); } else { TYPE("2"); } break;
            case GLFW_KEY_3: if (shift) { TYPE("#"); } else { TYPE("3"); } break;
            case GLFW_KEY_4: if (shift) { TYPE("$"); } else { TYPE("4"); } break;
            case GLFW_KEY_5: if (shift) { TYPE("%"); } else { TYPE("5"); } break;
            case GLFW_KEY_6: if (shift) { TYPE("^"); } else { TYPE("6"); } break;
            case GLFW_KEY_7: if (shift) { TYPE("&"); } else { TYPE("7"); } break;
            case GLFW_KEY_8: if (shift) { TYPE("*"); } else { TYPE("8"); } break;
            case GLFW_KEY_9: if (shift) { TYPE("("); } else { TYPE("9"); } break;
            case GLFW_KEY_MINUS: if (shift) { TYPE("_"); } else { TYPE("-"); } break;
            case GLFW_KEY_EQUAL: if (shift) { TYPE("+"); } else { TYPE("="); } break;
            case GLFW_KEY_PERIOD: if (shift) { TYPE(">"); } else { TYPE("."); } break;
            case GLFW_KEY_COMMA: if (shift) { TYPE("<"); } else { TYPE(","); } break;
            case GLFW_KEY_SLASH: if (shift) { TYPE("?"); } else { TYPE("/"); } break;
            case GLFW_KEY_BACKSLASH: if (shift) { TYPE("|"); } else { TYPE("\\"); } break;
            case GLFW_KEY_LEFT_BRACKET: if (shift) { TYPE("{"); } else { TYPE("["); } break;
            case GLFW_KEY_RIGHT_BRACKET: if (shift) { TYPE("}"); } else { TYPE("]"); } break;
            case GLFW_KEY_SEMICOLON: if (shift) { TYPE(":"); } else { TYPE(";"); } break;
            case GLFW_KEY_APOSTROPHE: if (shift) { TYPE("\""); } else { TYPE("'"); } break;
            case GLFW_KEY_GRAVE_ACCENT: if (shift) { TYPE("~"); } else { TYPE("`"); } break;
            case GLFW_KEY_KP_0: if (numLck) { TYPE("0"); } break;
            case GLFW_KEY_KP_1: if (numLck) { TYPE("1"); } break;
            case GLFW_KEY_KP_2: if (numLck) { TYPE("2"); } break;
            case GLFW_KEY_KP_3: if (numLck) { TYPE("3"); } break;
            case GLFW_KEY_KP_4: if (numLck) { TYPE("4"); } break;
            case GLFW_KEY_KP_5: if (numLck) { TYPE("5"); } break;
            case GLFW_KEY_KP_6: if (numLck) { TYPE("6"); } break;
            case GLFW_KEY_KP_7: if (numLck) { TYPE("7"); } break;
            case GLFW_KEY_KP_8: if (numLck) { TYPE("8"); } break;
            case GLFW_KEY_KP_9: if (numLck) { TYPE("9"); } break;
            case GLFW_KEY_KP_DECIMAL: if (numLck) { TYPE("."); } break;
            case GLFW_KEY_KP_DIVIDE: TYPE("/"); break;
            case GLFW_KEY_KP_MULTIPLY: TYPE("*"); break;
            case GLFW_KEY_KP_SUBTRACT: TYPE("-"); break;
            case GLFW_KEY_KP_ADD: TYPE("+"); break;
            case GLFW_KEY_KP_EQUAL: TYPE("="); break;
            case GLFW_KEY_SPACE: TYPE(" "); break;

            default: {
                // Nothing!
            }
        }
        #undef TYPE
        #undef BASIC_CMD
        #undef INCR_CURSOR
        #undef DECR_CURSOR
        #undef CUR_POS
        #undef UPDATE
        #undef UNSELECT
    }
}

void textInputKeyCallback(UIElement* elem, int key, int action, int mod) {
    bool alt = mod & GLFW_MOD_ALT;
    bool shift = mod & GLFW_MOD_SHIFT;
    bool ctrl = mod & GLFW_MOD_CONTROL;
    bool capsLck = mod & GLFW_MOD_CAPS_LOCK;
    bool numLck = mod & GLFW_MOD_NUM_LOCK;

    #ifdef DEBUG_TEXT_INPUT
        cout << "\n Key: ";

        if (alt) {
            cout << "Alt + ";
        }

        if (shift) {
            cout << "Shift + ";
        }

        if (ctrl) {
            cout << "Control + ";
        }

        if (capsLck) {
            cout << "Caps Lock + ";
        }

        if (numLck) {
            cout << "Num Lock + ";
        }

        cout << key;
    #endif

    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        #define TYPE(c) processCommand(elem, new Command(CommandType::Type, c, elem->textInputData->cursorPosition)); elem->textInputData->lastActiveTime = static_cast<float>(glfwGetTime())
        #define INCR_CURSOR() elem->textInputData->cursorPosition++
        #define DECR_CURSOR() elem->textInputData->cursorPosition--
        #define CUR_POS elem->textInputData->cursorPosition
        #define UPDATE_CURSOR_TIME() elem->textInputData->lastActiveTime = static_cast<float>(glfwGetTime())

        switch (key) {
            case GLFW_KEY_ESCAPE: {
                TextInputData* tid = elem->textInputData;

                if (tid->textSelected) {
                    textInputUnselect(tid);
                } else {
                    elem->active = false;
                }
                break;
            }

            case GLFW_KEY_LEFT:
                if (elem->textInputData != nullptr && CUR_POS != 0) {
                    TextInputData* tid = elem->textInputData;

                    if (shift) {
                        if (tid->textSelected) {
                            if (tid->cursorPosition - 1 == tid->initialCursorPosition) {
                                textInputUnselect(tid);
                            } else if (tid->cursorPosition > tid->initialCursorPosition) {
                                tid->textSelectEndCol -= 1;
                            } else {
                                tid->textSelectStartCol -= 1;
                            }
                        } else {
                            tid->textSelected = true;
                            tid->initialCursorPosition = tid->cursorPosition;
                            tid->textSelectStartCol = tid->cursorPosition - 1;
                            tid->textSelectEndCol = tid->cursorPosition;
                        }

                        tid->textSelectUpdated = true;
                    } else {
                        textInputUnselect(tid);
                    }

                    DECR_CURSOR();
                    UPDATE_CURSOR_TIME();
                }
                break;

            case GLFW_KEY_RIGHT:
                if (elem->textInputData != nullptr && CUR_POS != elem->text.length()) {
                    TextInputData* tid = elem->textInputData;

                    if (shift) {
                        if (tid->textSelected) {
                            if (tid->cursorPosition + 1 == tid->initialCursorPosition) {
                                textInputUnselect(tid);
                            } else if (tid->cursorPosition > tid->initialCursorPosition) {
                                tid->textSelectEndCol += 1;
                            } else {
                                tid->textSelectStartCol += 1;
                            }
                        } else {
                            tid->textSelected = true;
                            tid->initialCursorPosition = tid->cursorPosition;
                            tid->textSelectEndCol = tid->cursorPosition + 1;
                            tid->textSelectStartCol = tid->cursorPosition;
                        }

                        tid->textSelectUpdated = true;
                    } else {
                        textInputUnselect(tid);
                    }

                    INCR_CURSOR();
                    UPDATE_CURSOR_TIME();
                }
                break;

            case GLFW_KEY_UP:
                if (elem->type == UIElementType::TextArea) {
                }
                break;

            case GLFW_KEY_DOWN:
                break;

            case GLFW_KEY_ENTER:
                #ifdef DEBUG_TEXT_INPUT
                    cout << "\nEnter pressed";
                #endif

                if (shift && elem->type == UIElementType::TextArea) {
                    TYPE("\n");
                    INCR_CURSOR();
                }
                break;

            case GLFW_KEY_END:
                CUR_POS = static_cast<uint32_t>(elem->text.length());
                UPDATE_CURSOR_TIME();
                break;

            case GLFW_KEY_HOME:
                CUR_POS = 0;
                UPDATE_CURSOR_TIME();
                break;

            case GLFW_KEY_BACKSPACE: {
                TextInputData * tid = elem->textInputData;

                if (
                    CUR_POS <= elem->text.length() && CUR_POS > 0
                    || tid->textSelected
                ) {
                    string d;

                    if (tid->textSelected){
                        d = "";
                    } else {
                        d = elem->text[CUR_POS-1];
                    }

                    bool shouldDecrementCursor = !tid->textSelected;

                    processCommand(elem, new Command(CommandType::Backspace, d, CUR_POS));

                    if (shouldDecrementCursor && CUR_POS != 0) {
                        DECR_CURSOR();
                    }

                    UPDATE_CURSOR_TIME();
                }

                break;
            }

            case GLFW_KEY_DELETE: {
                TextInputData * tid = elem->textInputData;

                if (
                    CUR_POS < elem->text.length() && CUR_POS >= 0
                    || tid->textSelected
                ) {
                    string d;

                    if (tid->textSelected) {
                        d = "";
                    } else {
                        d = elem->text[CUR_POS];
                    }

                    processCommand(elem, new Command(CommandType::Delete, d, CUR_POS));
                    UPDATE_CURSOR_TIME();
                }

                break;
            }

            case GLFW_KEY_A:
                if (ctrl) {
                    TextInputData* tid = elem->textInputData;
                    const uint32_t len = static_cast<uint32_t>(elem->text.length());
                    tid->textSelected = true;
                    tid->initialCursorPosition = tid->cursorPosition;
                    tid->textSelectStartCol = 0;
                    tid->textSelectEndCol = len;
                    tid->textSelectUpdated = true;
                    tid->cursorPosition = len;
                } else if (capsLck || shift) { TYPE("A"); } else { TYPE("a"); }
                INCR_CURSOR();
                break;

            case GLFW_KEY_B:
                if (capsLck || shift) { TYPE("B"); } else { TYPE("b"); }
                INCR_CURSOR();
                break;

            case GLFW_KEY_C:
                if (capsLck || shift) { TYPE("C"); } else { TYPE("c"); }
                INCR_CURSOR();
                break;

            case GLFW_KEY_D:
                if (capsLck || shift) { TYPE("D"); } else { TYPE("d"); }
                INCR_CURSOR();
                break;

            case GLFW_KEY_E:
                if (capsLck || shift) { TYPE("E"); } else { TYPE("e"); }
                INCR_CURSOR();
                break;

            case GLFW_KEY_F:
                if (capsLck || shift) { TYPE("F"); } else { TYPE("f"); }
                INCR_CURSOR();
                break;

            case GLFW_KEY_G:
                if (capsLck || shift) { TYPE("G"); } else { TYPE("g"); }
                INCR_CURSOR();
                break;

            case GLFW_KEY_H:
                if (capsLck || shift) { TYPE("H"); } else { TYPE("h"); }
                INCR_CURSOR();
                break;

            case GLFW_KEY_I:
                if (capsLck || shift) { TYPE("I"); } else { TYPE("i"); }
                INCR_CURSOR();
                break;

            case GLFW_KEY_J:
                if (capsLck || shift) { TYPE("J"); } else { TYPE("j"); }
                INCR_CURSOR();
                break;

            case GLFW_KEY_K:
                if (capsLck || shift) { TYPE("K"); } else { TYPE("k"); }
                INCR_CURSOR();
                break;

            case GLFW_KEY_L:
                if (capsLck || shift) { TYPE("L"); } else { TYPE("l"); }
                INCR_CURSOR();
                break;

            case GLFW_KEY_M:
                if (capsLck || shift) { TYPE("M"); } else { TYPE("m"); }
                INCR_CURSOR();
                break;

            case GLFW_KEY_N:
                if (capsLck || shift) { TYPE("N"); } else { TYPE("n"); }
                INCR_CURSOR();
                break;

            case GLFW_KEY_O:
                if (capsLck || shift) { TYPE("O"); } else { TYPE("o"); }
                INCR_CURSOR();
                break;

            case GLFW_KEY_P:
                if (capsLck || shift) { TYPE("P"); } else { TYPE("p"); }
                INCR_CURSOR();
                break;

            case GLFW_KEY_Q:
                if (capsLck || shift) { TYPE("Q"); } else { TYPE("q"); }
                INCR_CURSOR();
                break;

            case GLFW_KEY_R:
                if (capsLck || shift) { TYPE("R"); } else { TYPE("r"); }
                INCR_CURSOR();
                break;

            case GLFW_KEY_S:
                if (capsLck || shift) { TYPE("S"); } else { TYPE("s"); }
                INCR_CURSOR();
                break;

            case GLFW_KEY_T:
                if (capsLck || shift) { TYPE("T"); } else { TYPE("t"); }
                INCR_CURSOR();
                break;

            case GLFW_KEY_U:
                if (capsLck || shift) { TYPE("U"); } else { TYPE("u"); }
                INCR_CURSOR();
                break;

            case GLFW_KEY_V:
                if (capsLck || shift) { TYPE("V"); } else { TYPE("v"); }
                INCR_CURSOR();
                break;

            case GLFW_KEY_W:
                if (capsLck || shift) { TYPE("W"); } else { TYPE("w"); }
                INCR_CURSOR();
                break;

            case GLFW_KEY_X:
                if (capsLck || shift) { TYPE("X"); } else { TYPE("x"); }
                INCR_CURSOR();
                break;

            case GLFW_KEY_Y:
                if (ctrl) {
                    processCommand(elem, new Command(CommandType::Redo, "", CUR_POS));
                } else if (capsLck || shift) {
                    TYPE("Y");
                    INCR_CURSOR();
                } else {
                    TYPE("y");
                    INCR_CURSOR();
                }
                break;

            case GLFW_KEY_Z:
                if (ctrl) {
                    processCommand(elem, new Command(CommandType::Undo, "", CUR_POS));
                } else if (capsLck || shift) {
                    TYPE("Z");
                    INCR_CURSOR();
                } else {
                    TYPE("z");
                    INCR_CURSOR();
                }
                break;

            case GLFW_KEY_0:
                if (shift) { TYPE(")"); } else { TYPE("0"); }
                INCR_CURSOR();
                break;

            case GLFW_KEY_1:
                if (shift) { TYPE("!"); } else { TYPE("1"); }
                INCR_CURSOR();
                break;

            case GLFW_KEY_2:
                if (shift) { TYPE("@"); } else { TYPE("2"); }
                INCR_CURSOR();
                break;

            case GLFW_KEY_3:
                if (shift) { TYPE("#"); } else { TYPE("3"); }
                INCR_CURSOR();
                break;

            case GLFW_KEY_4:
                if (shift) { TYPE("$"); } else { TYPE("4"); }
                INCR_CURSOR();
                break;

            case GLFW_KEY_5:
                if (shift) { TYPE("%"); } else { TYPE("5"); }
                INCR_CURSOR();
                break;

            case GLFW_KEY_6:
                if (shift) { TYPE("^"); } else { TYPE("6"); }
                INCR_CURSOR();
                break;

            case GLFW_KEY_7:
                if (shift) { TYPE("&"); } else { TYPE("7"); }
                INCR_CURSOR();
                break;

            case GLFW_KEY_8:
                if (shift) { TYPE("*"); } else { TYPE("8"); }
                INCR_CURSOR();
                break;

            case GLFW_KEY_9:
                if (shift) { TYPE("("); } else { TYPE("9"); }
                INCR_CURSOR();
                break;

            case GLFW_KEY_MINUS:
                if (shift) { TYPE("_"); } else { TYPE("-"); }
                INCR_CURSOR();
                break;

            case GLFW_KEY_EQUAL:
                if (shift) { TYPE("+"); } else { TYPE("="); }
                INCR_CURSOR();
                break;

            case GLFW_KEY_PERIOD:
                if (shift) { TYPE(">"); } else { TYPE("."); }
                INCR_CURSOR();
                break;

            case GLFW_KEY_COMMA:
                if (shift) { TYPE("<"); } else { TYPE(","); }
                INCR_CURSOR();
                break;

            case GLFW_KEY_SLASH:
                if (shift) { TYPE("?"); } else { TYPE("/"); }
                INCR_CURSOR();
                break;

            case GLFW_KEY_BACKSLASH:
                if (shift) { TYPE("|"); } else { TYPE("\\"); }
                INCR_CURSOR();
                break;

            case GLFW_KEY_LEFT_BRACKET:
                if (shift) { TYPE("{"); } else { TYPE("["); }
                INCR_CURSOR();
                break;

            case GLFW_KEY_RIGHT_BRACKET:
                if (shift) { TYPE("}"); } else { TYPE("]"); }
                INCR_CURSOR();
                break;

            case GLFW_KEY_SEMICOLON:
                if (shift) { TYPE(":"); } else { TYPE(";"); }
                INCR_CURSOR();
                break;

            case GLFW_KEY_APOSTROPHE:
                if (shift) { TYPE("\""); } else { TYPE("'"); }
                INCR_CURSOR();
                break;

            case GLFW_KEY_GRAVE_ACCENT:
                if (shift) { TYPE("~"); } else { TYPE("`"); }
                INCR_CURSOR();
                break;

            case GLFW_KEY_KP_0:
                if (numLck) { TYPE("0"); INCR_CURSOR(); }
                break;

            case GLFW_KEY_KP_1:
                if (numLck) { TYPE("1"); INCR_CURSOR(); }
                break;

            case GLFW_KEY_KP_2:
                if (numLck) { TYPE("2"); INCR_CURSOR(); }
                break;

            case GLFW_KEY_KP_3:
                if (numLck) { TYPE("3"); INCR_CURSOR(); }
                break;

            case GLFW_KEY_KP_4:
                if (numLck) { TYPE("4"); INCR_CURSOR(); }
                break;

            case GLFW_KEY_KP_5:
                if (numLck) { TYPE("5"); INCR_CURSOR(); }
                break;

            case GLFW_KEY_KP_6:
                if (numLck) { TYPE("6"); INCR_CURSOR(); }
                break;

            case GLFW_KEY_KP_7:
                if (numLck) { TYPE("7"); INCR_CURSOR(); }
                break;

            case GLFW_KEY_KP_8:
                if (numLck) { TYPE("8"); INCR_CURSOR(); }
                break;

            case GLFW_KEY_KP_9:
                if (numLck) { TYPE("9"); INCR_CURSOR(); }
                break;

            case GLFW_KEY_KP_DECIMAL:
                if (numLck) { TYPE("."); INCR_CURSOR(); }
                break;

            case GLFW_KEY_KP_DIVIDE:
                TYPE("/"); INCR_CURSOR();
                break;

            case GLFW_KEY_KP_MULTIPLY:
                TYPE("*"); INCR_CURSOR();
                break;

            case GLFW_KEY_KP_SUBTRACT:
                TYPE("-"); INCR_CURSOR();
                break;

            case GLFW_KEY_KP_ADD:
                TYPE("+"); INCR_CURSOR();
                break;

            case GLFW_KEY_KP_EQUAL:
                TYPE("="); INCR_CURSOR();
                break;

            case GLFW_KEY_SPACE:
                TYPE(" "); INCR_CURSOR();
                break;
                

            default: {
                // Nothing!
            }
        }
        #undef TYPE
        #undef INCR_CURSOR
        #undef DECR_CURSOR
        #undef CUR_POS
        #undef UPDATE_CURSOR_TIME
    }
}

Command* replaceSelectedText(UIElement* elem, Command* oldCommand) {
    TextInputData* tid = elem->textInputData;

    uint32_t len = tid->textSelectEndCol - tid->textSelectStartCol;

    Command * newCommand = new Command(
        0, 0, tid->textSelectStartCol, tid->textSelectEndCol,
        oldCommand->str,
        elem->text.substr(tid->textSelectStartCol, len),
        oldCommand->cursorPos
    );

    elem->text.erase(tid->textSelectStartCol, len);
    elem->text.insert(tid->textSelectStartCol, oldCommand->str);

    deleteCommand(oldCommand);

    textInputUnselect(tid);

    return newCommand;
}

// NOTE: might not need this since the active content block calculation is done
// with the highlighted content box.
void activateCBFromScreenCoords(float mouseX, float mouseY) {
    if (tabs.empty()) { return; }

    ContentTab* tab = &tabs[activeTabIndex];

    for (auto itr = tab->contentBlocks.begin(); itr != tab->contentBlocks.end(); ++itr) {
        float top = (*itr).top - tab->scrollOffsetY;

        if (top <= mouseY && top + (*itr).height >= mouseY) {
            uint32_t index = distance(tab->contentBlocks.begin(), itr);

            tab->activeCBIndex = index;
            tab->cursorCBIndex = index;
            return;
        }

        if (top > mouseY) {
            return;
        }
    }
}

inline uint8_t getCBFontSize(ContentBlockType cbType) {
    switch (cbType) {
        case ContentBlockType::Heading1: return 48;
        case ContentBlockType::Heading2: return 40;
        case ContentBlockType::Heading3: return 32;
        case ContentBlockType::Heading4: return 24;
        case ContentBlockType::Heading5: return 16;
        case ContentBlockType::Heading6: return 12;
        case ContentBlockType::Paragraph: return 16;
        case ContentBlockType::Title:  return 48;
    }

    return 16;
}

inline glm::vec2 getCBVerticalMargins(ContentBlockType cbType) {
    switch (cbType) {
        case ContentBlockType::Paragraph: return { 4.0f, 4.0f };
        case ContentBlockType::Title: return { 24.0f, 16.0f };

        default: return { 16.0f, 8.0f };
    }
}

SelectionData getCBTextCursor(glm::vec2 &resolution) {
    ContentTab* tab = &tabs[activeTabIndex];
    ContentBlock* cb = &tab->contentBlocks[tab->cursorCBIndex];

    RopeTraverser rt(cb->rope);

    uint8_t fontSize = getCBFontSize(cb->type);

    float x = contentLeft;
    float y = (*cb).top - tab->scrollOffsetY;
    float xpos = x;
    float ypos;
    float yNet = y;

    uint32_t index = 0;
    uint32_t row = 0;
    uint32_t col = 0;
    float h = fontSize * glyphDescender;
    uint32_t i = 0;

    uint16_t height = static_cast<uint16_t>(fontSize) - static_cast<uint16_t>(h);

    if (tab->cursorPosition == 0) {
        return SelectionData {
            .cursorPosition = i,
            .row = 0,
            .col = 0,
            .x = xpos,
            .y = yNet + static_cast<float>(fontSize) - h,
            .height = height,
        };
    }

    while (rt.hasNext()) {
        Rope* r = rt.next();

        for (const char c : r->text) {
            Character* ch = &characters[(int)c];

            float _tx1, _tx2, _ty1, _ty2, _h, _xw, _yh, advance;
            float glyphHeight, glyphWidth, bearingTop, bearingLeft;
            bool _;
            glm::vec2 pos = glm::vec2(x, y);

            getCharParams(
                characters[(int)c],
                pos,
                xpos,
                ypos,
                yNet,
                _xw,
                _yh,
                _h,
                _tx1,
                _tx2,
                _ty1,
                _ty2,
                glyphHeight,
                glyphWidth,
                bearingTop,
                bearingLeft,
                advance,
                contentRight,
                row,
                fontSize,
                _
            );

            if (i == tab->cursorPosition) {
                cout << "\nDEBUG: Cursor position: " << tab->cursorPosition << " found";

                return SelectionData {
                    .cursorPosition = i,
                    .row = row,
                    .col = col,
                    .x = xpos,
                    .y = ypos + bearingTop - glyphHeight - h,
                    .height = height,
                };
            }

            ++i;
            xpos += advance;
        }
    }

    if (tab->cursorPosition == cb->rope->length()) {
        return SelectionData {
            .cursorPosition = i,
            .row = row,
            .col = col,
            .x = xpos,
            .y = yNet + static_cast<float>(fontSize) - h,
            .height = height,
        };
    }

    return SelectionData { 0, 0, 0 };
}

SelectionData getCBTextCursorFromScreenCoords(float mouseX, float mouseY, glm::vec2 resolution) {
    ContentTab* tab = &tabs[activeTabIndex];
    ContentBlock* cb = &tab->contentBlocks[tab->activeCBIndex];

    RopeTraverser rt(cb->rope);

    float fontSize = static_cast<float>(getCBFontSize(cb->type));

    float x = contentLeft;
    float y = (*cb).top - tab->scrollOffsetY;

    float xpos = x;
    float ypos;
    float yNet = y;
    float xprev = x;

    uint32_t index = 0;
    uint32_t row = 0;
    uint32_t col = 0;
    float h = fontSize * glyphDescender;
    float height = fontSize - fontSize * glyphDescender;

    while (rt.hasNext()) {
        Rope* r = rt.next();

        for (const char c : r->text) {
            float _x1, _x2, _y1, _y2, xpos_plus_w, ypos_plus_h, advance;
            float glyphHeight, glyphWidth, bearingTop, bearingLeft;
            bool _;

            glm::vec2 pos = glm::vec2(x, y);

            getCharParams(
                characters[(int)c],
                pos,
                xpos,
                ypos,
                yNet,
                xpos_plus_w,
                ypos_plus_h,
                height,
                _x1,
                _x2,
                _y1,
                _y2,
                glyphHeight,
                glyphWidth,
                bearingTop,
                bearingLeft,
                advance,
                contentRight,
                row,
                fontSize,
                _
            );

            if (
                xpos <= mouseX && xpos + (glyphWidth/2) >= mouseX
                && ypos >= mouseY && ypos_plus_h <= mouseY
            ) {
                #ifdef DEBUG_TEXT
                cout << "\n2: xpos: " << xpos << ", ypos: " << ypos;

                if (tab->activeCBIndex == 2) {
                    cout << "\nArey bhai!!!! h: " << height;
                }
                #endif

                return SelectionData {
                    .cursorPosition = index,
                    .row = row,
                    .col = col,
                    .cursorCBIndex = tab->activeCBIndex,
                    .x = xprev,
                    .y = ypos + bearingTop - glyphHeight - h,
                    .height = static_cast<uint16_t>(fontSize - h),
                };
            } else if (
                xpos + (glyphWidth/2) <= mouseX && xpos_plus_w >= mouseX
                && ypos >= mouseY && ypos_plus_h <= mouseY
            ) {
                #ifdef DEBUG_TEXT
                cout << "\n3: xpos: " << xpos << ", ypos: " << ypos;

                if (tab->activeCBIndex == 2) {
                    cout << "\nArey bhai!!!! h: " << height;
                }
                #endif

                return SelectionData {
                    .cursorPosition = index + 1,
                    .row = row,
                    .col = col + 1,
                    .cursorCBIndex = tab->activeCBIndex,
                    .x = xpos_plus_w,
                    .y = ypos + bearingTop - glyphHeight - h,
                    .height = static_cast<uint16_t>(fontSize - h),
                };
            } else if (xpos_plus_w >= mouseX && ypos >= mouseY && ypos_plus_h <= mouseY) {
                return SelectionData {
                    .cursorPosition = index,
                    .row = row,
                    .col = col,
                    .cursorCBIndex = tab->activeCBIndex,
                    .x = xpos,
                    .y = ypos + bearingTop - glyphHeight - h,
                    .height = static_cast<uint16_t>(fontSize - h),
                };
            }

            xprev = xpos_plus_w;
            xpos += advance;

            ++index;
            ++col;
        }
    }

    return SelectionData {
        tab->cursorPosition, 0, 0, tab->cursorCBIndex, 0.0f, 0.0f, 0, SelectionCBPosition::None
    };
}

SelectionData getCBTextCursorVertical(ContentTab* tab, bool page, bool up) {
    #define UP_AND_TOP_ROW          up && tab->cursorRow == 0
    #define DOWN_AND_BOTTOM_ROW     !up && tab->cursorRow == cb->rows - 1

    SelectionData sd = {
        tab->cursorPosition,
        tab->cursorRow,
        0,
        tab->cursorCBIndex,
        tab->cursorX,
        tab->cursorY,
        0,
        SelectionCBPosition::Current,
    };

    ContentBlock* cb = &tab->contentBlocks[tab->cursorCBIndex];

    if (page && !up) {
        sd.row = cb->rows - 1;
    }

    if (
        UP_AND_TOP_ROW && tab->activeCBIndex < 2
        || DOWN_AND_BOTTOM_ROW && tab->cursorCBIndex == tab->contentBlocks.size() - 1
    ) {
        return sd;
    }

    if (!page) {
        if (UP_AND_TOP_ROW && tab->cursorCBIndex > 0) {
            sd.cbPos = SelectionCBPosition::Previous;
            cb = &tab->contentBlocks[tab->cursorCBIndex - 1];

            if (cb->rows > 0) {
                sd.row = cb->rows - 1;
            }
        } else if (DOWN_AND_BOTTOM_ROW) {
            sd.cbPos = SelectionCBPosition::Next;
            sd.row = 0;
            cb = &tab->contentBlocks[tab->cursorCBIndex + 1];
        } else if (up) {
            if (tab->cursorRow > 0) {
                sd.row = tab->cursorRow - 1;
            }
        } else {
            sd.row = tab->cursorRow + 1;
        }
    }

    float x = contentLeft;
    float y = (*cb).top - tab->scrollOffsetY;

    float xpos = x;
    float ypos;
    float yNet = y;
    float xprev = x;

    uint8_t fontSize = getCBFontSize(cb->type);

    uint32_t index = 0;
    uint32_t row = 0;
    uint32_t col = 0;
    float h = fontSize * glyphDescender;

    RopeTraverser rt(cb->rope);

    bool foundRow = false;
    bool visibleRow = y > TAB_BAR_HEIGHT;

    while (rt.hasNext()) {
        Rope* r = rt.next();

        for (const char c : r->text) {
            float xpos_plus_w, ypos_plus_h, advance, _x1, _x2, _y1, _y2, height;
            float glyphHeight, glyphWidth, bearingTop, bearingLeft;
            bool _;

            glm::vec2 pos = glm::vec2(x, y);

            getCharParams(
                characters[(int)c],
                pos,
                xpos,
                ypos,
                yNet,
                xpos_plus_w,
                ypos_plus_h,
                height,
                _x1,
                _x2,
                _y1,
                _y2,
                glyphHeight,
                glyphWidth,
                bearingTop,
                bearingLeft,
                advance,
                contentRight,
                row,
                fontSize,
                _
            );

            if (page) {
                if (ypos > windowHeight) {
                    break;
                }

                visibleRow = ypos > TAB_BAR_HEIGHT && ypos < windowHeight;

                if (!visibleRow) {
                    continue;
                }
            }

            #ifdef DEBUG_TEXT
            if (debugTextFirstRenderPrint) {
                cout << "; x: " << xpos << " y: " << ypos << " x + w: "
                    << xpos_plus_w << " y + h: " << ypos_plus_h
                    << " bearingLeft: " << bearingLeft
                    << " bearingTop: " << bearingTop
                    << " widthRatio: " << widthRatio
                    << " heightRatio: " << heightRatio;
            }
            #endif

            if (row == sd.row || page && up) {
                foundRow = true;

                if (xpos <= tab->cursorX && xpos + (glyphWidth/2) >= tab->cursorX) {
                    cout << "\n2: xpos: " << xpos << ", ypos: " << ypos;

                    uint32_t cursorCBIndex;

                    switch (sd.cbPos) {
                        case SelectionCBPosition::Next:
                            cursorCBIndex = tab->cursorCBIndex + 1;
                            break;
                        case SelectionCBPosition::Previous:
                            cursorCBIndex = tab->cursorCBIndex - 1;
                            break;
                        default:
                            cursorCBIndex = tab->cursorCBIndex;
                    }

                    return SelectionData {
                        .cursorPosition = index,
                        .row = row,
                        .col = col,
                        .cursorCBIndex = cursorCBIndex,
                        .x = xprev,
                        .y = ypos - h,
                        .height = static_cast<uint16_t>(fontSize - h),
                        .cbPos = sd.cbPos,
                    };
                } else if (xpos + (glyphWidth/2) <= tab->cursorX && xpos_plus_w >= tab->cursorX) {
                    cout << "\n3: xpos: " << xpos << ", ypos: " << ypos;

                    uint32_t cursorCBIndex;

                    switch (sd.cbPos) {
                        case SelectionCBPosition::Next:
                            cursorCBIndex = tab->cursorCBIndex + 1;
                            break;
                        case SelectionCBPosition::Previous:
                            cursorCBIndex = tab->cursorCBIndex - 1;
                            break;
                        default:
                            cursorCBIndex = tab->cursorCBIndex;
                    }

                    return SelectionData {
                        .cursorPosition = index + 1,
                        .row = row,
                        .col = col + 1,
                        .cursorCBIndex = cursorCBIndex,
                        .x = xpos_plus_w,
                        .y = ypos - h,
                        .height = static_cast<uint16_t>(fontSize - h),
                        .cbPos = sd.cbPos,
                    };
                } else if (xpos_plus_w >= tab->cursorX) {
                    cout << "\n4: xpos: " << xpos << ", ypos: " << ypos;

                    uint32_t cursorCBIndex;

                    switch (sd.cbPos) {
                        case SelectionCBPosition::Next:
                            cursorCBIndex = tab->cursorCBIndex + 1;
                            break;
                        case SelectionCBPosition::Previous:
                            cursorCBIndex = tab->cursorCBIndex - 1;
                            break;
                        default:
                            cursorCBIndex = tab->cursorCBIndex;
                    }

                    return SelectionData {
                        .cursorPosition = index,
                        .row = row,
                        .col = col,
                        .cursorCBIndex = cursorCBIndex,
                        .x = xpos,
                        .y = ypos - h,
                        .height = static_cast<uint16_t>(fontSize - h),
                        .cbPos = sd.cbPos,
                    };
                }
            }

            xprev = xpos_plus_w;
            xpos += advance;

            ++index;
            ++col;
        }
    }

    if (foundRow) {
        uint32_t cursorCBIndex;

        switch (sd.cbPos) {
            case SelectionCBPosition::Next:
                cursorCBIndex = tab->cursorCBIndex + 1;
                break;
            case SelectionCBPosition::Previous:
                cursorCBIndex = tab->cursorCBIndex - 1;
                break;
            default:
                cursorCBIndex = tab->cursorCBIndex;
        }

        return SelectionData {
            .cursorPosition = index,
            .row = sd.row,
            .col = col,
            .cursorCBIndex = cursorCBIndex,
            .x = xpos,
            .y = yNet + static_cast<float>(fontSize) - h,
            .height = static_cast<uint16_t>(fontSize - h),
            .cbPos = sd.cbPos,
        };
    }

    sd.cbPos = SelectionCBPosition::None;

    return sd;

    #undef UP_AND_TOP_ROW
    #undef DOWN_AND_BOTTOM_ROW
}

vector<ContentBlock>::iterator getFirstVisibleCB(ContentTab* tab) {
    vector<ContentBlock>::reverse_iterator cbIter = tab->contentBlocks.rbegin()
        + (tab->contentBlocks.size() - tab->cursorCBIndex);

    vector<ContentBlock>::reverse_iterator lastVisibleIter = tab->contentBlocks.rend();

    for (; cbIter != tab->contentBlocks.rend(); ++cbIter) {
        float top = (*cbIter).top - tab->scrollOffsetY;

        if (
            top > TAB_BAR_HEIGHT && top < windowHeight
            || top < TAB_BAR_HEIGHT && top + (*cbIter).height + TAB_BAR_HEIGHT > windowHeight
        ) {
            cout << "\nContent block \"" << (*cbIter).rope->getString().substr(0, 16) << " is visible";
            lastVisibleIter = cbIter;
        }
    }

    return lastVisibleIter.base();
}

vector<ContentBlock>::iterator getLastVisibleCB(ContentTab* tab) {
    vector<ContentBlock>::iterator cbIter = tab->contentBlocks.begin() + tab->cursorCBIndex;

    vector<ContentBlock>::iterator lastVisibleIter = tab->contentBlocks.end();

    for (; cbIter != tab->contentBlocks.end(); ++cbIter) {
        float top = (*cbIter).top - tab->scrollOffsetY;

        if (
            top > TAB_BAR_HEIGHT && top < windowHeight
            || top < TAB_BAR_HEIGHT && top + (*cbIter).height + TAB_BAR_HEIGHT > windowHeight
        ) {
            cout << "\nContent block \"" << (*cbIter).rope->getString().substr(0, 16) << " is visible";
            lastVisibleIter = cbIter;
        }
    }

    return lastVisibleIter;
}

void openFile(bool openDialog, filesystem::path p) {
    filesystem::path path;
    string fileName;

    if (openDialog) {
        FileDialogResult result = openFileDialog(window, "Open file", FileDialogMode::Open);

        if (result.success) {
            path = result.filepath;
            fileName = path.filename().string();

            cout << "\nDEBUG: Choosing file was successful! you chose: " << result.filepath
                << "\nDEBUG: File Name: " << fileName;
        } else {
            cerr << "\nSome error opening file dialog.";
            return;
        }
    } else {
        path = p;
        fileName = path.filename().string();
    }

    if (checkFile(path) == OpenedFileType::TextFile) {
        newTab(path);

        ContentTab* tab = &tabs[activeTabIndex];

        tab->contentBlocks.erase(tab->contentBlocks.begin());
        tab->filePath = path;

        vector<string> lines = readFileText(path);

        for (string line: lines) {
            tab->contentBlocks.emplace_back(ContentBlock {
                ContentBlockType::Paragraph,
                CONTENT_BLOCK_REDRAW | CONTENT_BLOCK_VISIBLE,
                0.0f, 0.0f, 0.0f, 0, 0, 0, 0, new Rope(line),
            });
        }

        #ifdef DEBUG_CONTENT_BLOCKS
            cout << "\nRead following content from file ("
                << tab->contentBlocks.size() << "): ";

            printContentBlocksFromVector(tab->contentBlocks);
        #endif

        updateWindowTitle();

        TAB_BAR.show = true;
        CONTENT_CONTAINER.show = true;
    }
}

void saveFile() {
    ContentTab* tab = CURRENT_TAB;

    if (tab->filePath.empty()) {
        FileDialogResult fdr = openFileDialog(window, "Save", FileDialogMode::Save);
        if (fdr.success) {
            tab->filePath = fdr.filepath;
        } else {
            cout << "\nError: some error occured!";
            return;
        }
    }

    ofstream fout(tab->filePath);

    for (auto itr = tab->contentBlocks.begin(); itr != tab->contentBlocks.end(); ++itr) {
        if (itr != tab->contentBlocks.begin()) {
            fout << "\n";
        }

        fout << (*itr).rope->getString();
    }
}

void newNote() {
    string title = "Untitled Note";

    if (untitledNotes > 0) {
        title += " " + to_string(untitledNotes);
    }

    ++untitledNotes;

    string separator = string(1, filesystem::path::preferred_separator);

    newTab(
        getVaultsDirectory()
        + separator
        + selectedVault
        + separator
        + title
    );

    updateWindowTitle();

    ContentTab* tab = &tabs[activeTabIndex];

    tab->contentBlocks.emplace_back(ContentBlock {
        ContentBlockType::Paragraph,
        CONTENT_BLOCK_REDRAW | CONTENT_BLOCK_VISIBLE,
        0.0f, 0.0f, 0.0f, 0, 0, 0, 0, new Rope()
    });

    TAB_BAR.show = true;
    CONTENT_CONTAINER.show = true;
}

void updateWindowTitle() {
    if (!tabs.empty()) {
        ContentTab* tab = CURRENT_TAB;
        glfwSetWindowTitle(window, ("patram - " + tab->filePath.filename().string()).c_str());
        return;
    }

    glfwSetWindowTitle(window, "patram");
}

SelectionData getTextIndexForScreenCoords(UIElement* elem, float mouseX, float mouseY, glm::vec2 res) {
    uint32_t currentIndex = 0;
    uint32_t row = 0;
    uint32_t col = 0;

    const float xStart = elem->left + BUTTON_PADDING;

    float x = xStart;
    float y = elem->top + BUTTON_PADDING;

    // cout << "\nMouse x: " << mouseX << " y: " << mouseY;
    const float Y_ADVANCE = static_cast<float>(elem->fontSize + LINE_GAP);
    bool shouldYAdvance = false;

    for (const char c: elem->text) {
        shouldYAdvance = c == '\n';
        GlyphDimensions gd = getGlyphDimensions(c, x, y, res, elem->fontSize);

        // cout << "\ncharacter: " << c << ", leftX: " << gd.leftX
        //     << ", rightX: " << gd.rightX << ", bottomY: " << gd.bottomY
        //     << ", topY: " << gd.topY << "\nx: " << x;

        uint32_t net_advance = (static_cast<uint32_t>(gd.advance * gd.scale) >> 6);

        if (
            x + static_cast<float>(net_advance)/2 > mouseX
            && (
                // !shouldYAdvance
                // || shouldYAdvance &&
                y + Y_ADVANCE > mouseY
            )
        ) {
            #ifdef DEBUG_TEXT_SELECTION
                cout << "\nx: " << x << ", y: " << y << ", shouldYAdvance: " << shouldYAdvance << " ";
            #endif
            break;
        }

        if (shouldYAdvance) {
            x = xStart;
            y += Y_ADVANCE;
            col = 0;
            ++row;
        } else {
            ++col;
            x += net_advance;
            ++currentIndex;
        }
    }

    return {
        .cursorPosition = currentIndex,
        .row = row,
        .col = col,
    };
}

void textInputMouseDownCallback(
    UIElement* elem,
    float mouseX,
    float mouseY,
    glm::vec2 res
) {
    if (elem->textInputData == nullptr) { return; }

    TextInputData * tid = elem->textInputData;

    SelectionData sd = getTextIndexForScreenCoords(
        elem,
        mouseX,
        mouseY,
        res
    );

    tid->cursorPosition = sd.cursorPosition;

    if (tid->textSelected) {
        tid->textSelectStartCol = sd.col;
        tid->textSelectStartRow = sd.row;
        textInputUnselect(tid, "Text Input Mouse Down callback");
    }

    tid->mouseDown = true;
    tid->mouseDownX = mouseX;
    tid->mouseDownY = mouseY;
    tid->lastActiveTime = TIME();
}

void tabBarMouseDownCallback(UIElement* elem, float mouseX, float mouseY, glm::vec2 res) {
    float x = 0;

    for (int i=0; i<tabs.size(); ++i) {
        x += TAB_HORIZONTAL_PADDING + tabsDisplay[i].width + TAB_HORIZONTAL_PADDING;

        if (mouseX <= x) {
            activeTabIndex = i;
            TAB_BAR.reDraw = true;
            TAB_BAR.active = false;
            activeElement = &CONTENT_CONTAINER;

            ContentTab* tab = CURRENT_TAB;

            if (tab->scrollbarUpdated == false) {
                tab->redrawCursor = true;
                reDrawCBsFrom(tab->firstVisibleCBIndex, tab->visibleCBLength);
            }

            break;
        }
    }

    updateWindowTitle();
}

void textInputMouseDragCallback(UIElement* elem, float mouseX, float mouseY, glm::vec2 res) {
    if (elem->textInputData == nullptr) { return; }

    #ifdef DEBUG_TEXT_SELECTION
        cout << "\nDrag Callback: ";
    #endif

    TextInputData* tid = elem->textInputData;

    if (!tid->mouseDown) {
        #ifdef DEBUG_TEXT_SELECTION
            cout << "Nothing!";
        #endif
        return;
    }

    SelectionData sd = getTextIndexForScreenCoords(elem, mouseX, mouseY, res);

    // cout << "\nsd.cursorPosition: " << sd.cursorPosition << ", sd.col: " << sd.col << ", sd.row: " << sd.row;

    if (!tid->textSelected) {
        tid->textSelected = true;
        tid->initialCursorPosition = tid->cursorPosition;
        tid->textSelectStartCol = sd.col;
        tid->textSelectEndCol = sd.col;
        tid->textSelectStartRow = sd.row;
        tid->textSelectEndRow = sd.row;
        tid->initialRow = sd.row;
        tid->initialCol = sd.col;

        tid->cursorPosition = sd.cursorPosition;
        tid->textSelectUpdated = true;

        #ifdef DEBUG_TEXT_SELECTION
            printTextInputData(tid, "Text Selection 1");
        #endif
    } else if (tid->textSelected) {
        if (sd.row == tid->initialRow) {
            tid->textSelectStartRow = sd.row;
            tid->textSelectEndRow = sd.row;
        } else if (sd.row < tid->initialRow) {
            tid->textSelectStartRow = sd.row;
        } else {
            tid->textSelectEndRow = sd.row;
        }

        if (
            sd.row == tid->textSelectStartRow
            && sd.row == tid->textSelectEndRow
            && sd.row == tid->initialRow
        ) {
            if (sd.col == tid->initialCol) {
                tid->textSelectStartCol = sd.col;
                tid->textSelectEndCol = sd.col;
            } else if (sd.col < tid->initialCol) {
                if (tid->textSelectStartCol > tid->textSelectEndCol) {
                    if (tid->textSelectStartRow > tid->prevRow) {
                        tid->textSelectEndCol = tid->textSelectStartCol;
                        tid->textSelectStartCol = sd.col;
                    } else if (tid->textSelectStartRow < tid->prevRow) {
                        tid->textSelectEndCol = tid->textSelectStartCol;
                        tid->textSelectStartCol = sd.col;
                    }
                } else {
                    tid->textSelectStartCol = sd.col;
                }
            } else {
                if (tid->textSelectStartCol > tid->textSelectEndCol) {
                    if (tid->textSelectStartRow > tid->prevRow) {
                        tid->textSelectStartCol = tid->textSelectEndCol;
                        tid->textSelectEndCol = sd.col;
                    } else if (tid->textSelectStartRow < tid->prevRow) {
                        tid->textSelectEndCol = tid->textSelectStartCol;
                        tid->textSelectStartCol = sd.col;
                    }
                } else {
                    tid->textSelectEndCol = sd.col;
                }
            }
        } else if (sd.row == tid->textSelectStartRow) {
            if (tid->prevRow != tid->textSelectStartRow && tid->textSelectStartCol < sd.col) {
                tid->textSelectEndCol = tid->textSelectStartCol;
            }

            tid->textSelectStartCol = sd.col;
        } else {
            if (tid->prevRow == tid->textSelectStartRow && tid->textSelectEndCol > sd.col) {
                tid->textSelectStartCol = tid->textSelectEndCol;
            }

            tid->textSelectEndCol = sd.col;
        }

        tid->prevRow = sd.row;
        tid->cursorPosition = sd.cursorPosition;
        tid->textSelectUpdated = true;

        #ifdef DEBUG_TEXT_SELECTION
            printTextInputData(tid, "Text Selection 2");
        #endif
    } else {
        #ifdef DEBUG_TEXT_SELECTION
            cout << "\nMarked selected to false";
        #endif
        textInputUnselect(tid, "Text Input Mouse Drag Callback 2");
    }

    tid->lastActiveTime = TIME();
}

void textInputMouseUpCallback(
    UIElement* elem,
    float mouseX,
    float mouseY,
    glm::vec2 res
) {
    if (elem->textInputData == nullptr) { return; }

    TextInputData * tid = elem->textInputData;

    tid->mouseDown = false;

    if (
        tid->textSelected
        && tid->textSelectStartCol == tid->textSelectEndCol
        && tid->textSelectStartRow == tid->textSelectEndRow
    ) {
        textInputUnselect(tid, "Text Input Mouse Up Callback");
    }
}

// TODO: reimpl for content block
void textInputUnselect(TextInputData * tid, string token) {
    if (tid == nullptr) { return; }

    #ifdef DEBUG_TEXT_SELECTION
        cout << "\nMarked selected to false";
        if (!token.empty()) {
            cout << " (" << token << ")";
        }
    #endif

    tid->textSelected = false;
    // tid->cursorPosition = tid->textSelectStartCol;
    // tid->textSelectStartCol = tid->textSelectStartCol;
    tid->textSelectEndCol = tid->textSelectStartCol;
    tid->initialCursorPosition = tid->textSelectStartCol;
    tid->textSelectEndRow = tid->textSelectStartRow;
}

void scrollAreaMouseDownCallback(UIElement* elem, float mouseX, float mouseY, glm::vec2 res) {
    if (elem->type == UIElementType::ScrollArea || elem->type == UIElementType::ContentContainer) {
        float top, left, bottom, right, thumbStart, thumbEnd;

        if (elem->scrollData->showVerticalScrollbar) {
            getScrollbarDimensions(
                elem, top, right, bottom, left,
                thumbStart, thumbEnd,
                true
            );

            if (
                isPointInside(mouseX, mouseY, left, right, top, bottom)
                && !isPointInside(mouseX, mouseY, left, right, thumbStart, thumbEnd)
            ) {
                float height = elem->height;
                float barHeight = thumbEnd - thumbStart;
                float newThumbStart = mouseY - elem->top - barHeight/2;

                if (newThumbStart < 0) {
                    newThumbStart = 0;
                }

                if (elem->scrollData->showHorizontalScrollbar) {
                    height -= SCROLL_BAR_THICKNESS;
                }

                if (newThumbStart + barHeight > height) {
                    newThumbStart = elem->top + height - barHeight;
                }

                elem->scrollData->yOffset = newThumbStart * elem->scrollData->height / height;
                elem->scrollData->renderedUpdate = false;
                elem->scrollData->lastScrolled = glfwGetTime();
                return;
            }
        }

        if (elem->scrollData->showHorizontalScrollbar) {
            getScrollbarDimensions(
                elem, top, right, bottom, left,
                thumbStart, thumbEnd,
                false
            );

            if (
                isPointInside(mouseX, mouseY, left, right, top, bottom)
                && !isPointInside(mouseX, mouseY, left, right, thumbStart, thumbEnd)
            ) {
                float width = elem->width;
                float barWidth = thumbEnd - thumbStart;
                float newThumbStart = mouseX - elem->left - barWidth/2;

                if (newThumbStart < 0) {
                    newThumbStart = 0;
                }

                if (elem->scrollData->showVerticalScrollbar) {
                    width -= SCROLL_BAR_THICKNESS;
                }

                if (newThumbStart + barWidth > elem->width) {
                    newThumbStart = elem->left + width - barWidth;
                }

                elem->scrollData->xOffset = newThumbStart * elem->scrollData->width / width;
                elem->scrollData->renderedUpdate = false;
                elem->scrollData->lastScrolled = glfwGetTime();
            }
        }
    }
}

void scrollAreaMouseDragCallback(UIElement* elem, float mouseX, float mouseY, glm::vec2 res) {
    if (glfwGetTime() - elem->scrollData->lastScrolled < SCROLL_DEBOUNCE_DELAY) {
        return;
    }

    scrollAreaMouseDownCallback(elem, mouseX, mouseY, res);
}

// TODO: delete
void selectTextWithMouse(
    UIElement* elem,
    float mouseX,
    float mouseY,
    float mouseDownX,
    float mouseDownY,
    bool mouseUp,
    glm::vec2& res
) {
    if (elem->textInputData == nullptr) { return; }

    TextInputData * tid = elem->textInputData;

    if (!tid->textSelected && mouseUp) {
        uint32_t currentIndex = 0;
        float x = elem->left + BUTTON_PADDING;
        float y = elem->top + BUTTON_PADDING;

        // cout << "\nMouse x: " << mouseX << " y: " << mouseY;

        for (const char c: elem->text) {
            GlyphDimensions gd = getGlyphDimensions(c, x, y, res, elem->fontSize);

            // cout << "\ncharacter: " << c << ", leftX: " << gd.leftX
            //     << ", rightX: " << gd.rightX << ", bottomY: " << gd.bottomY
            //     << ", topY: " << gd.topY << "\nx: " << x;

            uint32_t net_advance = (static_cast<uint32_t>(gd.advance * gd.scale) >> 6);

            if (static_cast<float>(x) + static_cast<float>(net_advance)/2 > mouseX) {
                break;
            }

            x += net_advance;

            ++currentIndex;
        }

        tid->cursorPosition = currentIndex;
    }
}

void initElement(UIElement& elem, UIElementType type, float top, float left, float width, float height) {
    glm::vec4 backgroundColor = TRANSPARENT_COLOR;
    glm::vec4 hoverColor = TRANSPARENT_COLOR;
    glm::vec4 color = darkTheme ? WHITE : BLACK;
    uint8_t borderRadius = 0;
    UIPositioning positioning = UIPositioning::Absolute;

    uint8_t fontSize = 16;

    UIPreferences* pref = nullptr;
    TextInputData* tid = nullptr;
    ScrollData* sd = nullptr;

    switch (type) {
        case UIElementType::Button: {
            pref = initPreferences();

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

        case UIElementType::IconButton:
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

        case UIElementType::Label:
            pref = initPreferences();
            pref->widthPreference = UISizePreference::Grow;

            backgroundColor = TRANSPARENT_COLOR;
            hoverColor = TRANSPARENT_COLOR;

            if (darkTheme) {
                color = WHITE;
            } else {
                color = BLACK;
            }
            break;

        case UIElementType::TabBar:
        case UIElementType::Container:
            pref = initPreferences();

            backgroundColor = TRANSPARENT_COLOR;
            hoverColor = TRANSPARENT_COLOR;

            if (darkTheme) {
                color = WHITE;
            } else {
                color = BLACK;
            }
            break;

        case UIElementType::Explorer:
        case UIElementType::ContentContainer:
        case UIElementType::ScrollArea:
            pref = initPreferences();
            pref->widthPreference = UISizePreference::Grow;

            sd = new ScrollData {
                0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
                false, false, false, false,
                0, 0, 0, 0, 0, 0, 0, 0
            };

            backgroundColor = TRANSPARENT_COLOR;
            hoverColor = TRANSPARENT_COLOR;

            if (darkTheme) {
                color = WHITE;
            } else {
                color = BLACK;
            }

            break;

        case UIElementType::TextArea:
            // rope = new Rope();
        case UIElementType::TextInput:
            pref = initPreferences();

            // backgroundColor = TRANSPARENT_COLOR;
            backgroundColor = darkTheme ? TEXT_INPUT_BG_DARK : TEXT_INPUT_BG;
            hoverColor = darkTheme ? TEXT_INPUT_HOVER_BG_DARK : TEXT_INPUT_HOVER_BG;

            if (darkTheme) {
                color = WHITE;
            } else {
                color = BLACK;
            }

            tid = new TextInputData {
                .cursorPosition = 0,
                .cursorVAO = 0,
                .cursorVBO = 0,
                .textSelectStartRow = 0,
                .textSelectStartCol = 0,
                .textSelectEndRow = 0,
                .textSelectEndCol = 0,
                .initialCursorPosition = 0,
                .initialRow = 0,
                .initialCol = 0,
                .prevRow = 0,
                .textSelected = false,
                .mouseDown = false,
                .textSelectUpdated = false,
                .password = false,
                .alignment = TextInputAlignment::Left,
                .mouseDownX = 0.0f,
                .mouseDownY = 0.0f,
                .lastActiveTime = 0.0f,
                .cmdList = CommandList(),
            };

            break;

        default:
            backgroundColor = TRANSPARENT_COLOR;
            hoverColor = TRANSPARENT_COLOR;
            color = BLACK;
    }

    elem = {
        type,
        0,
        top,
        left,
        width,
        height,

        false,
        false,
        true,
        true,

        "",
        // nullptr,

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

        0,
        0,
        0,
        0,

        Icon::None,

        fontSize,
        borderRadius,
        positioning,
        pref,
        tid,
        sd,
    };
}

void insertContentBlock(uint32_t tabIndex, ContentBlockType type, string text, uint32_t index) {
    if (tabIndex >= tabs.size()) {
        return;
    }

    ContentTab* tab = &tabs[tabIndex];

    ContentBlock cb = ContentBlock {
        type,
        CONTENT_BLOCK_REDRAW | CONTENT_BLOCK_VISIBLE,
        0.0f,
        0.0f,
        0.0f,
        0, 0, 0, 0,
        new Rope(text),
    };

    auto itr = tab->contentBlocks.begin() + index;

    tab->contentBlocks.insert(itr, cb);
}

void appendContentBlock(uint32_t tabIndex, ContentBlockType type, string text) {
    if (tabIndex >= tabs.size()) {
        return;
    }

    tabs[tabIndex].contentBlocks.push_back(ContentBlock {
        type,
        CONTENT_BLOCK_REDRAW | CONTENT_BLOCK_VISIBLE,
        0.0f,
        0.0f,
        0.0f,
        0, 0, 0, 0,
        new Rope(text),
    });
}

void moveContentBlock(uint32_t tabIndex, uint32_t newIndex, uint32_t oldIndex) {
    if (tabIndex >= tabs.size()) {
        cerr << "\nError: Invalid tab!";
        return;
    }

    if (newIndex == oldIndex || newIndex == 0 || oldIndex == 0) {
        return;
    }

    ContentTab* tab = &tabs[tabIndex];

    if (abs(static_cast<int64_t>(newIndex) - static_cast<int64_t>(oldIndex)) == 1) {
        ContentBlock temp = tab->contentBlocks[newIndex];
        tab->contentBlocks[newIndex] = tab->contentBlocks[oldIndex];
        tab->contentBlocks[oldIndex] = temp;
        return;
    }

    if (newIndex < oldIndex) {
        ContentBlock temp = tab->contentBlocks[newIndex];
        tab->contentBlocks[newIndex] = tab->contentBlocks[oldIndex];

        for (
            auto itr = tab->contentBlocks.begin() + newIndex + 1;
            itr != tab->contentBlocks.begin() + oldIndex + 1;
            ++itr
        ) {
            ContentBlock temp2 = *itr;
            *itr = temp;
            temp = temp2;
        }

        return;
    }

    ContentBlock temp = tab->contentBlocks[oldIndex];
    tab->contentBlocks[oldIndex] = tab->contentBlocks[newIndex];

    for (
        auto itr = tab->contentBlocks.begin() + oldIndex + 1;
        itr != tab->contentBlocks.begin() + newIndex + 1;
        ++itr
    ) {
        ContentBlock temp2 = *itr;
        *itr = temp;
        temp = temp2;
    }

    #ifdef DEBUG_CONTENT_BLOCKS
        printContentBlocks(activeTabIndex);
    #endif
}

void newTab(filesystem::path path = "Untitled Note") {
    vector<ContentBlock> content;
    vector<ContentCommand> commands;
    vector<ContentCommandExt> extCommands;

    commands.reserve(8);

    extCommands.reserve(4);

    content.push_back(ContentBlock {
        ContentBlockType::Title,
        CONTENT_BLOCK_REDRAW | CONTENT_BLOCK_VISIBLE,
        0.0f, 0.0f, 0.0f,
        0, 0, 0, 0,
        new Rope(path.filename().string()),
    });

    uint32_t vao[4];
    uint32_t vbo[4];

    float vertices[] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };

    for (int i=0; i<4; ++i) {
        SETUP_BUFFERS(vao[i], vbo[i], vertices);
    }

    commands.push_back(ContentCommand { CommandType::None, false, 0, 0, 0, "", });

    tabs.push_back(ContentTab {
        true, true, false, false, false, false, false, false,
        0.0f, 0.0f, 0.0f, 0.0f,
        1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        8, vao[0], vbo[0], vao[1], vbo[1], vao[2], vbo[2], vao[3], vbo[3], 0, 0,
        commands, extCommands, content, path
    });

    activeTabIndex = tabs.size() - 1;

    SETUP_BUFFERS(tabsDisplay[activeTabIndex].activeBgVao, tabsDisplay[activeTabIndex].activeBgVbo, vertices);

    TAB_BAR.reDraw = true;
    WELCOME_SCREEN.show = false;
    activeElement = &CONTENT_CONTAINER;
}

void closeCurrentTab() {
    ContentTab* tab = &tabs[activeTabIndex];

    for (auto itr = tab->extCommands.begin(); itr != tab->extCommands.end(); ++itr) {
        for (auto ibItr = (*itr).insertedBlocks.begin(); ibItr != (*itr).insertedBlocks.end(); ++ibItr) {
            if ((*ibItr).rope != nullptr) {
                delete (*ibItr).rope;
                (*ibItr).rope = nullptr;
            }
        }

        for (auto delItr = (*itr).deletedBlocks.begin(); delItr != (*itr).deletedBlocks.end(); ++delItr) {
            if ((*delItr).rope != nullptr) {
                delete (*delItr).rope;
                (*delItr).rope = nullptr;
            }
        }
    }

    for (auto itr = tab->contentBlocks.begin(); itr != tab->contentBlocks.end(); ++itr) {
        if ((*itr).rope != nullptr) {
            delete (*itr).rope;
            (*itr).rope = nullptr;
        }
    }

    if (tab->selVAO != 0) {
        glDeleteVertexArrays(1, &tab->selVAO);
    }

    if (tab->selVBO != 0) {
        glDeleteBuffers(1, &tab->selVBO);
    }

    if (tab->cursorVAO != 0) {
        glDeleteVertexArrays(1, &tab->cursorVAO);
    }

    if (tab->cursorVBO != 0) {
        glDeleteBuffers(1, &tab->cursorVBO);
    }

    if (tab->scrollbarVAO != 0) {
        glDeleteVertexArrays(1, &tab->scrollbarVAO);
    }

    if (tab->scrollbarVAO != 0) {
        glDeleteBuffers(1, &tab->scrollbarVBO);
    }

    if (tab->scrollThumbVAO != 0) {
        glDeleteVertexArrays(1, &tab->scrollThumbVAO);
    }

    if (tab->scrollThumbVAO != 0) {
        glDeleteBuffers(1, &tab->scrollThumbVBO);
    }

    tabs.erase(tabs.begin() + activeTabIndex);

    if (activeTabIndex >= tabs.size() && activeTabIndex > 0) {
        activeTabIndex = tabs.size() - 1;
    }

    TAB_BAR.reDraw = true;

    if (tabs.empty()) {
        WELCOME_SCREEN.show = true;
        activeElement = nullptr;
        TAB_BAR.show = false;
        CONTENT_CONTAINER.show = false;
        recalculateLayout(&WELCOME_SCREEN);
    }

    updateWindowTitle();
}

string getSelectedTextFromCurrentTab() {
    ContentTab* tab = &tabs[activeTabIndex];

    size_t contentBlocks = tab->contentBlocks.size();

    if (
        tab->textSelected
        && tab->selStartCBIndex < contentBlocks
        && tab->selEndCBIndex < contentBlocks
    ) {
        if (tab->selStartCBIndex == tab->selEndCBIndex) {
            return tab->contentBlocks[tab->selStartCBIndex]
                .rope->getString()
                .substr(
                    tab->selStartCursorPosition,
                    tab->selEndCursorPosition - tab->selStartCursorPosition
                );
        } else {
            stringstream ss;

            const auto beginIter = tab->contentBlocks.begin() + tab->selStartCBIndex;
            const auto endIter = tab->contentBlocks.begin() + tab->selEndCBIndex + 1;

            for (
                auto cbIter = tab->contentBlocks.begin() + tab->selStartCBIndex;
                cbIter != endIter && cbIter != tab->contentBlocks.end();
                ++cbIter
            ) {
                RopeTraverser rt((*cbIter).rope);

                while (rt.hasNext()) {
                    ss << rt.next()->text;
                }

                if (cbIter + 1 != endIter && cbIter + 1 != tab->contentBlocks.end()) {
                    ss << "\n";
                }
            }

            return ss.str();
        }
    }

    return "";
}

UIPreferences* initPreferences() {
    UIPreferences* pref = new UIPreferences;

    pref->maxWidth = -1;
    pref->maxHeight = -1;
    pref->direction = true;
    pref->verticalAlign = UIAlignment::Start;
    pref->horizontalAlign = UIAlignment::Start;
    pref->heightPreference = UISizePreference::Fixed;
    pref->widthPreference = UISizePreference::Fixed;

    return pref;
}

void initializeCapacity(UIElement* parent, int capacity) {
    if (parent->children == nullptr) {
        parent->children = new UIElement*[capacity];
        parent->capacity = capacity;
    }
}

void growCapacity(UIElement* parent) {
    if (parent->capacity > GROWTH_THRESHOLD) {
        parent->capacity += INCR_POST_GROWTH_THRESHOLD;
    } else {
        parent->capacity *= GROWTH_FACTOR;
    }

    UIElement** newlist = new UIElement*[parent->capacity];

    for (uint32_t i=0; i<parent->totalChildren; ++i) {
        newlist[i] = parent->children[i];
    }

    delete [] parent->children;

    parent->children = newlist;
}

void addChild(UIElement* parent, UIElement * child) {
    if (child->parent != nullptr) {
        cerr << "\nError: child already has a parent";
    }

    if (parent->capacity == 0) {
        initializeCapacity(parent, INITIAL_PARENT_CAPACITY);
    } else if (parent->totalChildren + 1 > parent->capacity) {
        growCapacity(parent);
    }

    parent->children[parent->totalChildren] = child;
    child->parent = parent;

    ++parent->totalChildren;
}

inline void toggleExplorer() {
    EXPLORER.show = !EXPLORER.show;
    highlightedContentBlock.show = false;
    highlightedContentBlock.updated = true;
    recalculateLayout(&MAIN_CONTAINER);

    if (CONTENT_CONTAINER.show) {
        CURRENT_TAB->redrawCursor = true;
    }

    recalculateContentWidth();
}

inline bool updateElementDimensionsAccordingToPreferences(UIElement* elem) {
    bool updated = false;

    if (elem->preferences != nullptr) {
        switch (elem->preferences->widthPreference) {
            case UISizePreference::Full:
                if (elem->parent != nullptr) {
                    #ifdef DEBUG_LAYOUT
                        cout << "\n-> Resizing width of element (text: "
                            << elem->text << ") with UI_FULL width preference.";
                    #endif

                    elem->width = elem->parent->width;
                    updated = true;
                }
                break;

            case UISizePreference::Grow:
                if (elem->parent != nullptr) {
                    if(!elem->parent->preferences->direction) {
                        elem->left = elem->parent->left + (elem->parent->width - elem->width)/2;
                        updated = true;
                        break;
                    }
                }
                // no break intentional

            case UISizePreference::Stretch:
                if (elem->parent != nullptr && elem->parent->preferences->direction) {
                    #ifdef DEBUG_LAYOUT
                        cout << "\n-> Resizing width of element (text: "
                            << elem->text << ") with UI_STRETCH width preference";
                    #endif

                    float width = elem->parent->width;
                    float left = elem->parent->left;
                    bool currElemFound = false;

                    UIElement* siblingElem = nullptr;

                    uint32_t totalChildren = elem->parent->totalChildren;
                    uint32_t stretchCandidates = 0;

                    for (uint32_t i=0; i<elem->parent->totalChildren; ++i) {
                        siblingElem = elem->parent->children[i];

                        if (elem == siblingElem) {
                            currElemFound = true;
                        }

                        if (
                            siblingElem != elem
                            && siblingElem->show
                            && siblingElem->preferences != nullptr
                        ) {
                            switch (siblingElem->preferences->widthPreference) {
                                case UISizePreference::Fixed:
                                case UISizePreference::Grow:
                                    if (!currElemFound) {
                                        left += siblingElem->width;
                                    }

                                    width -= siblingElem->width;
                                    break;

                                case UISizePreference::Percent:
                                    width -= elem->parent->width * siblingElem->width;

                                case UISizePreference::Stretch:
                                    if (!currElemFound) {
                                        left += siblingElem->width;
                                    }

                                    width -= siblingElem->width;
                                    ++stretchCandidates;
                                    break;

                                default:
                                    // Do nothing!
                                    break;
                            }
                        }
                    }

                    if (stretchCandidates) {
                        width /= stretchCandidates;
                    }

                    if (width < 0) {
                        #ifdef DEBUG_LAYOUT
                            cout << "\nLAYOUT ERROR: width less than 0";
                        #endif
                        break;
                    }

                    elem->left = left;
                    elem->width = width;

                    if (elem->type == UIElementType::ScrollArea && elem->scrollData->width < width) {
                        cout << "\n!!!!!";
                        elem->scrollData->width = width;
                        elem->scrollData->xOffset = 0;
                    }

                    updated = true;
                }
                break;

            default:
                break;
        }

        switch (elem->preferences->heightPreference) {
            case UISizePreference::Full:
                if (elem->parent != nullptr) {
                    #ifdef DEBUG_LAYOUT
                        cout << "\n-> Resizing height of element (text: "
                            << elem->text << ") with UI_FULL height preference.";
                    #endif

                    elem->height = elem->parent->height;
                    updated = true;
                }
                break;

            case UISizePreference::Stretch:
                if (elem->parent != nullptr) {
                    #ifdef DEBUG_LAYOUT
                        cout << "\n-> Resizing height of element (text: "
                            << elem->text << ") with UI_STRETCH height preference.";
                    #endif

                    float height = elem->parent->height;

                    UIElement* siblingElem = nullptr;

                    uint32_t totalChildren = elem->parent->totalChildren;
                    uint32_t stretchCandidates = 0;

                    for (uint32_t i=0; i<elem->parent->totalChildren; ++i) {
                        siblingElem = elem->parent->children[i];

                        if (siblingElem != elem && siblingElem->preferences != nullptr) {
                            switch (siblingElem->preferences->heightPreference) {
                                case UISizePreference::Fixed:
                                case UISizePreference::Grow:
                                    if (!siblingElem->parent->preferences->direction) {
                                        height -= siblingElem->height;
                                    }
                                    break;

                                case UISizePreference::Percent:
                                    height -= elem->parent->height * siblingElem->height;

                                case UISizePreference::Stretch:
                                    ++stretchCandidates;
                                    break;

                                default:
                                    // Do nothing!
                                    break;
                            }
                        }
                    }

                    if (stretchCandidates) {
                        height /= stretchCandidates;
                    }

                    if (height < 0) {
                        cout << "\nLAYOUT ERROR: height less than 0";
                        break;
                    }

                    elem->height = height;
                    updated = true;
                }
                break;

            default:
                break;
        }

        switch (elem->preferences->verticalAlign) {
            case UIAlignment::Start:
                if (elem->parent != nullptr && elem->positioning == UIPositioning::Absolute) {
                    elem->top = elem->parent->top;
                }
                break;

            case UIAlignment::End:
                if (elem->parent != nullptr && elem->positioning == UIPositioning::Absolute) {
                    elem->top = elem->parent->top + elem->parent->height - elem->height;
                }
                break;

            case UIAlignment::Center:
                if (elem->parent != nullptr && elem->height < elem->parent->height && elem->positioning == UIPositioning::Absolute) {
                    elem->top = elem->parent->top + (elem->parent->height - elem->height) / 2;
                }
                break;
        }

        switch (elem->preferences->horizontalAlign) {
            case UIAlignment::Start:
                if (elem->parent != nullptr) {
                    elem->left = elem->parent->left;
                }
                break;

            case UIAlignment::End:
                if (elem->parent != nullptr) {
                    elem->left = elem->parent->width - elem->width;
                }
                break;

            case UIAlignment::Center:
                if (elem->parent != nullptr) {
                    elem->left = elem->parent->left + (elem->parent->width - elem->width) / 2;
                }
                break;
        }
    }

    return updated;
}

void recalculateLayout(UIElement * elem) {
    elem->reDraw = true;

    if (updateElementDimensionsAccordingToPreferences(elem)) {
        #ifdef DEBUG_SCROLL_AREA
            cout << "\n    -> Updated dimensions: width: " << elem->width << ", height: " << elem->height;
        #endif

        if (elem->type == UIElementType::ScrollArea || elem->type == UIElementType::ContentContainer) {
            elem->scrollData->renderedUpdate = false;
            elem->reDraw = true;

            if (!tabs.empty() && !tabs[activeTabIndex].contentBlocks.empty()) {
                reDrawAllContentBlocks();

                elem->scrollData->height = 0;
            }

            if (elem->scrollData->width < elem->width) {
                elem->scrollData->xOffset = 0;
            }
        }
    }

    for (uint32_t i=0; i<elem->totalChildren; ++i) {
        if (elem->children[i]->show) {
            recalculateLayout(elem->children[i]);
        }
    }
}

inline void recalculateContentWidth() {
    if (CONTENT_CONTAINER.width >= CONTENT_BREAKPOINT) {
        contentLeft = CONTENT_CONTAINER.left + (CONTENT_CONTAINER.width - CONTENT_MAX_WIDTH) / 2 + CONTENT_HORIZONTAL_MARGIN;
        contentRight = contentLeft + CONTENT_MAX_WIDTH - 2*CONTENT_HORIZONTAL_MARGIN;
    } else {
        contentLeft = CONTENT_CONTAINER.left + CONTENT_HORIZONTAL_MARGIN;
        contentRight = contentLeft + CONTENT_CONTAINER.width - 2*CONTENT_HORIZONTAL_MARGIN;
    }
}

void setupScrollbars(
    UIElement* elem,
    bool vertical, uint32_t& vVAO, uint32_t& vVBO, uint32_t& vtVAO, uint32_t& vtVBO,
    bool horizontal, uint32_t& hVAO, uint32_t& hVBO, uint32_t& htVAO, uint32_t& htVBO
) {
    // WARNING: side effects!
    float top, left, bottom, right, thumbStart, thumbEnd;

    if (vertical && vVAO == 0) {
        getScrollbarDimensions(elem, top, right, bottom, left, thumbStart, thumbEnd, true);

        float vertices[] = { right, bottom, right, elem->top, left, bottom, left, elem->top };
        float thumbVertices[] = { right, thumbEnd, right, thumbStart, left, thumbEnd, left, thumbStart };

        SETUP_BUFFERS_DYNAMIC(vVAO, vVBO, vertices);
        SETUP_BUFFERS_DYNAMIC(vtVAO, vtVBO, thumbVertices);
    }

    if (horizontal && hVAO == 0) {
        getScrollbarDimensions(elem, top, right, bottom, left, thumbStart, thumbEnd, false);

        float vertices[] = { right, bottom, right, elem->top, left, bottom, left, elem->top };
        float thumbVertices[] = { right, thumbEnd, right, thumbStart, left, thumbEnd, left, thumbStart };

        SETUP_BUFFERS_DYNAMIC(hVAO, hVBO, vertices);
        SETUP_BUFFERS_DYNAMIC(htVAO, htVBO, thumbVertices);
    }
}

void updateScrollbars(
    UIElement* elem,
    bool vertical, uint32_t vVAO, uint32_t vVBO, uint32_t vtVAO, uint32_t vtVBO,
    bool horizontal, uint32_t hVAO, uint32_t hVBO, uint32_t htVAO, uint32_t htVBO
) {
    #define UPDATE(vao, vbo, vertices) do { \
        glBindVertexArray(vao);\
        glBindBuffer(GL_ARRAY_BUFFER, vbo);\
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);\
    } while (false)

    // WARNING: side effects!
    float top, left, bottom, right, thumbStart, thumbEnd;

    if (vertical) {
        getScrollbarDimensions(elem, top, right, bottom, left, thumbStart, thumbEnd, true);

        float vertices[] = { right, bottom, right, elem->top, left, bottom, left, elem->top };
        float thumbVertices[] = { right, thumbEnd, right, thumbStart, left, thumbEnd, left, thumbStart };

        UPDATE(vVAO, vVBO, vertices);
        UPDATE(vtVAO, vtVBO, thumbVertices);
    }

    if (vertical) {
        getScrollbarDimensions(elem, top, right, bottom, left, thumbStart, thumbEnd, true);

        float vertices[] = { right, bottom, right, elem->top, left, bottom, left, elem->top };
        float thumbVertices[] = { right, thumbEnd, right, thumbStart, left, thumbEnd, left, thumbStart };

        UPDATE(hVAO, hVBO, vertices);
        UPDATE(htVAO, htVBO, thumbVertices);
    }
}

// NOTE: only for UIElementType::ContentContainer and UIElementType::ScrollArea.
inline void renderScrollbars(UIElement* elem) {
    #define SCROLL_DATA elem->scrollData
    #define DRAW(vao, c) do {\
        glBindVertexArray(vao);\
        glUseProgram(shaders[SHADER_DEFAULT].programId);\
        glUniform4f(uniforms[U_DEFAULT_COLOR], c.r, c.g, c.b, c.a);\
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);\
    } while (false)

    if (
        SCROLL_DATA->showVerticalScrollbar && SCROLL_DATA->verticalScrollBarVAO == 0
        || SCROLL_DATA->showHorizontalScrollbar && SCROLL_DATA->horizontalScrollBarVAO == 0
    ) {
        setupScrollbars(
            elem,
            SCROLL_DATA->showVerticalScrollbar,
            SCROLL_DATA->verticalScrollBarVAO, SCROLL_DATA->verticalScrollBarVBO,
            SCROLL_DATA->verticalScrollThumbVAO, SCROLL_DATA->verticalScrollThumbVBO,
            SCROLL_DATA->showHorizontalScrollbar,
            SCROLL_DATA->horizontalScrollBarVAO, SCROLL_DATA->horizontalScrollBarVBO,
            SCROLL_DATA->horizontalScrollThumbVAO, SCROLL_DATA->horizontalScrollThumbVBO
        );
    } else if (!SCROLL_DATA->renderedUpdate) {
        updateScrollbars(
            elem,
            SCROLL_DATA->showVerticalScrollbar,
            SCROLL_DATA->verticalScrollBarVAO, SCROLL_DATA->verticalScrollBarVBO,
            SCROLL_DATA->verticalScrollThumbVAO, SCROLL_DATA->verticalScrollThumbVBO,
            SCROLL_DATA->showHorizontalScrollbar,
            SCROLL_DATA->horizontalScrollBarVAO, SCROLL_DATA->horizontalScrollBarVBO,
            SCROLL_DATA->horizontalScrollThumbVAO, SCROLL_DATA->horizontalScrollThumbVBO
        );
    }

    glm::vec4 trackColor = darkTheme ? SCROLL_BAR_TRACK_COLOR_DARK : SCROLL_BAR_TRACK_COLOR;
    glm::vec4 thumbColor = darkTheme ? SCROLL_BAR_THUMB_COLOR_DARK : SCROLL_BAR_THUMB_COLOR;

    if (SCROLL_DATA->showVerticalScrollbar) {
        DRAW(SCROLL_DATA->verticalScrollBarVAO, trackColor);
        DRAW(SCROLL_DATA->verticalScrollThumbVAO, thumbColor);
    }

    if (SCROLL_DATA->showHorizontalScrollbar) {
        DRAW(SCROLL_DATA->horizontalScrollBarVAO, trackColor);
        DRAW(SCROLL_DATA->horizontalScrollThumbVAO, thumbColor);
    }

    SCROLL_DATA->renderedUpdate = true;

    #undef SCROLL_DATA
    #undef DRAW
}

// NOTE: only for UIElementType::ContentContainer.
inline void renderCBScrollbar() {
    ContentTab* tab = &tabs[activeTabIndex];

    bool update = !tab->scrollbarUpdated;

    // WARNING: side effects!
    float top, left, bottom, right, thumbStart, thumbEnd;

    if (update) {
        // WARNING: side effects!
        getCBScrollbarDimensions(
            tab, top, right, bottom, left,
            thumbStart, thumbEnd
        );
    }

    #ifdef DEBUG_SCROLL_AREA
        cout << "\nShowing vertical scrollbar. Element width: " << elem->width;

        cout << "\n--> Vertical scrollbar dimensions. top: "
            << top << ", left: " << left << ", bottom: "
            << bottom << ", right:" << right
            << ", thumbStart: " << thumbStart << ", thumbEnd: " << thumbEnd;
    #endif

    glBindVertexArray(tab->scrollbarVAO);
    glBindBuffer(GL_ARRAY_BUFFER, tab->scrollbarVBO);

    if (update) {
        float vertices[] = {
            right, bottom,
            right, CONTENT_CONTAINER.top,
            left, bottom,
            left, CONTENT_CONTAINER.top,
        };

        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    }

    glUseProgram(shaders[SHADER_DEFAULT].programId);

    glm::vec4 color = SCROLL_BAR_TRACK_COLOR;

    if (darkTheme) {
        color = SCROLL_BAR_TRACK_COLOR_DARK;
    }

    glUniform4f(uniforms[U_DEFAULT_COLOR], color.r, color.g, color.b, color.a);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glBindVertexArray(tab->scrollThumbVAO);
    glBindBuffer(GL_ARRAY_BUFFER, tab->scrollThumbVBO);

    if (update) {
        float vertices[] = {
            right, thumbEnd,
            right, thumbStart,
            left, thumbEnd,
            left, thumbStart,
        };

        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    }

    glUseProgram(shaders[SHADER_DEFAULT].programId);

    glUniform4f(uniforms[U_DEFAULT_COLOR], 0.6f, 0.6f, 0.6f, 1.0f);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    tab->scrollbarUpdated = true;
}

inline void recalculateCBCursor() {
    SelectionData sd = getCBTextCursor(resolution);
    updateCBCursor(CURRENT_TAB, sd);
}

void updateCBCursor(ContentTab* tab, SelectionData sd) {
    tab->showCursor = true;
    tab->redrawCursor = false;

    if (
        tab->cursorPosition == sd.cursorPosition
        && tab->cursorRow == sd.row
        && tab->cursorCBIndex == sd.cursorCBIndex
    ) {
        return;
    }

    if (tab->updateCursorX) {
        tab->cursorX = static_cast<uint16_t>(sd.x);
        tab->updateCursorX = false;
    }

    tab->cursorY = static_cast<uint16_t>(sd.y) + (static_cast<uint16_t>(sd.height) >> 1);

    if (sd.y - tab->scrollOffsetY > CONTENT_CONTAINER.height + CONTENT_CONTAINER.top) {
        tab->scrollOffsetY += sd.y - CONTENT_CONTAINER.height - CONTENT_CONTAINER.top;
        tab->redrawCursor = true;
        highlightedContentBlock.show = false;
        tab->scrollbarUpdated = false;
    } else if (sd.y - sd.height < CONTENT_CONTAINER.top && sd.y != 0 && sd.height != 0) {
        tab->scrollOffsetY -= CONTENT_CONTAINER.top - sd.y + sd.height;
        tab->redrawCursor = true;
        tab->scrollbarUpdated = false;
    }

    tab->cursorPosition = sd.cursorPosition;
    tab->lastCursorTime = TIME();
    tab->cursorRow = sd.row;
    tab->cursorCBIndex = sd.cursorCBIndex;

    glBindVertexArray(tab->cursorVAO);
    glBindBuffer(GL_ARRAY_BUFFER, tab->cursorVBO);

    float vertices[] = {
        sd.x + 1.0f, sd.y - sd.height,
        sd.x + 1.0f, sd.y,
        sd.x - 1.0f, sd.y - sd.height,
        sd.x - 1.0f, sd.y,
    };

    #ifdef DEBUG_TEXT
    cout << "\nSetting cursor dimensions: " << sd.x + 1 << ", "
        << sd.y - sd.height << "; " << sd.x + 1 << ", " << sd.y << "; " << sd.x
        << ", " << sd.y - sd.height << "; " << sd.x << ", " << sd.y;
    #endif

    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void renderElement(UIElement& elem, glm::vec2 &resolution, glm::vec2 &cursor, uint32_t childIndex, float& yPrev) {
    switch (elem.type) {
        case UIElementType::Container: {
            if (!elem.show) { break; }

            if (elem.reDraw) {
                updateElementDimensionsAccordingToPreferences(&elem);
            }

            if (elem.backgroundColor.a != 0) {
                if (elem.reDraw) {
                    #ifdef DEBUG_RENDER
                        cout << "\nRe-drawing Container";
                    #endif

                    float right = elem.left + elem.width;
                    float bottom = elem.top + elem.height;

                    float vertices[] = {
                        right, bottom,
                        right, elem.top,
                        elem.left, bottom,
                        elem.left, elem.top,
                    };

                    glBindVertexArray(elem.vao);
                    glBindBuffer(GL_ARRAY_BUFFER, elem.vbo);

                    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
                }

                glBindVertexArray(elem.vao);
                glUseProgram(shaders[SHADER_DEFAULT].programId);

                glUniform2f(uniforms[U_DEFAULT_RESOLUTION], resolution.x, resolution.y);
                glUniform4f(
                    uniforms[U_DEFAULT_COLOR],
                    elem.backgroundColor.x,
                    elem.backgroundColor.y,
                    elem.backgroundColor.z,
                    elem.backgroundColor.w
                );

                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            }

            float oldHeight = elem.height;

            if (elem.totalChildren) {
                float yp = 0.0f;

                for (uint32_t i=0; i<elem.totalChildren; ++i) {
                    if (elem.children[i]->show) {
                        renderElement(*elem.children[i], resolution, cursor, i, yp);
                    }
                }

                if (
                    elem.reDraw
                    && !elem.preferences->direction
                    && elem.preferences->heightPreference == UISizePreference::Grow
                ) {
                    elem.height = yp - elem.top - elem.children[0]->top
                        + elem.children[elem.totalChildren - 1]->height;
                }
            }

            if (oldHeight != elem.height) {
                elem.reDraw = true;

                for (uint32_t i=0; i<elem.totalChildren; ++i) {
                    elem.children[i]->reDraw = true;
                }
            } else {
                elem.reDraw = false;
            }

            break;
        }

        case UIElementType::ContentContainer: {
            elem.reDraw = false;

            if (tabs.empty()) { return; }

            ContentTab* tab = CURRENT_TAB;

            float width = elem.width;
            float height = elem.height;

            bool showScrollbar = tab->maxHeight > elem.height;

            if (showScrollbar) {
                width -= SCROLL_BAR_THICKNESS;
            }

            glScissor(elem.left, resolution.y - elem.top - elem.height, width, height);
            glEnable(GL_SCISSOR_TEST);

            renderContentBlockHighlight();
            renderCBTextSelection(tab);

            uint32_t i=0;

            bool visible = false;

            for (
                auto itr = tab->contentBlocks.begin();
                itr != tab->contentBlocks.end();
                ++itr, ++i
            ) {
                if (renderContentBlock(tab, itr, resolution, cursor)) {
                    if (!visible) {
                        tab->firstVisibleCBIndex = i;
                    }

                    visible = true;
                } else if (visible) {
                    #ifdef DEBUG_CONTENT_BLOCKS
                        // cout << "\nDEBUG: breaking render at " << to_string((ContentBlock)(*itr));
                    #endif
                    break;
                }
            }

            tab->visibleCBLength = i - tab->firstVisibleCBIndex + 1;

            renderCBMoveMarker();

            if (tab->redrawCursor) {
                SelectionData sd = getCBTextCursor(resolution);

                if (tab->updateCursorX) {
                    tab->cursorX = static_cast<uint16_t>(sd.x);
                    tab->updateCursorX = false;
                }
                tab->cursorY = static_cast<uint16_t>(sd.y) + (static_cast<uint16_t>(sd.height) >> 1);
                tab->cursorRow = sd.row;

                #ifdef DEBUG_TEXT
                cout << "\nSetting cursor dimensions (redraw): " << sd.x + 1 << ", "
                    << sd.y - sd.height << "; " << sd.x + 1 << ", " << sd.y << "; " << sd.x
                    << ", " << sd.y - sd.height << "; " << sd.x << ", " << sd.y;
                #endif

                if (tab->showCursor && !tab->textSelected) {
                    float vertices[] = {
                        sd.x + 1.0f, sd.y - sd.height,
                        sd.x + 1.0f, sd.y,
                        sd.x - 1.0f, sd.y - sd.height,
                        sd.x - 1.0f, sd.y,
                    };

                    glBindVertexArray(tab->cursorVAO);
                    glBindBuffer(GL_ARRAY_BUFFER, tab->cursorVBO);
                    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
                }

                tab->redrawCursor = false;
            }

            if (tab->showCursor && !tab->textSelected) {
                glBindVertexArray(tab->cursorVAO);
                glBindBuffer(GL_ARRAY_BUFFER, tab->cursorVBO);

                glUseProgram(shaders[SHADER_TEXT_CURSOR].programId);

                glm::vec4 color = CURSOR_COLOR;

                if (darkTheme) {
                    color = CURSOR_COLOR_DARK;
                }

                glUniform4f(uniforms[U_TEXT_CURSOR_COLOR], color.x, color.y, color.z, color.w);
                glUniform2f(uniforms[U_TEXT_CURSOR_RESOLUTION], resolution.x, resolution.y);
                glUniform1f(uniforms[U_TEXT_CURSOR_TIME], TIME() - tab->lastCursorTime);

                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            }

            glDisable(GL_SCISSOR_TEST);

            renderCBScrollbar();
            break;
        }

        case UIElementType::Label: {
            if (!elem.show) { break; }

            glm::vec4 dim;

            glm::vec2 pos = glm::vec2(elem.left, elem.top);

            if (elem.positioning == UIPositioning::Relative) {
                pos.x = elem.parent->left + elem.left;
                pos.y = elem.parent->top + elem.top;

                if (elem.parent->type == UIElementType::ScrollArea) {
                    pos.x -= elem.parent->scrollData->xOffset;
                    pos.y -= elem.parent->scrollData->yOffset;
                }

                RopeRenderResult rrr = renderTextCached(
                    elem.text,
                    pos,
                    resolution,
                    elem.textVAO,
                    elem.textVBO,
                    elem.textBufferSize,
                    elem.textVerticesCount,
                    elem.parent->left,
                    elem.parent->width,
                    elem.color,
                    elem.fontSize,
                    elem.reDraw,
                    false
                );

                if (
                    elem.reDraw
                    && elem.preferences->widthPreference == UISizePreference::Grow
                    && elem.parent->type == UIElementType::ScrollArea
                    && elem.parent->scrollData->width < rrr.width
                ) {
                    elem.parent->scrollData->width = rrr.width;
                    elem.reDraw = false;
                }

                if (elem.parent->type == UIElementType::ScrollArea) {
                    if (elem.parent->scrollData->height < rrr.height) {
                        #ifdef DEBUG_SCROLL_AREA
                            cout << "\nScroll height less than dim.z " << dim.z;
                        #endif

                        elem.parent->scrollData->height = rrr.height;
                        elem.parent->scrollData->renderedUpdate = false;
                        elem.parent->reDraw = true;
                    }
                }
                break;
            }

            if (
                elem.positioning == UIPositioning::Relative_sibling
                && elem.parent != nullptr
            ) {
                if (childIndex == 0) {
                    pos.x = elem.parent->left + elem.left;
                    pos.y = elem.parent->top + elem.top;
                } else {
                    if (elem.parent->preferences->direction) {
                        pos.x = elem.parent->left
                            + elem.parent->children[childIndex - 1]->left
                            + elem.parent->children[childIndex - 1]->width;
                    } else {
                        pos.y = yPrev
                            + elem.parent->children[childIndex - 1]->height
                            + elem.top;
                    }
                }
            }

            if (
                elem.reDraw
                && elem.preferences->widthPreference == UISizePreference::Grow
            ) {
                RopeRenderResult rRes = getTextDimensions(
                    elem.text, pos, resolution, elem.fontSize,
                    elem.parent->left, elem.parent->width, false
                );

                elem.width = rRes.width;
                elem.height = rRes.height;
                updateElementDimensionsAccordingToPreferences(&elem);
                pos.x = elem.left;

                if (
                    elem.parent->type == UIElementType::ScrollArea
                    && elem.parent->scrollData->width < elem.width
                ) {
                    elem.parent->scrollData->width = elem.width;
                }
            }

            renderTextCached(
                elem.text,
                pos,
                resolution,
                elem.textVAO,
                elem.textVBO,
                elem.textBufferSize,
                elem.textVerticesCount,
                elem.parent->left,
                elem.parent->width,
                elem.color,
                elem.fontSize,
                elem.reDraw,
                false
            );

            if (
                elem.positioning == UIPositioning::Relative_sibling
                && elem.parent != nullptr
            ) {
                yPrev = pos.y;
            }

            elem.reDraw = false;
            break;
        }

        case UIElementType::Button: {
            float top = elem.top;
            float left = elem.left;

            if (elem.parent->type == UIElementType::ScrollArea) {
                top -= elem.parent->scrollData->yOffset;
            }

            if (
                elem.positioning == UIPositioning::Relative_sibling
                && elem.parent != nullptr
            ) {
                if (childIndex == 0) {
                    left = elem.parent->left + elem.left;
                    top = elem.parent->top + elem.top;
                } else {
                    if (elem.parent->preferences->direction) {
                        left = elem.parent->left
                            + elem.parent->children[childIndex - 1]->left
                            + elem.parent->children[childIndex - 1]->width
                            + elem.left;
                    } else {
                        top = yPrev
                            + elem.parent->children[childIndex - 1]->height
                            + elem.top;
                    }
                }
            }

            if (elem.reDraw) {
                if (elem.parent->type == UIElementType::ScrollArea) {
                    if (
                        top < elem.parent->top
                        || top > elem.parent->top + elem.parent->height
                    ) {
                        elem.show = false;
                        elem.reDraw = false;
                        return;
                    }
                }

                if (
                    elem.preferences->widthPreference == UISizePreference::Grow
                    || elem.preferences->heightPreference == UISizePreference::Grow
                ) {
                    glm::vec2 pos = glm::vec2(elem.left + BUTTON_PADDING, top + BUTTON_PADDING);

                    RopeRenderResult rRes = getTextDimensions(
                        elem.text, pos, resolution, elem.fontSize,
                        elem.parent->left, elem.parent->width, false
                    );

                    if (elem.preferences->widthPreference == UISizePreference::Grow) {
                        elem.width = BUTTON_PADDING + rRes.width + BUTTON_PADDING;
                    }

                    if (elem.preferences->heightPreference == UISizePreference::Grow) {
                        elem.height = BUTTON_PADDING + rRes.height + BUTTON_PADDING;
                    }
                }

                if (elem.preferences->widthPreference == UISizePreference::Full) {
                    updateElementDimensionsAccordingToPreferences(&elem);
                }

                if (elem.preferences->horizontalAlign == UIAlignment::Center) {
                    elem.left = elem.parent->left + (elem.parent->width - elem.width) / 2;
                    left = elem.left;
                }

                #ifdef DEBUG_RENDER
                    cout << "\nRe-drawing";
                #endif

                float right = left + elem.width;
                float bottom = top + elem.height;

                float vertices[] = {
                    right, bottom,
                    right, top,
                    left, bottom,
                    left, top,
                };

                glBindVertexArray(elem.vao);
                glBindBuffer(GL_ARRAY_BUFFER, elem.vbo);
                glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
            }

            if (!elem.show) { return; }

            glBindVertexArray(elem.vao);
            glUseProgram(shaders[SHADER_BUTTON].programId);

            glUniform2f(uniforms[U_BUTTON_RESOLUTION], resolution.x, resolution.y);
            glUniform4f(
                uniforms[U_BUTTON_BG_COLOR],
                elem.backgroundColor.x,
                elem.backgroundColor.y,
                elem.backgroundColor.z,
                elem.backgroundColor.w
            );
            glUniform4f(
                uniforms[U_BUTTON_HOV_COLOR],
                elem.hoverColor.x,
                elem.hoverColor.y,
                elem.hoverColor.z,
                elem.hoverColor.w
            );

            glUniform2f(uniforms[U_BUTTON_TOP_LEFT], left, top);
            glUniform2f(uniforms[U_BUTTON_DIMENSIONS], elem.width, elem.height);
            glUniform1f(uniforms[U_BUTTON_BORDER_RADIUS], static_cast<float>(elem.borderRadius));
            glUniform2f(uniforms[U_BUTTON_CURSOR], cursor.x, cursor.y);

            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

            glm::vec2 pos = glm::vec2(left + BUTTON_PADDING, top + BUTTON_PADDING);

            renderTextCached(
                elem.text,
                pos,
                resolution,
                elem.textVAO,
                elem.textVBO,
                elem.textBufferSize,
                elem.textVerticesCount,
                elem.parent->left,
                elem.parent->width,
                elem.color,
                elem.fontSize,
                elem.reDraw,
                false
            );

            if (
                elem.positioning == UIPositioning::Relative_sibling
                && elem.parent != nullptr
            ) {
                yPrev = top;
            }

            elem.reDraw = false;

            break;
        }

        case UIElementType::ScrollArea: {
            // TODO: Render the rectangle with background colors. If borders
            // are set, then render borders.

            if (elem.totalChildren) {
                float width = elem.width;
                float height = elem.height;

                if (elem.reDraw) {
                    if (elem.preferences->horizontalAlign == UIAlignment::Center) {
                        elem.left = elem.parent->left + (elem.parent->width - elem.width) / 2;
                    }

                    glBindVertexArray(elem.vao);
                    glBindBuffer(GL_ARRAY_BUFFER, elem.vbo);

                    float vertices[8] = {
                        elem.left,              elem.top,
                        elem.left + elem.width, elem.top,
                        elem.left + elem.width, elem.top + elem.height,
                        elem.left,              elem.top + elem.height,
                    };

                    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

                    elem.scrollData->height = 0.0f;
                }

                if (elem.scrollData->showVerticalScrollbar) {
                    width -= SCROLL_BAR_THICKNESS;
                }

                if (elem.scrollData->showHorizontalScrollbar) {
                    height -= SCROLL_BAR_THICKNESS;
                }

                glScissor(elem.left, resolution.y - elem.top - elem.height, width, height);
                glEnable(GL_SCISSOR_TEST);

                float yp = 0;

                for (uint32_t i=0; i<elem.totalChildren; ++i) {
                    if (elem.children[i]->show || elem.reDraw) {
                        renderElement(*elem.children[i], resolution, cursor, i, yp);
                    }

                    if (elem.reDraw) {
                        elem.scrollData->height += elem.children[i]->height;
                    }
                }

                glDisable(GL_SCISSOR_TEST);

                if (elem.reDraw) {
                    elem.scrollData->showVerticalScrollbar = elem.scrollData->height > elem.height;
                    elem.scrollData->showHorizontalScrollbar = elem.scrollData->width > elem.width;

                    #ifdef DEBUG_SCROLL_AREA
                        cout << "\nShow vertical scrollbar: " << elem.scrollData->showVerticalScrollbar;
                        cout << "\nShow horizontal scrollbar: " << elem.scrollData->showHorizontalScrollbar;
                    #endif

                    elem.reDraw = false;
                }

                renderScrollbars(&elem);

                glBindVertexArray(elem.vao);
                glUseProgram(shaders[SHADER_DEFAULT].programId);
                glUniform4f(uniforms[U_DEFAULT_COLOR], 0.4f, 0.4f, 0.4f, 1.0f);
                glDrawArrays(GL_LINE_LOOP, 0, 4);
            }
            break;
        }

        case UIElementType::IconButton: {
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
                    #ifdef DEBUG_RENDER
                        cout << "\nRedrawing icon button";
                    #endif

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

            // cout << "icon button color: r: " << elem.color.r << " g: " << elem.color.g << " b: " << elem.color.b << " a: " << elem.color.a;

            glUniform2f(uniforms[U_ICON_BUTTON_RESOLUTION], resolution.x, resolution.y);
            glUniform4f(
                uniforms[U_ICON_BUTTON_BG_COLOR],
                elem.backgroundColor.r,
                elem.backgroundColor.g,
                elem.backgroundColor.b,
                elem.backgroundColor.a
            );
            glUniform4f(
                uniforms[U_ICON_BUTTON_COLOR],
                elem.color.r,
                elem.color.g,
                elem.color.b,
                elem.color.a
            );
            glUniform4f(
                uniforms[U_ICON_BUTTON_HOV_COLOR],
                elem.hoverColor.r,
                elem.hoverColor.g,
                elem.hoverColor.b,
                elem.hoverColor.a
            );
            glUniform2f(uniforms[U_ICON_BUTTON_TOP_LEFT], elem.left, elem.top);
            glUniform2f(uniforms[U_ICON_BUTTON_CURSOR], cursor.x, cursor.y);
            glUniform1f(uniforms[U_ICON_BUTTON_WIDTH], elem.width);
            glUniform1f(uniforms[U_ICON_BUTTON_HEIGHT], elem.height);

            if (elem.icon != Icon::None) {
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
                        #ifdef DEBUG_TEXTURE
                            cout << "\nTexture " << location << " loaded.";
                        #endif

                        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
                        glGenerateMipmap(GL_TEXTURE_2D);
                    } else {
                        cerr << "\nError: Failed to load texture (" << location << ").";
                    }

                    stbi_image_free(data);
                }

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, elem.texture);
            }

            glUniform1i(uniforms[U_ICON_BUTTON_TEXTURE], 0);

            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

            elem.reDraw = false;
            break;
        }

        case UIElementType::TextInput: {
            if (elem.reDraw) {
                #ifdef DEBUG_RENDER
                    cout << "\nRe-drawing Text Input";
                #endif

                updateElementDimensionsAccordingToPreferences(&elem);

                float right = elem.left + elem.width;
                float bottom = elem.top + elem.height;

                float vertices[] = {
                    right, bottom,
                    right, elem.top,
                    elem.left, bottom,
                    elem.left, elem.top,
                };

                glBindVertexArray(elem.vao);
                glBindBuffer(GL_ARRAY_BUFFER, elem.vbo);
                glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
            }

            glBindVertexArray(elem.vao);

            glUseProgram(shaders[SHADER_TEXT_INPUT].programId);

            glUniform2f(uniforms[U_TEXT_INPUT_RESOLUTION], resolution.x, resolution.y);
            glUniform4f(
                uniforms[U_TEXT_INPUT_COLOR],
                elem.color.r,
                elem.color.g,
                elem.color.b,
                elem.color.a
            );
            glUniform4f(
                uniforms[U_TEXT_INPUT_BG_COLOR],
                elem.backgroundColor.r,
                elem.backgroundColor.g,
                elem.backgroundColor.b,
                elem.backgroundColor.a
            );
            glUniform4f(
                uniforms[U_TEXT_INPUT_HOV_COLOR],
                elem.hoverColor.r,
                elem.hoverColor.g,
                elem.hoverColor.b,
                elem.hoverColor.a
            );

            glUniform2f(uniforms[U_TEXT_INPUT_TOP_LEFT], elem.left, elem.top);
            glUniform2f(uniforms[U_TEXT_INPUT_DIMENSIONS], elem.width, elem.height);
            glUniform1f(uniforms[U_TEXT_INPUT_BORDER_RADIUS], static_cast<float>(elem.borderRadius));
            glUniform2f(uniforms[U_TEXT_INPUT_CURSOR], cursor.x, cursor.y);

            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

            TextInputData* tid = elem.textInputData;

            glm::vec2 pos = glm::vec2(elem.left + TEXT_INPUT_PADDING, elem.top + TEXT_INPUT_PADDING);

            string text;
            text.reserve(elem.text.length());

            if (tid->password) {
                for (int i=0; i < elem.text.length(); ++i) {
                    text += "*";
                }
            } else {
                text = elem.text;
            }

            RopeRenderResult rRes = getTextDimensions(
                text,
                pos,
                resolution,
                elem.fontSize,
                elem.parent->left,
                elem.parent->width
            );

            switch (tid->alignment) {
                case TextInputAlignment::Center:
                    pos.x = elem.left + (elem.width - rRes.width)/2;
                    pos.y = elem.top + TEXT_INPUT_PADDING;
                    break;

                case TextInputAlignment::Right:
                    pos.x = elem.left + elem.width - rRes.width;
                    pos.y = elem.top + TEXT_INPUT_PADDING;
                    break;
            }

            renderTextCached(
                text,
                pos,
                resolution,
                elem.textVAO,
                elem.textVBO,
                elem.textBufferSize,
                elem.textVerticesCount,
                elem.parent->left,
                elem.parent->width,
                elem.color,
                elem.fontSize,
                elem.reDraw,
                false
            );

            uint32_t currentIndex = 0;
            float x = elem.left + TEXT_INPUT_PADDING;
            float y = elem.top + TEXT_INPUT_PADDING;

            float cursorX = -1;
            float cursorY = -1;
            float selectStartX = -1;
            float selectEndX = -1;

            if (tid->cursorPosition == elem.text.length()) {
                cursorX = pos.x + rRes.width;
                cursorY = y;
            } else if (tid->cursorPosition == 0) {
                cursorX = pos.x;
                cursorY = y;
            }

            // When text input is active, draw the cursor.
            if (elem.active) {
                float vertices[] = {
                    cursorX - 1, cursorY + static_cast<float>(elem.fontSize) * (1 - glyphDescender),
                    cursorX + 1, cursorY + static_cast<float>(elem.fontSize) * (1 - glyphDescender),
                    cursorX - 1, cursorY,
                    cursorX + 1, cursorY,
                };

                if (tid->cursorVAO == 0) {
                    SETUP_BUFFERS(tid->cursorVAO, tid->cursorVBO, vertices);
                } else {
                    glBindVertexArray(tid->cursorVAO);
                    glBindBuffer(GL_ARRAY_BUFFER, tid->cursorVBO);

                    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
                }

                // cout << "\ncursorX: " << cursorX << " cursorY: " << cursorY;

                glUseProgram(shaders[SHADER_TEXT_CURSOR].programId);

                glm::vec4 color = darkTheme ? CURSOR_COLOR_DARK : CURSOR_COLOR;

                glUniform4f(uniforms[U_TEXT_CURSOR_COLOR], color.x, color.y, color.z, color.w);
                glUniform2f(uniforms[U_TEXT_CURSOR_RESOLUTION], resolution.x, resolution.y);
                glUniform1f(uniforms[U_TEXT_CURSOR_TIME], TIME() - tid->lastActiveTime);

                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

                if (tid->textSelected && selectStartX != -1 && selectEndX != -1) {
                    if (tid->textSelectUpdated) {
                        float vertices[] = {
                            selectStartX, cursorY + elem.fontSize,
                            selectEndX, cursorY + elem.fontSize,
                            selectStartX, cursorY,
                            selectEndX, cursorY,
                        };

                        if (tid->textSelectVAO == 0) {
                            SETUP_BUFFERS(tid->textSelectVAO, tid->textSelectVBO, vertices);
                        } else {
                            glBindVertexArray(tid->textSelectVAO);
                            glBindBuffer(GL_ARRAY_BUFFER, tid->textSelectVBO);
                            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
                        }

                        tid->textSelectUpdated = false;
                        #ifdef DEBUG_TEXT_SELECTION
                            cout << "\ncursorY: " << cursorY;
                        #endif
                    }

                    glBindVertexArray(tid->textSelectVAO);

                    glUseProgram(shaders[SHADER_DEFAULT].programId);

                    glm::vec4 color = darkTheme ? CURSOR_COLOR_DARK : CURSOR_COLOR;

                    glUniform4f(uniforms[U_DEFAULT_COLOR], color.x, color.y, color.z, 0.25);

                    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
                }
            }

            elem.reDraw = false;

            break;
        }

        case UIElementType::TextArea: {
            if (elem.reDraw) {
                #ifdef DEBUG_RENDER
                    cout << "\nRe-drawing Text Area" << "\nwidth: " << elem.width << ", height: " << elem.height;
                #endif

                float right = elem.left + elem.width;
                float bottom = elem.top + elem.height;

                float vertices[] = {
                    right, bottom,
                    right, elem.top,
                    elem.left, bottom,
                    elem.left, elem.top,
                };

                glBindVertexArray(elem.vao);
                glBindBuffer(GL_ARRAY_BUFFER, elem.vbo);
                glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

                elem.reDraw = false;
            }

            glBindVertexArray(elem.vao);
            glUseProgram(shaders[SHADER_TEXT_INPUT].programId);

            glUniform2f(uniforms[U_TEXT_INPUT_RESOLUTION], resolution.x, resolution.y);
            glUniform4f(
                uniforms[U_TEXT_INPUT_COLOR],
                elem.color.x,
                elem.color.y,
                elem.color.z,
                elem.color.w
            );
            glUniform4f(
                uniforms[U_TEXT_INPUT_BG_COLOR],
                elem.backgroundColor.x,
                elem.backgroundColor.y,
                elem.backgroundColor.z,
                elem.backgroundColor.w
            );
            glUniform4f(
                uniforms[U_TEXT_INPUT_HOV_COLOR],
                elem.hoverColor.x,
                elem.hoverColor.y,
                elem.hoverColor.z,
                elem.hoverColor.w
            );

            glUniform2f(uniforms[U_TEXT_INPUT_TOP_LEFT], elem.left, elem.top);
            glUniform2f(uniforms[U_TEXT_INPUT_DIMENSIONS], elem.width, elem.height);

            // glUniform1f(uniforms[U_TEXT_INPUT_TIME], TIME());
            glUniform1f(uniforms[U_TEXT_INPUT_BORDER_RADIUS], static_cast<float>(elem.borderRadius));
            glUniform2f(uniforms[U_TEXT_INPUT_CURSOR], cursor.x, cursor.y);

            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

            glUseProgram(shaders[SHADER_TEXT].programId);

            glUniform4f(
                uniforms[U_TEXT_COLOR],
                elem.color.r,
                elem.color.g,
                elem.color.b,
                elem.color.a
            );
            glUniform2f(uniforms[U_TEXT_RESOLUTION], resolution.x, resolution.y);
            glActiveTexture(GL_TEXTURE0);
            glBindVertexArray(textVAO);
            glBindBuffer(GL_ARRAY_BUFFER, textVBO);

            uint32_t currentIndex = 0;
            uint32_t row = 0;
            uint32_t col = 0;
            const float xStart = elem.left + TEXT_INPUT_PADDING; 
            float x = xStart;
            float y = elem.top + TEXT_INPUT_PADDING;

            float cursorX = -1;
            float cursorY = -1;
            float selectStartX = -1;
            float selectStartY = -1;
            float selectEndX = -1;
            float selectEndY = -1;

            bool selectStarted = false;
            bool selectEnded = false;

            TextInputData* tid = elem.textInputData;

            vector<float> selectionVertices;
            selectionVertices.reserve(18);

            const float newLineSize = static_cast<float>(elem.fontSize)
                + static_cast<float>(BUTTON_PADDING);

            const float heightIncrement = static_cast<float>(elem.fontSize + LINE_GAP);

            for (const char c: elem.text) {
                if (c == '\n') {
                    if (tid->textSelected && selectStarted && !selectEnded) {
                        const float y1 = y + elem.fontSize;

                        selectEndX = x;
                        selectEndY = y;

                        selectionVertices.push_back(selectStartX);
                        selectionVertices.push_back(y1);
                        selectionVertices.push_back(selectEndX);
                        selectionVertices.push_back(y1);
                        selectionVertices.push_back(selectStartX);
                        selectionVertices.push_back(y);

                        selectionVertices.push_back(selectEndX);
                        selectionVertices.push_back(y1);
                        selectionVertices.push_back(selectStartX);
                        selectionVertices.push_back(y);
                        selectionVertices.push_back(selectEndX);
                        selectionVertices.push_back(y);
                    }

                    y += heightIncrement;
                    x = xStart;
                    col = 0;
                    ++row;

                    selectStartX = x;
                    selectStartY = y;
                    continue;
                }

                if (currentIndex == tid->cursorPosition) {
                    cursorX = x;
                    cursorY = y;
                }

                if (tid->textSelected && !selectEnded) {
                    if (col == tid->textSelectStartCol && row == tid->textSelectStartRow) {
                        selectStartX = x;
                        selectStartY = y;
                        selectStarted = true;
                    }

                    if (col == tid->textSelectEndCol && row == tid->textSelectEndRow) {
                        selectEndX = x;
                        selectEndY = y;
                        selectEnded = true;

                        const float y1 = y + elem.fontSize;

                        selectionVertices.push_back(selectStartX);
                        selectionVertices.push_back(y1);
                        selectionVertices.push_back(selectEndX);
                        selectionVertices.push_back(y1);
                        selectionVertices.push_back(selectStartX);
                        selectionVertices.push_back(y);

                        selectionVertices.push_back(selectEndX);
                        selectionVertices.push_back(y1);
                        selectionVertices.push_back(selectStartX);
                        selectionVertices.push_back(y);
                        selectionVertices.push_back(selectEndX);
                        selectionVertices.push_back(y);
                    }
                }

                x += renderGlyph(c, x, y, resolution, elem.fontSize);
                ++col;

                ++currentIndex;
            }

            if (tid->cursorPosition == elem.text.length()) {
                cursorX = x;
                cursorY = y;
            }

            if (tid->textSelected && tid->textSelectEndCol == elem.text.length()) {
                cursorX = x;
                cursorY = y;
                selectEndX = x;

                const float y1 = y + elem.fontSize;
                selectionVertices.push_back(selectStartX);
                selectionVertices.push_back(y1);
                selectionVertices.push_back(selectEndX);
                selectionVertices.push_back(y1);
                selectionVertices.push_back(selectStartX);
                selectionVertices.push_back(y);

                selectionVertices.push_back(selectEndX);
                selectionVertices.push_back(y1);
                selectionVertices.push_back(selectStartX);
                selectionVertices.push_back(y);
                selectionVertices.push_back(selectEndX);
                selectionVertices.push_back(y);
            }

            #ifdef DEBUG_TEXT_SELECTION
                if (tid->textSelected) {
                    printTextSelectionVertices(selectionVertices);
                }
            #endif

            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);
            glBindTexture(GL_TEXTURE_2D, 0);

            // When text input is active, draw the cursor.
            if (elem.active) {
                if (tid->cursorVAO != 0) {
                    glDeleteVertexArrays(1, &tid->cursorVAO);
                }

                if (tid->cursorVBO != 0) {
                    glDeleteBuffers(1, &tid->cursorVBO);
                }

                // cout << "\ncursorX: " << cursorX << " cursorY: " << cursorY;

                float vertices[] = {
                    cursorX - 1, cursorY + elem.fontSize,
                    cursorX + 1, cursorY + elem.fontSize,
                    cursorX - 1, cursorY,
                    cursorX + 1, cursorY,
                };

                SETUP_BUFFERS(tid->cursorVAO, tid->cursorVBO, vertices);

                glUseProgram(shaders[SHADER_TEXT_CURSOR].programId);

                glm::vec4 color = CURSOR_COLOR;

                if (darkTheme) {
                    color = CURSOR_COLOR_DARK;
                }

                glUniform4f(uniforms[U_TEXT_CURSOR_COLOR], color.x, color.y, color.z, color.w);
                glUniform2f(uniforms[U_TEXT_CURSOR_RESOLUTION], resolution.x, resolution.y);
                glUniform1f(uniforms[U_TEXT_CURSOR_TIME], TIME() - tid->lastActiveTime);

                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

                if (tid->textSelected && selectStartX != -1 && selectEndX != -1) {
                    if (tid->textSelectUpdated && !selectionVertices.empty()) {
                        if (tid->textSelectVAO != 0) {
                            glDeleteVertexArrays(1, &tid->textSelectVAO);
                        }

                        if (tid->textSelectVBO != 0) {
                            glDeleteBuffers(1, &tid->textSelectVBO);
                        }

                        // float vertices[] = {
                        //     selectStartX, cursorY + elem.fontSize,
                        //     selectEndX, cursorY + elem.fontSize,
                        //     selectStartX, cursorY,
                        //     selectEndX, cursorY,
                        // };

                        glGenVertexArrays(1, &tid->textSelectVAO);
                        glBindVertexArray(tid->textSelectVAO);
                        glGenBuffers(1, &tid->textSelectVBO);
                        glBindBuffer(GL_ARRAY_BUFFER, tid->textSelectVBO);
                        glBufferData(GL_ARRAY_BUFFER, selectionVertices.size() * sizeof(float), &selectionVertices[0], GL_STATIC_DRAW);
                        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
                        glEnableVertexAttribArray(0);

                        tid->textSelectUpdated = false;
                        #ifdef DEBUG_TEXT_SELECTION
                            cout << "\ncursorY: " << cursorY;
                        #endif
                    }

                    glBindVertexArray(tid->textSelectVAO);

                    glUseProgram(shaders[SHADER_DEFAULT].programId);

                    glm::vec4 color = CURSOR_COLOR;

                    if (darkTheme) {
                        color = CURSOR_COLOR_DARK;
                    }

                    glUniform4f(uniforms[U_DEFAULT_COLOR], color.x, color.y, color.z, 0.25);

                    glDrawArrays(GL_TRIANGLES, 0, selectionVertices.size() / 2);
                }
            }

            break;
        }

        case UIElementType::TabBar: {
            if (tabs.empty()) { return; }
            bool drawCachedText = false;

            if (elem.reDraw) {
                #ifdef DEBUG_RENDER
                    cout << "\nRe-drawing Container";
                #endif

                float right = elem.left + elem.width;
                float bottom = elem.top + elem.height;

                float vertices[] = {
                    right, bottom,
                    right, elem.top,
                    elem.left, bottom,
                    elem.left, elem.top,
                };

                glBindVertexArray(elem.vao);
                glBindBuffer(GL_ARRAY_BUFFER, elem.vbo);

                glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
            } else {
                drawCachedText = true;
            }

            glBindVertexArray(elem.vao);
            glUseProgram(shaders[SHADER_DEFAULT].programId);

            glUniform2f(uniforms[U_DEFAULT_RESOLUTION], resolution.x, resolution.y);
            glUniform4f(
                uniforms[U_DEFAULT_COLOR],
                elem.backgroundColor.x,
                elem.backgroundColor.y,
                elem.backgroundColor.z,
                elem.backgroundColor.w
            );

            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

            if (elem.reDraw) {
                float x = 0.0f;
                float y = TAB_VERTICAL_PADDING;

                for (int i=0; i<tabs.size(); ++i) {
                    x += TAB_HORIZONTAL_PADDING;

                    if (i == activeTabIndex) {
                        glBindVertexArray(tabsDisplay[i].activeBgVao);
                        glBindBuffer(GL_ARRAY_BUFFER, tabsDisplay[i].activeBgVbo);
                        glUseProgram(shaders[SHADER_DEFAULT].programId);
                        glm::vec4 color = ROOT_BG_DARK;
                        glUniform4f(uniforms[U_DEFAULT_COLOR], color.r, color.g, color.b, color.a);
                        glUniform2f(uniforms[U_DEFAULT_RESOLUTION], resolution.x, resolution.y);
                        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
                    }

                    uint32_t bytes = static_cast<uint32_t>(tabsDisplay[i].bytes);
                    uint32_t verts = static_cast<uint32_t>(tabsDisplay[i].numVertices);

                    RopeRenderResult rRes = renderTextCached(
                        tabs[i].filePath.filename().string().substr(0, 16),
                        { x, y },
                        resolution,
                        tabsDisplay[i].vao,
                        tabsDisplay[i].vbo,
                        verts,
                        bytes,
                        elem.left,
                        elem.width,
                        WHITE,
                        16,
                        true
                    );

                    if (i == activeTabIndex) {
                        float vertices[8] = {
                            x + rRes.width + TAB_HORIZONTAL_PADDING, 0.0f,
                            x + rRes.width + TAB_HORIZONTAL_PADDING, TAB_BAR_HEIGHT,
                            x - TAB_HORIZONTAL_PADDING, 0.0f,
                            x - TAB_HORIZONTAL_PADDING, TAB_BAR_HEIGHT,
                        };

                        glBindVertexArray(tabsDisplay[i].activeBgVao);
                        glBindBuffer(GL_ARRAY_BUFFER, tabsDisplay[i].activeBgVbo);

                        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
                    }

                    x += rRes.width + TAB_HORIZONTAL_PADDING;

                    tabsDisplay[i].width = rRes.width;
                    tabsDisplay[i].bytes = bytes;
                    tabsDisplay[i].numVertices = verts;
                }
            } else {
                vector<float> v;
                uint32_t bytes;
                uint32_t verts;

                for (int i=0; i<tabs.size(); ++i) {
                    bytes = static_cast<uint32_t>(tabsDisplay[i].bytes);
                    verts = static_cast<uint32_t>(tabsDisplay[i].numVertices);

                    if (i == activeTabIndex) {
                        glBindVertexArray(tabsDisplay[i].activeBgVao);
                        glBindBuffer(GL_ARRAY_BUFFER, tabsDisplay[i].activeBgVbo);
                        glUseProgram(shaders[SHADER_DEFAULT].programId);
                        glm::vec4 color = ROOT_BG_DARK;
                        glUniform4f(uniforms[U_DEFAULT_COLOR], color.r, color.g, color.b, color.a);
                        glUniform2f(uniforms[U_DEFAULT_RESOLUTION], resolution.x, resolution.y);
                        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
                    }

                    drawTextBuffers(
                        tabsDisplay[i].vao,
                        tabsDisplay[i].vbo,
                        bytes,
                        verts,
                        resolution,
                        v,
                        WHITE,
                        false
                    );
                }
            }

            elem.reDraw = false;

            break;
        }

        case UIElementType::Tab: {
            break;
        }

        case UIElementType::Explorer: {
            if (!elem.show) {
                break;
            }

            glBindVertexArray(elem.vao);

            if (elem.reDraw) {
                updateElementDimensionsAccordingToPreferences(&elem);

                glBindBuffer(GL_ARRAY_BUFFER, elem.vbo);

                float vertices[] = {
                    elem.left, elem.top,
                    elem.left, elem.top + elem.height,
                    elem.left + elem.width, elem.top,
                    elem.left + elem.width, elem.top + elem.height,
                };

                glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
            }

            glUseProgram(shaders[SHADER_DEFAULT].programId);
            glUniform4f(
                uniforms[U_DEFAULT_COLOR],
                elem.backgroundColor.r,
                elem.backgroundColor.g,
                elem.backgroundColor.b,
                elem.backgroundColor.a
            );
            glUniform2f(uniforms[U_DEFAULT_RESOLUTION], windowWidth, windowHeight);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

            glScissor(0, 0, elem.width, elem.height);
            glEnable(GL_SCISSOR_TEST);

            if (elem.reDraw) {
                setupExplorer();
            }

            glm::vec4 bg = darkTheme ? EXPLORER_HOVER_BG_DARK : EXPLORER_HOVER_BG;
            glBindVertexArray(explorer.activeBgVao);
            glUseProgram(shaders[SHADER_DEFAULT].programId);
            glUniform4f(uniforms[U_DEFAULT_COLOR], bg.r, bg.g, bg.b, bg.a);
            glUniform2f(uniforms[U_DEFAULT_RESOLUTION], windowWidth, windowHeight);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

            if (explorer.elementHover) {
                bg = darkTheme ? EXPLORER_HOVER_BG_DARK : EXPLORER_HOVER_BG;
                glBindVertexArray(explorer.hoverBgVao);
                glUseProgram(shaders[SHADER_DEFAULT].programId);
                glUniform4f(uniforms[U_DEFAULT_COLOR], bg.r, bg.g, bg.b, bg.a);
                glUniform2f(uniforms[U_DEFAULT_RESOLUTION], windowWidth, windowHeight);
                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            }

            glm::vec2 ___;
            vector<float> _;
            uint32_t __ = 0;

            drawTextBuffers(
                explorer.vao,
                explorer.vbo,
                __,
                explorer.textVertices,
                ___,
                _,
                elem.color, false
            );

            glDisable(GL_SCISSOR_TEST);

            if (
                elem.scrollData->showVerticalScrollbar
                || elem.scrollData->showHorizontalScrollbar
            ) {
                renderScrollbars(&elem);
            }

            if (explorer.lineHover) {
                glBindVertexArray(explorer.lineVAO);

                if (explorer.redrawLine) {
                    float vertices[8] = {
                        EXPLORER.width - 2, EXPLORER.top,
                        EXPLORER.width - 2, windowHeight,
                        EXPLORER.width,     EXPLORER.top,
                        EXPLORER.width,     windowHeight,
                    };

                    glBindBuffer(GL_ARRAY_BUFFER, explorer.lineVBO);
                    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

                    explorer.redrawLine = false;
                    elem.scrollData->renderedUpdate = false;
                }

                glUseProgram(shaders[SHADER_DEFAULT].programId);
                glUniform4f(uniforms[U_DEFAULT_COLOR], 1.0, 0.7, 0.7, 1.0);
                glUniform2f(uniforms[U_DEFAULT_RESOLUTION], windowWidth, windowHeight);

                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            }

            elem.reDraw = false;

            break;
        }

        default: break;
    }
}

void destroyElement(UIElement& elem) {
    if (elem.vao != 0) {
        glDeleteVertexArrays(1, &elem.vao);
        elem.vao = 0;
    }

    if (elem.vbo != 0) {
        glDeleteBuffers(1, &elem.vbo);
        elem.vbo = 0;
    }

    if (elem.textVAO != 0) {
        glDeleteVertexArrays(1, &elem.textVAO);
        elem.textVAO = 0;
    }

    if (elem.textVBO != 0) {
        glDeleteBuffers(1, &elem.textVBO);
        elem.textVBO = 0;
    }

    if (elem.preferences != nullptr) {
        delete elem.preferences;
        elem.preferences = nullptr;
    }

    if (elem.textInputData != nullptr) {
        glDeleteVertexArrays(1, &elem.textInputData->cursorVAO);
        glDeleteBuffers(1, &elem.textInputData->cursorVBO);
        glDeleteVertexArrays(1, &elem.textInputData->textSelectVAO);
        glDeleteBuffers(1, &elem.textInputData->textSelectVBO);

        elem.textInputData->cursorVAO = 0;
        elem.textInputData->cursorVBO = 0;
        elem.textInputData->textSelectVAO = 0;
        elem.textInputData->textSelectVBO = 0;

        delete elem.textInputData;
        elem.textInputData = nullptr;
    }

    if (elem.scrollData != nullptr) {
        glDeleteVertexArrays(1, &elem.scrollData->verticalScrollBarVAO);
        glDeleteBuffers(1, &elem.scrollData->verticalScrollBarVBO);
        glDeleteVertexArrays(1, &elem.scrollData->verticalScrollThumbVAO);
        glDeleteBuffers(1, &elem.scrollData->verticalScrollThumbVBO);
        glDeleteVertexArrays(1, &elem.scrollData->horizontalScrollBarVAO);
        glDeleteBuffers(1, &elem.scrollData->horizontalScrollBarVBO);
        glDeleteVertexArrays(1, &elem.scrollData->horizontalScrollThumbVAO);
        glDeleteBuffers(1, &elem.scrollData->horizontalScrollThumbVBO);

        elem.scrollData->verticalScrollBarVAO = 0;
        elem.scrollData->verticalScrollBarVBO = 0;
        elem.scrollData->verticalScrollThumbVAO = 0;
        elem.scrollData->verticalScrollThumbVBO = 0;
        elem.scrollData->horizontalScrollBarVAO = 0;
        elem.scrollData->horizontalScrollBarVBO = 0;
        elem.scrollData->horizontalScrollThumbVAO = 0;
        elem.scrollData->horizontalScrollThumbVBO = 0;

        delete elem.scrollData;
        elem.scrollData = nullptr;
    }

    elem.parent = nullptr;

    destroyElementChildren(elem);
}

void destroyElementChildren(UIElement& elem) {
    if (elem.totalChildren) {
        UIElement** toDelete = new UIElement*[elem.totalChildren];

        for (int i=0; i<elem.totalChildren; ++i) {
            destroyElementChildren((*elem.children[i]));
            destroyElement((*elem.children[i]));

            delete elem.children[i];
        }

        delete [] elem.children;

        elem.children = nullptr;
        elem.totalChildren = 0;
        elem.capacity = 0;
    }
}

inline glm::vec2 getActiveContentDimensions() {
    return glm::vec2 {
        windowWidth - SCROLL_BAR_THICKNESS - CONTENT_BLOCK_DRAGTAB_WIDTH,
        windowHeight - TAB_BAR_HEIGHT
    }; // width, height
}

bool renderContentBlock(
    ContentTab* tab,
    vector<ContentBlock>::iterator cb,
    glm::vec2 &resolution,
    glm::vec2 &cursor
) {
    if (!((*cb).flags & CONTENT_BLOCK_VISIBLE)) {
        cout << "\nContent block not visible";
        return false;
    }

    if (CONTENT_CONTAINER.parent == nullptr) {
        cout << "\nRender Error: UIElement's parent is NULL!";
        return false;
    }

    if (((*cb).flags & CONTENT_BLOCK_REDRAW) != 0) {
        if (cb > tabs[activeTabIndex].contentBlocks.begin()) {
            auto prev = cb - 1;

            glm::vec2 prevMargins = getCBVerticalMargins((*prev).type);
            glm::vec2 currMargins = getCBVerticalMargins((*cb).type);

            (*cb).top = (*prev).top + (*prev).height + prevMargins[1] + currMargins[0];
            (*cb).left = (*prev).left;
        } else {
            (*cb).top = CONTENT_CONTAINER.top + getCBVerticalMargins((*cb).type)[0];
            (*cb).left = CONTENT_CONTAINER.left;
        }
    }

    float top = (*cb).top - tab->scrollOffsetY;
    float left = contentLeft;
    float bottom = top + (*cb).height;

    bool visible = top >= 0 && top <= windowHeight
        || bottom >= 0 && bottom <= windowHeight
        || top < 0 && bottom > windowHeight;

    #ifdef DEBUG_TEXT
        auto t0 = std::chrono::high_resolution_clock::now();
    #endif

    RopeRenderResult res = renderContentRope(
        (*cb).rope,
        glm::vec2(left, top),
        resolution,
        (*cb).textVAO,
        (*cb).textVBO,
        (*cb).bufferSize,
        CONTENT_CONTAINER.width,
        darkTheme ? WHITE : BLACK,
        getCBFontSize((*cb).type),
        ((*cb).flags & CONTENT_BLOCK_REDRAW) != 0
    );

    #ifdef DEBUG_TEXT
        auto t1 = std::chrono::high_resolution_clock::now();

        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0);
        cumulativeTextTime += duration.count();
        ++textTimeCalculated;
    #endif

    if (((*cb).flags & CONTENT_BLOCK_REDRAW) != 0) {
        (*cb).height = (res.height == 0.0f && glyphDescender < 0) ?
            static_cast<float>(getCBFontSize((*cb).type)) * (1.0f - glyphDescender)
            :
            res.height;

        (*cb).rows = res.rows;
        (*cb).flags &= ~(CONTENT_BLOCK_REDRAW);
    }

    if (tab->maxHeight < (*cb).top + (*cb).height) {
        #ifdef DEBUG_SCROLL_AREA
            cout << "\nScroll height less than dim.z " << dim.z;
        #endif

        tab->maxHeight = (*cb).top + (*cb).height;
        tab->scrollbarUpdated = false;
        CONTENT_CONTAINER.reDraw = true;
    }

    return visible;
}

void renderContentBlockHighlight() {
    if (!highlightedContentBlock.show) {
        return;
    }

    glBindVertexArray(highlightedContentBlock.backgroundVAO);
    glBindBuffer(GL_ARRAY_BUFFER, highlightedContentBlock.backgroundVBO);

    if (highlightedContentBlock.updated) {
        float vertices[] = {
            highlightedContentBlock.right, highlightedContentBlock.top,
            highlightedContentBlock.right, highlightedContentBlock.bottom,
            highlightedContentBlock.left, highlightedContentBlock.top,
            highlightedContentBlock.left, highlightedContentBlock.bottom,
        };

        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    }

    glUseProgram(shaders[SHADER_DEFAULT].programId);

    glm::vec4 color = CONTENT_HIGHLIGHT_BG;

    if (darkTheme) {
        color = CONTENT_HIGHLIGHT_BG_DARK;
    }

    glUniform4f(uniforms[U_DEFAULT_COLOR], color.r, color.g, color.b, color.a);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glBindVertexArray(highlightedContentBlock.dragTabVAO);
    glBindBuffer(GL_ARRAY_BUFFER, highlightedContentBlock.dragTabVBO);

    if (highlightedContentBlock.updated) {
        float vertices[] = {
            highlightedContentBlock.left, highlightedContentBlock.top,
            highlightedContentBlock.left, highlightedContentBlock.bottom,
            highlightedContentBlock.left - 20.0f, highlightedContentBlock.top,
            highlightedContentBlock.left - 20.0f, highlightedContentBlock.bottom,
        };

        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    }

    glUseProgram(shaders[SHADER_DEFAULT].programId);

    color = CONTENT_HIGHLIGHT_DRAG;

    if (darkTheme) {
        color = CONTENT_HIGHLIGHT_DRAG_DARK;
    }

    glUniform4f(uniforms[U_DEFAULT_COLOR], color.r, color.g, color.b, color.a);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    highlightedContentBlock.updated = false;
}

void renderCBTextSelection(ContentTab* tab) {
    if (!tab->textSelected) {
        return;
    }

    if (tab->redrawSel) {
        vector<float> vertices;

        auto firstIter = tab->contentBlocks.begin() + tab->selStartCBIndex;
        auto lastIter = tab->contentBlocks.begin() + tab->selEndCBIndex + 1;
        bool selectionStarted = false;
        bool selectionFinished = false;

        float startX = 0.0f;
        float startY = 0.0f;
        float startY_h = 0.0f;
        float endX = 0.0f;
        float endY = 0.0f;
        float endY_h = 0.0f;

        cout << "\nRedrawing selection (" << tab->selStartCBIndex << ", " << tab->selEndCBIndex << "): ";

        for (auto cbIter = firstIter; cbIter != lastIter; ++cbIter) {
            RopeTraverser rt((*cbIter).rope);

            float fontSize = static_cast<float>(getCBFontSize((*cbIter).type));
            float fontHeightRatio = fontSize / DEFAULT_FONT_SIZE;

            uint32_t row = 0;
            uint32_t cursorPosition = 0;

            float initialX = (*cbIter).left + CONTENT_BLOCK_DRAGTAB_WIDTH;
            float initialY = (*cbIter).top;

            float xpos = initialX;
            float yNet = initialY;

            while (rt.hasNext()) {
                Rope* rope = rt.next();

                for (char c: rope->text) {
                    Character* ch = &characters[(int)c];

                    float aspectRatio = static_cast<float>(ch->width) / static_cast<float>(ch->height);

                    float glyphHeight = static_cast<float>(ch->height) * fontHeightRatio;
                    float glyphWidth = glyphHeight * aspectRatio;

                    if (glyphWidth != glyphWidth) {
                        if (ch->width != 0) {
                            glyphWidth = static_cast<float>(ch->width) / fontSize;
                        } else {
                            glyphWidth = 0.0f;
                        }
                    }

                    float widthRatio = glyphWidth / ch->width;
                    float heightRatio = glyphHeight / ch->height;

                    float bearingTop = ch->bearingTop * heightRatio;
                    float bearingLeft = ch->bearingLeft * widthRatio;

                    if (bearingLeft != bearingLeft) {
                        bearingLeft = 0.0f;
                    }

                    if (bearingTop != bearingTop) {
                        bearingTop = 0.0f;
                    }

                    xpos += bearingLeft;

                    if (
                        selectionStarted
                        && cursorPosition == tab->selEndCursorPosition
                        && cbIter == lastIter - 1
                    ) {
                        selectionFinished = true;
                        endX = xpos;
                        endY_h = yNet;
                        endY = yNet + fontSize - fontSize * glyphDescender;

                        cout << "\n  (cb: "
                            << distance(tab->contentBlocks.begin(), cbIter)
                            << ", row: " << row << ", cur: " << cursorPosition
                            << ")\n    (" << startX << ", " << startY_h << "), ("
                            << endX << ", " << endY_h << "), ("
                            << endX << ", " << endY << "), ("
                            << startX << ", " << startY_h << ")";

                        vertices.insert(vertices.end(), {
                            startX, startY_h,
                            endX,   endY_h,
                            endX,   endY,
                            endX,   endY,
                            startX, startY,
                            startX, startY_h,
                        });
                        break;
                    }

                    if (xpos + glyphWidth > contentRight) {
                        if (selectionStarted) {
                            endX = xpos;
                            endY_h = yNet;
                            endY = yNet + fontSize - fontSize * glyphDescender;

                            cout << "\n  (cb: "
                                << distance(tab->contentBlocks.begin(), cbIter)
                                << ", row: " << row << ", cur: " << cursorPosition
                                << ")\n    (" << startX << ", " << startY_h << "), ("
                                << endX << ", " << endY_h << "), ("
                                << endX << ", " << endY << "), ("
                                << startX << ", " << startY_h << ")";

                            vertices.insert(vertices.end(), {
                                startX, startY_h,
                                endX,   endY_h,
                                endX,   endY,
                                endX,   endY,
                                startX, startY,
                                startX, startY_h,
                            });
                        }

                        yNet += fontSize - fontSize * glyphDescender;
                        xpos = initialX;
                        row++;

                        startX = xpos;
                        startY_h = yNet;
                        startY = yNet + fontSize - fontSize * glyphDescender;
                    }

                    float ypos = yNet + glyphHeight - bearingTop + fontSize;

                    if (
                        (
                            !selectionStarted
                            && cursorPosition == tab->selStartCursorPosition
                            && cbIter == firstIter
                        )
                        || selectionStarted && cbIter != firstIter && row == 0
                        && cursorPosition == 0
                    ) {
                        selectionStarted = true;

                        cout << "\n  Start (cb: "
                            << distance(tab->contentBlocks.begin(), cbIter)
                            << ", row: " << row << ", cur: " << cursorPosition
                            << "), ";

                        startX = xpos;
                        startY = yNet + fontSize - fontSize * glyphDescender;
                        startY_h = yNet;
                    }

                    xpos += (static_cast<uint32_t>(ch->advance * fontHeightRatio) >> 6) - bearingLeft;
                    ++cursorPosition;
                }

                if (selectionFinished) {
                    break;
                }
            }

            if (selectionFinished) {
                break;
            } else if (selectionStarted) {
                cout << "\n  (cb: "
                    << distance(tab->contentBlocks.begin(), cbIter)
                    << ", row: " << row << ", cur: " << cursorPosition
                    << "), ";

                endX = xpos;
                endY_h = yNet;
                endY = yNet + fontSize - fontSize * glyphDescender;

                vertices.insert(vertices.end(), {
                    startX, startY_h,
                    endX,   endY_h,
                    endX,   endY,
                    endX,   endY,
                    startX, startY,
                    startX, startY_h,
                });
            }
        }

        if (vertices.empty()) {
            return;
        }

        selectionStarted = false;
        selectionFinished = false;

        if (vertices.size() > tab->selVertexCount) {
            glDeleteVertexArrays(1, &tab->selVAO);
            glDeleteBuffers(1, &tab->selVBO);

            glGenVertexArrays(1, &tab->selVAO);
            glBindVertexArray(tab->selVAO);
            glGenBuffers(1, &tab->selVBO);
            glBindBuffer(GL_ARRAY_BUFFER, tab->selVBO);
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);

            tab->selVertexCount = vertices.size();
        } else {
            for (int i=vertices.size(); i<tab->selVertexCount; ++i) {
                vertices.push_back(0.0f);
            }

            glBindVertexArray(tab->selVAO);
            glBindBuffer(GL_ARRAY_BUFFER, tab->selVBO);
            glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(float), &vertices[0]);
        }

        tab->redrawSel = false;
    }

    glBindVertexArray(tab->selVAO);
    glBindBuffer(GL_ARRAY_BUFFER, tab->selVBO);

    glUseProgram(shaders[SHADER_CB_TEXT_HIGHLIGHT].programId);

    glUniform2f(uniforms[U_TEXT_HL_RESOLUTION], resolution.x, resolution.y);
    glUniform1f(uniforms[U_TEXT_HL_SCROLL_OFFSET_Y], tab->scrollOffsetY);

    glm::vec4 textSelColor = darkTheme ? CURSOR_COLOR_DARK : CURSOR_COLOR;

    glUniform4f(uniforms[U_TEXT_HL_COLOR], textSelColor.r, textSelColor.g, textSelColor.b, 0.25f);

    glDrawArrays(GL_TRIANGLES, 0, tab->selVertexCount/2);
}

void contentBlockMouseDownCallback(UIElement* elem, float mouseX, float mouseY, glm::vec2 res) {
    #ifdef DEBUG_CONTENT_BLOCKS
        cout << "\nDEBUG: Mouse down on content block!";
    #endif

    if (tabs.empty()) { return; }

    CONTENT_CONTAINER.active = true;
    activeElement = &CONTENT_CONTAINER;

    activateCBFromScreenCoords(mouseX, mouseY);

    SelectionData sd = getCBTextCursorFromScreenCoords(mouseX, mouseY, res);

    #ifdef DEBUG_TEXT_SELECTION
        cout << "\nSelectionData: mouseX: " << mouseX << ", mouseY: " << mouseY
            << " cursorPosition: " << sd.cursorPosition << ", row: " << sd.row
            << ", col: " << sd.col << ", cursorCBIndex: " << sd.cursorCBIndex;
    #endif

    ContentTab* tab = &tabs[activeTabIndex];

    tab->updateCursorX = true;

    if (
        tab->textSelected
        && (
            sd.cursorCBIndex == tab->cursorCBIndex
            && (
                sd.cursorPosition < tab->selStartCursorPosition
                || sd.cursorPosition > tab->selEndCursorPosition
           )
           || sd.cursorCBIndex != tab->cursorCBIndex
       )
    ) {
        tab->textSelected = false;
        tab->selStartCBIndex = 0;
        tab->selStartCursorPosition = 0;
        tab->selEndCBIndex = 0;
        tab->selEndCursorPosition = 0;
        tab->redrawSel = true;
    }

    tab->initialCursorPosition = sd.cursorPosition;
    updateCBCursor(CURRENT_TAB, sd);

    tab->cbTextMouseDown = true;
}

void contentBlockMouseUpCallback(UIElement* elem, float mouseX, float mouseY, glm::vec2 res) {
    ContentTab* tab = &tabs[activeTabIndex];

    SelectionData sd = getCBTextCursorFromScreenCoords(mouseX, mouseY, res);

    #ifdef DEBUG_TEXT
    cout << "\nDEBUG: contentBlockMouseUpCallback: " << "\n    selStartCBIndex: " << tab->selStartCBIndex
        << "\n    selEndCBIndex: " << tab->selEndCBIndex
        << "\n    selStartCursorPosition: " << tab->selStartCursorPosition
        << "\n    selEndCursorPosition: " << tab->selEndCursorPosition
        << "\n    cursorCBIndex: " << tab->cursorCBIndex
        << "\n    cursorPosition: " << tab->cursorPosition
        << "\n    textSelected: " << tab->textSelected
        << "\n    updateCursorX: " << tab->updateCursorX
        << "\n    redrawSel: " << tab->redrawSel
        << "\n    selVertexCount: " << tab->selVertexCount;
    #endif

    tab->cbTextMouseDown = false;

    if (sd.cursorPosition == tab->initialCursorPosition) {
        tab->textSelected = false;
        tab->selStartCursorPosition = 0;
        tab->selStartCBIndex = 0;
        tab->selEndCursorPosition = 0;
        tab->selEndCBIndex = 0;
    }
}

void renderCBMoveMarker() {
    #define GET_TOP(block) block->top - 1.5f - CONTENT_CONTAINER.scrollData->yOffset

    if (
        highlightedContentBlock.mouseDown
        && cbMove.show
    ) {
        if (cbMove.updated) {
            float top;

            if (cbMove.targetIndex == 0) {
                uint32_t index = tabs[activeTabIndex].contentBlocks.size() - 1;
                ContentBlock* last = &tabs[activeTabIndex].contentBlocks[index];

                top = GET_TOP(last) + last->height;
            } else {
                ContentBlock* last = &tabs[activeTabIndex].contentBlocks[cbMove.targetIndex];
                top = GET_TOP(last);
            }

            float bottom = top + 3.0f;

            float vertices[] = {
                contentRight, top,
                contentRight, bottom,
                contentLeft, top,
                contentLeft, bottom,
            };

            if (cbMove.vao) {
                glBindVertexArray(cbMove.vao);
                glBindBuffer(GL_ARRAY_BUFFER, cbMove.vbo);
                glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

                glBindVertexArray(0);
                glBindBuffer(GL_ARRAY_BUFFER, 0);
            } else {
                if (cbMove.vbo) {
                    glDeleteBuffers(1, &cbMove.vbo);
                    cbMove.vbo = 0;
                }

                SETUP_BUFFERS(cbMove.vao, cbMove.vbo, vertices);
            }

            cbMove.updated = false;
        }

        glBindVertexArray(cbMove.vao);
        glUseProgram(shaders[SHADER_DEFAULT].programId);
        glUniform4f(uniforms[U_DEFAULT_COLOR], 1.0f, 0.0f, 0.0f, 1.0f);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }

    #undef GET_TOP
}

void hideCBMoveMarker() {
    cbMove.show = false;
    glDeleteVertexArrays(1, &cbMove.vao);
    glDeleteBuffers(1, &cbMove.vbo);

    cbMove.vao = 0;
    cbMove.vbo = 0;
    cbMove.targetIndex = 0;
}

void reDrawAllContentBlocks() {
    for (
        auto itr = tabs[activeTabIndex].contentBlocks.begin();
        itr != tabs[activeTabIndex].contentBlocks.end();
        ++itr
    ) {
        (*itr).flags |= CONTENT_BLOCK_REDRAW;
    }
}

void reDrawCBsFrom(uint32_t i, uint32_t limit) {
    uint32_t j = 0;

    for (
        auto itr = tabs[activeTabIndex].contentBlocks.begin() + i;
        itr != tabs[activeTabIndex].contentBlocks.end() && j < limit;
        ++itr, ++j
    ) {
        (*itr).flags |= CONTENT_BLOCK_REDRAW | CONTENT_BLOCK_VISIBLE;
    }
}

void insertCBs(
    ContentTab* tab,
    uint32_t index,
    vector<ContentBlock>::iterator from,
    vector<ContentBlock>::iterator to,
    uint32_t count
) {
    tab->contentBlocks.insert(tab->contentBlocks.begin() + index, from, to);

    auto start = tab->contentBlocks.begin() + index;

    for (auto cbIter = start; cbIter != start + count; ++cbIter) {
        (*cbIter).rope = Rope::deepCopy((*cbIter).rope);
    }
}

void deleteCBs(
    ContentTab* tab,
    vector<ContentBlock>::iterator from,
    vector<ContentBlock>::iterator to
) {
    for (auto cbIter = from; cbIter != to; ++cbIter) {
        if ((*cbIter).textVAO != 0) {
            glDeleteVertexArrays(1, &(*cbIter).textVAO);
            glDeleteBuffers(1, &(*cbIter).textVBO);
        }

        delete (*cbIter).rope;
    }

    tab->contentBlocks.erase(from, to);
}

void deleteCBsToDeletedBlocks(
    ContentTab* tab,
    vector<ContentBlock>::iterator from,
    vector<ContentBlock>::iterator to,
    ContentCommandExt* extCmd
) {
    for (auto cbIter = from; cbIter != to; ++cbIter) {
        if ((*cbIter).textVAO != 0) {
            glDeleteVertexArrays(1, &(*cbIter).textVAO);
            glDeleteBuffers(1, &(*cbIter).textVBO);
        }

        delete (*cbIter).rope;

        extCmd->deletedBlocks.emplace_back(ContentBlock {
            (*cbIter).type,
            0, 0.0f, 0.0f, 0.0f, 0, 0, 0, 0, Rope::deepCopy((*cbIter).rope)
        });
    }

    tab->contentBlocks.erase(from, to);
}

void processRedo(UIElement* element) {
    #define CUR_CMD element->textInputData->cmdList.currentCommand
    #define CUR_POS element->textInputData->cursorPosition

    Command* cmd = CUR_CMD;

    if (cmd == nullptr || cmd->next == nullptr) {
        return;
    }

    moveToNextCommand(element->textInputData->cmdList);

    cmd = CUR_CMD;

    switch (cmd->type) {
        case CommandType::Type: {
            // cout << "\nCursor Position: " << elem->textInputData->cursorPosition;
            if (cmd->cursorPos == element->text.length()) {
                // cout << "\nAppending at the end! " << cmd->str.length();
                element->text += cmd->str;
                // elem->textInputData->cursorPosition += cmd->str.length();
                break;
            }

            element->text.insert(cmd->cursorPos, cmd->str);
            CUR_POS = cmd->cursorPos + 1;
            break;
        }

        case CommandType::Delete: {
            element->text.erase(cmd->cursorPos, 1);
            CUR_POS = cmd->cursorPos;
            break;
        }

        case CommandType::Backspace: {
            element->text.erase(cmd->cursorPos-1, 1);
            CUR_POS = cmd->cursorPos - 1;
            break;
        }

        case CommandType::Replace: {
            TextInputData* tid = element->textInputData;
            element->text.erase(tid->textSelectStartCol, cmd->replace.prevString.length());

            if (!cmd->replace.newString.empty()) {
                element->text.insert(tid->textSelectStartCol, cmd->replace.newString);
            }

            CUR_POS = cmd->cursorPos;
            break;
        }

        default:
            // Nothing!
            break;
    }

    #undef CUR_CMD
    #undef CUR_POS
}

void processUndo(UIElement* element) {
    #define CUR_POS element->textInputData->cursorPosition

    TextInputData* tid = element->textInputData;

    if (tid == nullptr) { return; }

    Command* currentCmd = tid->cmdList.currentCommand;

    if (currentCmd == nullptr) { return; }

    textInputUnselect(tid);

    switch (currentCmd->type) {
        case CommandType::Type: {
            element->text.erase(currentCmd->cursorPos, 1);
            moveToPrevCommand(tid->cmdList);
            CUR_POS = currentCmd->cursorPos;

            uint32_t newLines = 0;

            for (char c: currentCmd->str) {
                if (c == '\n') { ++newLines; }
            }

            if (newLines > 0) {
                element->height -= static_cast<float>(element->fontSize + LINE_GAP);
                element->reDraw = true;
            }
            break;
        }
        case CommandType::Backspace: {
            element->text.insert(currentCmd->cursorPos-1, currentCmd->str);
            moveToPrevCommand(tid->cmdList);
            CUR_POS = currentCmd->cursorPos;
            break;
        }
        case CommandType::Delete: {
            element->text.insert(currentCmd->cursorPos, currentCmd->str);
            moveToPrevCommand(tid->cmdList);
            CUR_POS = currentCmd->cursorPos;
            break;
        }

        case CommandType::Replace: {
            element->text.erase(currentCmd->replace.selectColStart, currentCmd->replace.newString.length());
            element->text.insert(currentCmd->replace.selectColStart, currentCmd->replace.prevString);
            moveToPrevCommand(tid->cmdList);
            CUR_POS = currentCmd->cursorPos;
            break;
        }

        default:
            // Nothing!
            break;
    }
    #undef CUR_POS
}

UIElement* getElementFromScreenCoords(UIElement * root, float x, float y) {
    #define IS_CURSOR_WITHIN_BOUNDS() (\
        x > elem->left && x < elem->left + elem->width\
        && y > elem->top && y < elem->top + elem->height\
    )

    // Check if the cursor is above the exact same position as it was when
    // mouse was down.
    // TODO: check instead if the mouse down element and mouse release
    // elements are the same.
    stack<UIElement*> elemStack;

    for (uint8_t i=0; i<root->totalChildren; ++i) {
        UIElement* e = root->children[(int)i];
        elemStack.push(e);
    }

    UIElement* elem = nullptr;
    UIElement* scrollElem = nullptr;

    while (!elemStack.empty()) {
        elem = elemStack.top();

        if (elem->show && IS_CURSOR_WITHIN_BOUNDS()) {
            if ((
                    elem->type == UIElementType::Button
                    || elem->type == UIElementType::TextInput
                    || elem->type == UIElementType::TextArea
                    || elem->type == UIElementType::ContentContainer
                    || elem->type == UIElementType::IconButton
                    || elem->type == UIElementType::TabBar
                    || elem->type == UIElementType::Explorer
                )
                && !(
                    elem->type == UIElementType::Button
                    && elem->parent->type == UIElementType::ScrollArea
                    && (
                        y > elem->parent->top + elem->parent->height
                        || y < elem->parent->top
                        || x > elem->parent->left + elem->parent->width
                        || x < elem->parent->left
                    )
                )
            ) {
                return elem;
            }

            if (
                elem->type == UIElementType::Container
                && elem->totalChildren
                && elem->children[0]->positioning == UIPositioning::Relative_sibling
            ) {
                float yPrev = elem->top;

                for (uint8_t i=0; i<elem->totalChildren; ++i) {
                    UIElement* e = elem->children[(int)i];

                    float top = yPrev;

                    if (i > 0) {
                        top += elem->children[i - 1]->height;
                    }

                    top += e->top;

                    if (
                        x > e->left && x < e->left + e->width
                        && y > top && y < top + e->height
                    ) {
                        return e;
                    }

                    yPrev = top;
                }
            }

            if (elem->type == UIElementType::ScrollArea) {
                scrollElem = elem;
            }
        }

        elemStack.pop();

        if (elem->totalChildren > 0) {
            for (uint8_t i=0; i<elem->totalChildren; ++i) {
                UIElement* e = elem->children[(int)i];

                if (e->show) {
                    elemStack.push(e);
                }
            }
        }
    }

    if (scrollElem != nullptr) { return scrollElem; }

    return nullptr;
    #undef IS_CURSOR_WITHIN_BOUNDS
}

void cbKeypressType(ContentTab* tab, string text) {
    if (tab->textSelected) {
        tab->extCommands.push_back(ContentCommandExt {
            ContentDeletionType::None,
            tab->selEndCBIndex,
            tab->selEndCursorPosition,
            vector<ContentBlock>(),
            vector<ContentBlock>(),
        });

        processCBCommand(ContentCommand {
            CommandType::Replace,
            true,
            static_cast<uint16_t>(tab->extCommands.size() - 1),
            tab->selStartCBIndex,
            tab->selStartCursorPosition,
            text,
        });

        tab->cursorPosition = tab->selStartCursorPosition + text.length();
        tab->textSelected = false;
    } else {
        processCBCommand(ContentCommand {
            CommandType::Type, false, 0, tab->cursorCBIndex, tab->cursorPosition, text
        });
        tab->cursorPosition += text.length();
    }

    tab->lastCursorTime = static_cast<float>(glfwGetTime());
    tab->redrawCursor = true;

    reDrawCBsFrom(tab->activeCBIndex, 64);
}

ContentDeletionType deleteSelectedCBText(
    ContentTab* tab, ContentCommand* cmd, ContentCommandExt* extCmd
) {
    #define START tab->contentBlocks.begin() + cmd->startCBIndex
    #define END   tab->contentBlocks.begin() + extCmd->endCBIndex

    if (cmd->startCBIndex == extCmd->endCBIndex) {
        ContentBlock* cbSel = &tab->contentBlocks[cmd->startCBIndex];
        vector<ContentBlock>::iterator itr = START;

        if (cmd->startCursorPos == 0 && extCmd->endCursorPos == cbSel->rope->length()) {
            extCmd->deletedBlocks.push_back(*itr);
            (*itr).rope = new Rope();
            return ContentDeletionType::DeleteSingleCB;
        }

        extCmd->deletedBlocks.push_back(ContentBlock {
            (*itr).type,
            0, 0.0f, 0.0f, 0.0f, 0, 0, 0, 0,
            new Rope((*itr).rope->getString().substr(
                cmd->startCursorPos,
                extCmd->endCursorPos - cmd->startCursorPos
            ))
        });

        (*itr).rope->remove(
            cmd->startCursorPos,
            extCmd->endCursorPos - cmd->startCursorPos
        );

        return ContentDeletionType::PartiallyDeleteSingleCB;
    }

    if (
        cmd->startCursorPos == 0
        && extCmd->endCursorPos == tab->contentBlocks[extCmd->endCBIndex].rope->length()
    ) {
        extCmd->deletedBlocks.assign(START, END + 1);
        tab->contentBlocks.erase(START, END + 1);
        tab->contentBlocks.insert(START, ContentBlock {
            ContentBlockType::Paragraph,
            CONTENT_BLOCK_REDRAW | CONTENT_BLOCK_VISIBLE,
            0.0f, 0.0f, 0.0f, 0, 0, 0, 0, new Rope()
        });
        return ContentDeletionType::DeleteMultipleCBs;
    }

    if (cmd->startCursorPos == 0) {
        auto lastCB = END;

        ContentBlockType lastBlockType = (*lastCB).type;
        string ropeString = (*lastCB).rope->getString().substr(0, extCmd->endCursorPos);

        extCmd->deletedBlocks.assign(START, lastCB);
        (*lastCB).rope->remove(0, extCmd->endCursorPos);
        tab->contentBlocks.erase(START, lastCB);

        extCmd->deletedBlocks.push_back(ContentBlock {
            lastBlockType,
            0, 0.0f, 0.0f, 0.0f, 0, 0, 0, 0,
            new Rope(ropeString),
        });
        return ContentDeletionType::DeleteMultipleCBsPartialLast;
    }

    if (
        extCmd->endCursorPos == tab->contentBlocks[extCmd->endCBIndex].rope->length()
    ) {
        auto firstCB = START;
        auto lastCB = END + 1;

        string ropeString = (*firstCB).rope->getString();
        extCmd->deletedBlocks.push_back(ContentBlock {
            (*firstCB).type,
            (*firstCB).flags,
            0.0f, 0.0f, 0.0f, 0, 0, 0, 0,
            new Rope(ropeString.substr(cmd->startCursorPos, ropeString.length() - cmd->startCursorPos))
        });
        (*firstCB).rope->remove(cmd->startCursorPos, ropeString.length() - cmd->startCursorPos);

        extCmd->deletedBlocks.insert(extCmd->deletedBlocks.begin() + 1, firstCB + 1, lastCB);

        tab->contentBlocks.erase(firstCB + 1, lastCB);

        return ContentDeletionType::DeleteMultipleCBsPartialFirst;
    }

    auto firstCB = START;
    auto lastCB = END;

    string firstRopeString = (*firstCB).rope->getString();
    string lastRopeString = (*lastCB).rope->getString();
    (*firstCB).rope->remove(cmd->startCursorPos, firstRopeString.length() - cmd->startCursorPos);
    (*firstCB).rope->append(lastRopeString.substr(extCmd->endCursorPos, lastRopeString.length() - extCmd->endCursorPos));

    extCmd->deletedBlocks.push_back(ContentBlock {
        (*firstCB).type,
        0, 0.0f, 0.0f, 0.0f, 0, 0, 0, 0,
        new Rope(firstRopeString.substr(cmd->startCursorPos, firstRopeString.length() - cmd->startCursorPos))
    });

    extCmd->deletedBlocks.insert(extCmd->deletedBlocks.begin() + 1, firstCB + 1, lastCB);

    extCmd->deletedBlocks.push_back(ContentBlock {
        (*lastCB).type,
        0, 0.0f, 0.0f, 0.0f, 0, 0, 0, 0,
        new Rope(lastRopeString.substr(0, extCmd->endCursorPos))
    });

    tab->contentBlocks.erase(firstCB + 1, lastCB + 1);
    return ContentDeletionType::DeleteMultipleCBsPartial;

    #undef START
    #undef END
}

vector<ContentBlock> textToCB(string& text) {
    string::iterator prevItr = text.begin();

    vector<ContentBlock> blocks;

    for(auto itr = text.begin(); itr != text.end(); ++itr) {
        if (*itr == '\n' || *itr == '\r') {
            string str = text.substr(distance(text.begin(), prevItr), distance(prevItr, itr));

            if (!str.empty()) {
                blocks.push_back(ContentBlock {
                    ContentBlockType::Paragraph,
                    0, 0.0f, 0.0f, 0.0f, 0, 0, 0, 0, new Rope(str)
                });
            }

            prevItr = itr + 1;

            if (*prevItr == '\n') {
                ++prevItr;
                ++itr;
            }
        }
    }

    if (prevItr != text.end()) {
        string str = text.substr(distance(text.begin(), prevItr), distance(prevItr, text.end()));

        if (!str.empty()) {
            blocks.push_back(ContentBlock {
                ContentBlockType::Paragraph,
                0, 0.0f, 0.0f, 0.0f, 0, 0, 0, 0, new Rope(str)
            });
        }
    }

    return blocks;
}

bool processCBTypeCommand(
    ContentTab* tab, ContentCommand* cmd, ContentBlock* cb, bool newCmd
) {
    if (cmd->str != "") {
        cb->rope->insert(cmd->startCursorPos, cmd->str);
        reDrawCBsFrom(cmd->startCBIndex, 64);

        if (
            newCmd && !tab->commands.empty()
            && tab->currentCommand == tab->commands.size() - 1
            && tab->commands.back().type == CommandType::Type
        ) {
            tab->commands.back().str += cmd->str;
            return false;
        }

        return true;
    }

    return false;
}

bool processCBPasteCommand(
    ContentTab* tab, ContentCommand* cmd, ContentBlock* cb, bool newCmd
) {
    if (!cmd->str.empty()) {
        uint32_t cursorCBIndex = tab->cursorCBIndex;

        vector<ContentBlock> pastedBlocks = textToCB(cmd->str);

        if (pastedBlocks.size() == 1) {
            cb->rope->insert(cmd->startCursorPos, cmd->str);
            tab->cursorPosition += cmd->str.length();
        } else {
            string firstString = cb->rope->getString();

            tab->extCommands.push_back(ContentCommandExt {
                ContentDeletionType::None,
                0, 0, pastedBlocks, vector<ContentBlock>()
            });

            cmd->ext = true;
            cmd->extIndex = tab->extCommands.size() - 1;

            cb->rope->remove(cmd->startCursorPos, firstString.length() - cmd->startCursorPos);
            cb->rope->append(pastedBlocks.front().rope->getString());

            tab->contentBlocks.insert(
                tab->contentBlocks.begin() + tab->cursorCBIndex + 1,
                pastedBlocks.begin() + 1,
                pastedBlocks.end()
            );

            tab->contentBlocks[tab->cursorCBIndex + pastedBlocks.size() - 1].rope = new Rope(
                pastedBlocks.back().rope->getString(),
                firstString.substr(
                    cmd->startCursorPos,
                    firstString.length() - cmd->startCursorPos
                ));

            tab->cursorCBIndex += pastedBlocks.size() - 1;
            tab->cursorPosition = pastedBlocks.back().rope->length();
        }

        reDrawCBsFrom(cursorCBIndex, 64);
    }

    return true;
}

bool processCBReplaceCommand(
    ContentTab* tab, ContentCommand* cmd, ContentBlock* cb, bool newCmd
) {
    ContentCommandExt* extCmd = &*(tab->extCommands.begin() + cmd->extIndex);

    #define START tab->contentBlocks.begin() + cmd->startCBIndex
    #define END   tab->contentBlocks.begin() + extCmd->endCBIndex

    vector<ContentBlock> pastedCBs = textToCB(cmd->str);

    if (pastedCBs.empty()) {
        tab->extCommands.pop_back();
        return false;
    }

    extCmd->insertedBlocks.assign(pastedCBs.begin(), pastedCBs.end());

    extCmd->delType = deleteSelectedCBText(tab, cmd, extCmd);

    switch (extCmd->delType) {
        case ContentDeletionType::DeleteMultipleCBs:
        case ContentDeletionType::DeleteSingleCB:
            // tab->contentBlocks.erase(START);

            deleteCBs(tab, START, START + 1);

            // tab->contentBlocks.insert(START, pastedCBs.begin(), pastedCBs.end());

            insertCBs(
                tab,
                cmd->startCBIndex,
                pastedCBs.begin(),
                pastedCBs.end(),
                pastedCBs.size()
            );

            tab->cursorPosition = pastedCBs.back().rope->length();

            if (pastedCBs.size() > 1) {
                tab->cursorCBIndex = cmd->startCBIndex + pastedCBs.size() - 1;
            }
            break;

        case ContentDeletionType::PartiallyDeleteSingleCB: {
            auto firstCB = START;
            string firstCBStr = (*firstCB).rope->getString();

            string pastedFirstStr = pastedCBs.front().rope->getString();

            (*firstCB).rope->insert(cmd->startCursorPos, pastedFirstStr);

            if (pastedCBs.size() > 1) {
                uint32_t firstCBPastedEnd = cmd->startCursorPos + pastedFirstStr.length();

                (*firstCB).rope->remove(
                    firstCBPastedEnd,
                    (*firstCB).rope->length() - firstCBPastedEnd
                );

                // tab->contentBlocks.insert(START + 1, pastedCBs.begin() + 1, pastedCBs.end());

                insertCBs(
                    tab,
                    cmd->startCBIndex + 1,
                    pastedCBs.begin() + 1,
                    pastedCBs.end(),
                    pastedCBs.size() - 1
                );

                auto lastPastedCB = START + pastedCBs.size() - 1;

                string lastPastedStr = (*lastPastedCB).rope->getString();

                (*lastPastedCB).rope = new Rope(
                    lastPastedStr,
                    firstCBStr.substr(
                        cmd->startCursorPos,
                        firstCBStr.length() - cmd->startCursorPos
                    )
                );

                tab->cursorPosition = lastPastedStr.length();
                tab->cursorCBIndex = cmd->startCBIndex + pastedCBs.size() - 1;
            } else {
                tab->cursorPosition += pastedFirstStr.length();
            }

            break;
        }

        case ContentDeletionType::DeleteMultipleCBsPartial: {
            // tab->contentBlocks.insert(START + 1, pastedCBs.begin(), pastedCBs.end());

            insertCBs(
                tab,
                cmd->startCBIndex + 1,
                pastedCBs.begin(),
                pastedCBs.end(),
                pastedCBs.size() - 1
            );

            string firstCBString = (*(START)).rope->getString();

            (*(START)).rope->remove(cmd->startCursorPos, firstCBString.length() - cmd->startCursorPos);
            (*(START)).rope->append(pastedCBs.front().rope->getString());
            string lastCBRope = (*(START + pastedCBs.size())).rope->getString();

            (*(START + pastedCBs.size())).rope = new Rope(
                lastCBRope,
                firstCBString.substr(cmd->startCursorPos, firstCBString.length() - cmd->startCursorPos)
            );

            tab->cursorPosition = pastedCBs.back().rope->length();

            if (pastedCBs.size() > 1) {
                tab->cursorCBIndex = cmd->startCBIndex + pastedCBs.size();
            }
            break;
        }

        case ContentDeletionType::DeleteMultipleCBsPartialFirst: {
            auto firstCB = START;

            string pastedFirstStr = pastedCBs.front().rope->getString();

            (*firstCB).rope->append(pastedFirstStr);

            if (pastedCBs.size() > 1) {
                // tab->contentBlocks.insert(START + 1, pastedCBs.begin() + 1, pastedCBs.end());

                insertCBs(
                    tab,
                    cmd->startCBIndex + 1,
                    pastedCBs.begin() + 1,
                    pastedCBs.end(),
                    pastedCBs.size() - 1
                );
            }

            tab->cursorPosition = pastedCBs.back().rope->length();

            if (pastedCBs.size() > 1) {
                tab->cursorCBIndex = cmd->startCBIndex + pastedCBs.size() - 1;
            }
            break;
        }

        case ContentDeletionType::DeleteMultipleCBsPartialLast: {
            // tab->contentBlocks.insert(START, pastedCBs.begin(), pastedCBs.end() - 1);

            insertCBs(
                tab,
                cmd->startCBIndex,
                pastedCBs.begin(),
                pastedCBs.end() - 1,
                pastedCBs.size() - 1
            );

            auto lastPastedCB = START + pastedCBs.size() - 1;

            (*lastPastedCB).rope->prepend(pastedCBs.back().rope->getString());

            tab->cursorPosition = pastedCBs.back().rope->length();

            if (pastedCBs.size() > 1) {
                tab->cursorCBIndex = cmd->startCBIndex + pastedCBs.size() - 1;
            }
            break;
        }
    }

    reDrawCBsFrom(cmd->startCBIndex, 64);

    #undef START
    #undef END

    return true;
}

bool processCBNewLineCommand(
    ContentTab* tab, ContentCommand* cmd, ContentBlock* cb, bool newCmd
) {
    if (cmd->startCursorPos == cb->rope->length() || cmd->startCursorPos == 0) {
        if (cmd->startCursorPos != 0) {
            ++tab->cursorCBIndex;
        }

        tab->contentBlocks.insert(
            tab->contentBlocks.begin() + tab->cursorCBIndex,
            ContentBlock {
                cb->type,
                CONTENT_BLOCK_REDRAW | CONTENT_BLOCK_VISIBLE,
                0.0f, 0.0f, 0.0f,
                0, 0, 0, 0,
                new Rope()
            }
        );
    } else if (cb->rope != nullptr) {
        Rope* result = cb->rope->split(cmd->startCursorPos);

        if (result != nullptr) {
            Rope* rope = result->right;

            if (rope != nullptr) {
                ++tab->cursorCBIndex;

                tab->contentBlocks.insert(
                    tab->contentBlocks.begin() + tab->cursorCBIndex,
                    ContentBlock {
                        cb->type,
                        CONTENT_BLOCK_REDRAW | CONTENT_BLOCK_VISIBLE,
                        0.0f, 0.0f, 0.0f,
                        0, 0, 0, 0,
                        rope
                });

                if (tab->cursorCBIndex == 0) {
                    (*tab->contentBlocks.begin()).rope->pick(rope);
                } else {
                    (*(tab->contentBlocks.begin() + tab->cursorCBIndex - 1)).rope->pick(rope);
                }
            }
        }
    } else {
        tab->contentBlocks.insert(
            tab->contentBlocks.begin() + tab->cursorCBIndex,
            ContentBlock {
                cb->type,
                CONTENT_BLOCK_REDRAW | CONTENT_BLOCK_VISIBLE,
                0.0f, 0.0f, 0.0f,
                0, 0, 0, 0,
                nullptr
            });
    }

    tab->cursorPosition = 0;

    reDrawCBsFrom(cmd->startCBIndex, 64);

    return true;
}

bool processCBBackspaceCommand(
    ContentTab* tab, ContentCommand* cmd, ContentBlock* cb, bool newCmd
) {
    if (cmd->startCursorPos > 0) {
        cb->rope->remove(cmd->startCursorPos - 1, cmd->str.length());

        if (
            newCmd && !tab->commands.empty()
            && tab->currentCommand == tab->commands.size() - 1
            && tab->commands.back().type == CommandType::Backspace
            && tab->commands.back().startCursorPos == cmd->startCursorPos + 1
        ) {
            tab->commands.back().str = cmd->str + tab->commands.back().str;
            --tab->commands.back().startCursorPos;
            reDrawCBsFrom(cmd->startCBIndex, 64);
            return false;
        }

        reDrawCBsFrom(cmd->startCBIndex, 64);
        return true;
    }

    return false;
}

bool processCBDeleteCommand(
    ContentTab* tab, ContentCommand* cmd, ContentBlock* cb, bool newCmd
) {
    if (cmd->startCursorPos < cb->rope->length()) {
        cb->rope->remove(cmd->startCursorPos, cmd->str.length());

        if (
            newCmd && !tab->commands.empty()
            && tab->currentCommand == tab->commands.size() - 1
            && tab->commands.back().type == CommandType::Delete
            && tab->commands.back().startCursorPos == cmd->startCursorPos
        ) {
            tab->commands.back().str += cmd->str;
            return false;
        }

        return true;
    }

    return false;
}

bool processCBDeleteMergeCBCommand(
    ContentTab* tab, ContentCommand* cmd, ContentBlock* cb, bool newCmd
) {
    if (
        cmd->ext
        && cmd->extIndex < tab->extCommands.size()
        && !tab->extCommands[cmd->extIndex].deletedBlocks.empty()
        && cmd->startCBIndex != 0
        && cmd->startCBIndex + 1 < tab->contentBlocks.size()
    ) {
        Rope* nextRope = tab->contentBlocks[cmd->startCBIndex + 1].rope;

        tab->contentBlocks[cmd->startCBIndex].rope->append(nextRope->getString());

        tab->contentBlocks.erase(
            tab->contentBlocks.begin() + cmd->startCBIndex + 1,
            tab->contentBlocks.begin() + cmd->startCBIndex + 2
        );

        for (
            auto cbIter = tab->contentBlocks.begin() + cmd->startCBIndex;
            cbIter != tab->contentBlocks.end() && distance(tab->contentBlocks.begin(), cbIter) < 64;
            ++cbIter
        ) {
            (*cbIter).flags |= CONTENT_BLOCK_REDRAW;
        }

        return true;
    }

    return false;
}

bool processCBDeleteSelectedCommand(
    ContentTab* tab, ContentCommand* cmd, ContentBlock* cb, bool newCmd
) {
    auto extCmd = tab->extCommands.rbegin();
    (*extCmd).delType = deleteSelectedCBText(tab, cmd, &*extCmd);
    reDrawCBsFrom(cmd->startCBIndex, 64);
    return true;
}

void processCBCommand(ContentCommand cmd) {
    ContentTab* tab = &tabs[activeTabIndex];
    ContentBlock* cb = &tab->contentBlocks[cmd.startCBIndex];

    #define CONCAT(a, b, c) a ## b ## c
    #define PROC(cmdType) CONCAT(processCB, cmdType, Command)(tab, &cmd, cb, true);

    #ifdef DEBUG_TEXT_INPUT
        cout << "\nProcessing command \"" << to_string(cmd.type)
            << "\"\n  -> str: \"" << cmd.str << "\", startCBIndex: "
            << cmd.startCBIndex << ", startCursorPos: " << cmd.startCursorPos;

        if (cmd.ext) {
            cout << ", endCBIndex: " << tab->extCommands[cmd.extIndex].endCBIndex
                << ", endCursorPos: " << tab->extCommands[cmd.extIndex].endCursorPos;
        }
    #endif

    bool append = false;

    switch (cmd.type) {
        case CommandType::Type: append = PROC(Type); break;
        case CommandType::Paste: append = PROC(Paste); break;
        case CommandType::Replace: append = PROC(Replace); break;
        case CommandType::NewLine: append = PROC(NewLine); break;
        case CommandType::Backspace: append = PROC(Backspace); break;
        case CommandType::Delete: append = PROC(Delete); break;
        case CommandType::DeleteMergeCB: append = PROC(DeleteMergeCB); break;
        case CommandType::DeleteSelected: append = PROC(DeleteSelected); break;
        case CommandType::Undo: processCBUndo(tab); break;
        case CommandType::Redo: processCBRedo(tab); break;
    }

    if (append) {
        if (tab->currentCommand < tab->commands.size() - 1) {
            auto cmdIter = tab->commands.end() - 1;

            while (cmdIter != tab->commands.begin() + tab->currentCommand) {
                if ((*cmdIter).ext && (*cmdIter).extIndex == tab->extCommands.size() - 1) {
                    tab->extCommands.pop_back();
                }

                #ifdef DEBUG_COMMANDS
                    cout << "\nDEBUG: Deleting command: ";
                    printContentCommand(&*cmdIter);
                #endif

                tab->commands.pop_back();
                cmdIter = tab->commands.end() - 1;
            }
        }

        ++tab->currentCommand;
        tab->commands.push_back(cmd);
    }

    #ifdef DEBUG_COMMANDS
        cout << "\nDEBUG: Commands:";

        for (
            auto cmd = tab->commands.begin();
            cmd != tab->commands.end();
            ++cmd
        ) {
            if (distance(tab->commands.begin(), cmd) == tab->currentCommand) {
                cout << "\n  -> ";
            } else {
                cout << "\n     ";
            }

            if ((*cmd).ext) {
                printContentCommand(&*cmd, &tab->extCommands[(*cmd).extIndex]);
                #ifdef DEBUG_CONTENT_BLOCKS
                    printContentBlocksFromVector(tab->extCommands[(*cmd).extIndex].deletedBlocks);
                    if (!tab->extCommands[(*cmd).extIndex].insertedBlocks.empty()) {
                        cout << "\n     |-> Inserted Content Blocks:";
                        printContentBlocksFromVector(tab->extCommands[(*cmd).extIndex].insertedBlocks);
                    }
                #endif
            } else {
                printContentCommand(&*cmd);
            }
        }
    #endif

    #undef CONCAT
    #undef PROC
}

void processCBUndo(ContentTab* tab) {
    #define SET_CURSOR_POS(c) tab->cursorPosition = c; tab->redrawCursor = true
    #define CB_ROPE(cbIndex) tab->contentBlocks[cbIndex].rope

    if (tab->currentCommand == 0) { return; }

    #ifdef DEBUG_COMMANDS
        cout << "\nDEBUG: undoing command";
    #endif

    ContentCommand* cmd = &tab->commands[tab->currentCommand];
    ContentCommandExt* extCmd;

    if (cmd->ext) {
        extCmd = &tab->extCommands[cmd->extIndex];
    } else {
        extCmd = nullptr;
    }

    #define START tab->contentBlocks.begin() + cmd->startCBIndex
    #define END   tab->contentBlocks.begin() + extCmd->endCBIndex

    if (cmd == nullptr) { return; }

    bool decrement = false;

    switch (cmd->type) {
        case CommandType::Type: {
            CB_ROPE(cmd->startCBIndex)->remove(cmd->startCursorPos, cmd->str.length());
            SET_CURSOR_POS(cmd->startCursorPos);
            decrement = true;
            break;
        }

        case CommandType::Paste: {
            if (cmd->ext) {
                (*(START)).rope->remove(
                    cmd->startCursorPos,
                    (*(START)).rope->length() - cmd->startCursorPos
                );

                uint32_t lastCBRopeLen = extCmd->insertedBlocks.back().rope->length();

                string secondString = (*(START + extCmd->insertedBlocks.size() - 1)).rope->getString();
                delete (*(START + extCmd->insertedBlocks.size() - 1)).rope;

                deleteCBs(tab, START + 1, START + extCmd->insertedBlocks.size());
                // tab->contentBlocks.erase(START + 1, START + extCmd->insertedBlocks.size());

                (*(START)).rope->append(secondString.substr(lastCBRopeLen, secondString.length() - lastCBRopeLen));
            } else {
                (*(START)).rope->remove(cmd->startCursorPos, cmd->str.length());
            }

            tab->cursorCBIndex = cmd->startCBIndex;
            tab->cursorPosition = cmd->startCursorPos;
            tab->redrawCursor = true;

            decrement = true;
            break;
        }

        case CommandType::Replace: {
            switch (extCmd->delType) {
                case ContentDeletionType::DeleteSingleCB:
                    if (extCmd->insertedBlocks.size() > 1) {
                        tab->contentBlocks.erase(START + 1, START + extCmd->insertedBlocks.size());
                    }

                    delete (*(START)).rope;
                    (*(START)).rope = new Rope(extCmd->deletedBlocks.back().rope->getString());
                    break;

                case ContentDeletionType::PartiallyDeleteSingleCB:
                    if (!extCmd->deletedBlocks.empty()) {
                        auto firstCB = START;
                        string lastCBStr = "";

                        if (extCmd->insertedBlocks.size() > 1) {
                            string lastStr = (*(firstCB + extCmd->insertedBlocks.size() - 1)).rope->getString();

                            uint32_t len = extCmd->insertedBlocks.back().rope->length();

                            lastCBStr = lastStr.substr(len, lastStr.length() - len);

                            (*(firstCB + extCmd->insertedBlocks.size() - 1)).rope->remove(
                                0,
                                extCmd->insertedBlocks.back().rope->length()
                            );

                            deleteCBs(tab, START + 1, START + extCmd->insertedBlocks.size());
                            // tab->contentBlocks.erase(START + 1, START + extCmd->insertedBlocks.size());
                        }

                        (*firstCB).rope->remove(
                            cmd->startCursorPos,
                            extCmd->insertedBlocks.front().rope->length()
                        );

                        (*firstCB).rope->insert(cmd->startCursorPos, extCmd->deletedBlocks.back().rope->getString());
                        (*firstCB).rope->append(lastCBStr);
                    } else {
                        cout << "\nError: deleted blocks is empty";
                    }
                    break;

                case ContentDeletionType::DeleteMultipleCBs:
                    deleteCBs(tab, START, START + extCmd->insertedBlocks.size());

                    // tab->contentBlocks.erase(
                    //     START,
                    //     START + extCmd->insertedBlocks.size()
                    // );

                    insertCBs(
                        tab,
                        cmd->startCBIndex,
                        extCmd->deletedBlocks.begin(),
                        extCmd->deletedBlocks.end(),
                        extCmd->deletedBlocks.size()
                    );

                    // tab->contentBlocks.insert(
                    //     START,
                    //     extCmd->deletedBlocks.begin(),
                    //     extCmd->deletedBlocks.end()
                    // );
                    break;

                case ContentDeletionType::DeleteMultipleCBsPartialLast:
                    deleteCBs(tab, START, START + extCmd->insertedBlocks.size() - 1);

                    // tab->contentBlocks.erase(
                    //     START,
                    //     START + extCmd->insertedBlocks.size() - 1
                    // );

                    insertCBs(
                        tab,
                        cmd->startCBIndex,
                        extCmd->deletedBlocks.begin(),
                        extCmd->deletedBlocks.end() - 1,
                        extCmd->deletedBlocks.size() - 1
                    );

                    // tab->contentBlocks.insert(
                    //     START,
                    //     extCmd->deletedBlocks.begin(),
                    //     extCmd->deletedBlocks.end() - 1
                    // );

                    CB_ROPE(extCmd->endCBIndex)->remove(
                        0,
                        extCmd->insertedBlocks.back().rope->length()
                    );

                    CB_ROPE(extCmd->endCBIndex)->prepend(extCmd->deletedBlocks.back().rope->getString());
                    break;

                case ContentDeletionType::DeleteMultipleCBsPartialFirst:
                    deleteCBs(tab, START + 1, START + extCmd->insertedBlocks.size());

                    // tab->contentBlocks.erase(START + 1, START + extCmd->insertedBlocks.size());

                    CB_ROPE(cmd->startCBIndex)->remove(
                        cmd->startCursorPos,
                        CB_ROPE(cmd->startCBIndex)->length() - cmd->startCursorPos
                    );

                    CB_ROPE(cmd->startCBIndex)->append(
                        extCmd->deletedBlocks.front().rope->getString()
                    );

                    // tab->contentBlocks.insert(
                    //     START + 1,
                    //     extCmd->deletedBlocks.begin() + 1,
                    //     extCmd->deletedBlocks.end()
                    // );

                    insertCBs(
                        tab,
                        cmd->startCBIndex + 1,
                        extCmd->deletedBlocks.begin() + 1,
                        extCmd->deletedBlocks.end(),
                        extCmd->deletedBlocks.size()
                    );

                    break;

                case ContentDeletionType::DeleteMultipleCBsPartial:
                    auto firstCB = START;

                    string firstRopeString = (*firstCB).rope->getString();

                    (*firstCB).rope->remove(cmd->startCursorPos, firstRopeString.length() - cmd->startCursorPos);
                    (*firstCB).rope->append(extCmd->deletedBlocks.front().rope->getString());

                    // tab->contentBlocks.erase(
                    //     START + 1,
                    //     START + extCmd->insertedBlocks.size()
                    // );

                    deleteCBs(tab, START + 1, START + extCmd->insertedBlocks.size());

                    // tab->contentBlocks.insert(
                    //     START + 1,
                    //     extCmd->deletedBlocks.begin() + 1,
                    //     extCmd->deletedBlocks.end() - 1
                    // );

                    insertCBs(
                        tab,
                        cmd->startCBIndex + 1,
                        extCmd->deletedBlocks.begin() + 1,
                        extCmd->deletedBlocks.end() - 1,
                        extCmd->deletedBlocks.size() - 2
                    );

                    cout << "\nDEBUG: End rope is: \"" << (*(END)).rope->getString() << "\"";
                    cout << "\nDEBUG: Last pasted rope is: \"" << extCmd->insertedBlocks.back().rope->getString() << "\"";
                    cout << "\nDEBUG: End rope type is: " << to_string((*(END)).type);

                    string lastString = (*(END)).rope->getString();
                    delete (*(END)).rope;

                    (*(END)).rope = extCmd->deletedBlocks.back().rope;

                    uint32_t insertedRopeLength = extCmd->insertedBlocks.back().rope->length();

                    (*(END)).rope->append(lastString.substr(insertedRopeLength, lastString.length() - insertedRopeLength));
                    (*(END)).type = extCmd->deletedBlocks.back().type;
                    break;
            }

            tab->cursorCBIndex = cmd->startCBIndex;
            tab->cursorPosition = cmd->startCursorPos;
            tab->redrawCursor = true;
            decrement = true;

            break;
        }

        case CommandType::NewLine: {
            if (cmd->startCursorPos == 0) {
                auto cbIter = tab->contentBlocks.begin() + cmd->startCBIndex;

                delete (*cbIter).rope;

                // tab->contentBlocks.erase(cbIter);
                deleteCBs(tab, cbIter, cbIter + 1);
            } else {
                auto cbIter = tab->contentBlocks.begin() + cmd->startCBIndex + 1;
                string newLineString = (*cbIter).rope->getString();

                delete (*cbIter).rope;

                // tab->contentBlocks.erase(cbIter);
                deleteCBs(tab, cbIter, cbIter + 1);
                CB_ROPE(cmd->startCBIndex)->append(newLineString);
            }

            tab->cursorPosition = cmd->startCursorPos;
            tab->redrawCursor = true;
            tab->cursorCBIndex = cmd->startCBIndex;
            decrement = true;
            break;
        }

        case CommandType::Backspace: {
            CB_ROPE(cmd->startCBIndex)->insert(cmd->startCursorPos - 1, cmd->str);
            SET_CURSOR_POS(cmd->startCursorPos + cmd->str.length() - 1);
            decrement = true;
            break;
        }

        case CommandType::Delete: {
            CB_ROPE(cmd->startCBIndex)->insert(cmd->startCursorPos, cmd->str);
            SET_CURSOR_POS(cmd->startCursorPos);
            decrement = true;
            break;
        }

        case CommandType::DeleteSelected: {
            ContentCommandExt* extCmd = &tab->extCommands[cmd->extIndex];

            switch (extCmd->delType) {
                case ContentDeletionType::DeleteSingleCB:
                    delete (*(START)).rope;
                    (*(START)).rope = Rope::deepCopy(extCmd->deletedBlocks.back().rope);
                    break;

                case ContentDeletionType::PartiallyDeleteSingleCB:
                    if (!extCmd->deletedBlocks.empty()) {
                        (*(START)).rope->insert(cmd->startCursorPos, extCmd->deletedBlocks.back().rope->getString());
                    } else {
                        cout << "\nError: deleted blocks is empty";
                    }
                    break;

                case ContentDeletionType::DeleteMultipleCBs:
                    // delete (*(START)).rope;

                    // tab->contentBlocks.erase(START);
                    deleteCBs(tab, START, START + 1);

                    // tab->contentBlocks.insert(
                    //     START,
                    //     extCmd->deletedBlocks.begin(),
                    //     extCmd->deletedBlocks.end()
                    // );

                    insertCBs(
                        tab,
                        cmd->startCBIndex,
                        extCmd->deletedBlocks.begin(),
                        extCmd->deletedBlocks.end(),
                        extCmd->deletedBlocks.size()
                    );

                    break;

                case ContentDeletionType::DeleteMultipleCBsPartialLast:
                    // tab->contentBlocks.insert(
                    //     START,
                    //     extCmd->deletedBlocks.begin(),
                    //     extCmd->deletedBlocks.end() - 1
                    // );

                    insertCBs(
                        tab,
                        cmd->startCBIndex,
                        extCmd->deletedBlocks.begin(),
                        extCmd->deletedBlocks.end() - 1,
                        extCmd->deletedBlocks.size() - 1
                    );

                    // for (
                    //     auto cbIter = START;
                    //     cbIter != START + extCmd->deletedBlocks.size() - 1;
                    //     ++cbIter
                    // ) {
                    //     (*cbIter).rope = Rope::deepCopy((*cbIter).rope);
                    // }

                    tab->contentBlocks[extCmd->endCBIndex].rope->prepend(extCmd->deletedBlocks.back().rope->getString());
                    break;

                case ContentDeletionType::DeleteMultipleCBsPartialFirst:
                    tab->contentBlocks[cmd->startCBIndex].rope->remove(
                        cmd->startCursorPos,
                        tab->contentBlocks[cmd->startCBIndex].rope->length() - cmd->startCursorPos
                    );

                    tab->contentBlocks[cmd->startCBIndex].rope->append(
                        extCmd->deletedBlocks.front().rope->getString()
                    );

                    // tab->contentBlocks.insert(
                    //     START + 1,
                    //     extCmd->deletedBlocks.begin() + 1,
                    //     extCmd->deletedBlocks.end()
                    // );

                    insertCBs(
                        tab,
                        cmd->startCBIndex + 1,
                        extCmd->deletedBlocks.begin() + 1,
                        extCmd->deletedBlocks.end(),
                        extCmd->deletedBlocks.size()
                    );

                    break;
                case ContentDeletionType::DeleteMultipleCBsPartial:
                    auto firstCB = START;

                    string firstRopeString = (*firstCB).rope->getString();

                    (*firstCB).rope->remove(cmd->startCursorPos, firstRopeString.length() - cmd->startCursorPos);
                    (*firstCB).rope->append(extCmd->deletedBlocks.front().rope->getString());

                    tab->contentBlocks.insert(
                        START + 1,
                        extCmd->deletedBlocks.begin() + 1,
                        extCmd->deletedBlocks.end()
                    );

                    insertCBs(
                        tab,
                        cmd->startCBIndex + 1,
                        extCmd->deletedBlocks.begin() + 1,
                        extCmd->deletedBlocks.end(),
                        extCmd->deletedBlocks.size()
                    );

                    (*(END)).rope->append(firstRopeString.substr(
                        cmd->startCursorPos,
                        firstRopeString.length() - cmd->startCursorPos
                    ));
                    break;
            }

            tab->redrawCursor = true;
            decrement = true;

            break;
        }
    }

    if (decrement) {
        --tab->currentCommand;
    }

    reDrawCBsFrom(cmd->startCBIndex, 64);

    #undef START
    #undef END
    #undef SET_CURSOR_POS
    #undef CB_ROPE
}

void processCBRedo(ContentTab* tab) {
    #define CONCAT(a, b, c) a ## b ## c
    #define PROC(cmdType) CONCAT(processCB, cmdType, Command)(tab, cmd, cb, false);

    if (tab->currentCommand == tab->commands.size() - 1) { return; }

    ContentCommand* cmd = &tab->commands[tab->currentCommand + 1];
    ContentBlock* cb = &tab->contentBlocks[cmd->startCBIndex];

    bool increment = false;

    switch (cmd->type) {
        case CommandType::Type: {
            increment = processCBTypeCommand(tab, cmd, cb, false);
            tab->cursorPosition = cmd->startCursorPos + cmd->str.length();
            tab->redrawCursor = true;
            break;
        }

        case CommandType::Paste: increment = PROC(Paste); break;
        case CommandType::Replace: increment = PROC(Replace); break;
        case CommandType::NewLine: increment = PROC(NewLine); break;
        case CommandType::Backspace: increment = PROC(Backspace); break;
        case CommandType::Delete: increment = PROC(Delete); break;
        case CommandType::DeleteMergeCB: increment = PROC(DeleteMergeCB); break;
        case CommandType::DeleteSelected: increment = PROC(DeleteSelected); break;
    }

    if (increment) {
        ++tab->currentCommand;
    }

    #undef CONCAT
    #undef PROC
}

void processCommand(UIElement* elem, Command* cmd) {
    #define CUR_POS elem->textInputData->cursorPosition
    if (elem->type == UIElementType::TextInput || elem->type == UIElementType::TextArea) {
        bool append = false;

        TextInputData * tid = elem->textInputData;

        switch(cmd->type) {
            case CommandType::None: break;
            case CommandType::Type: {
                if (tid->textSelected) {
                    cmd = replaceSelectedText(elem, cmd);
                } else {
                    if (CUR_POS == elem->text.length()) {
                        elem->text += cmd->str;
                        append = true;

                        if (elem->type == UIElementType::TextArea) {
                            uint32_t newLines = 0;

                            for (char c: cmd->str) {
                                if (c == '\n') { ++newLines; }
                            }

                            if (newLines > 0) {
                                elem->height += static_cast<float>(elem->fontSize + LINE_GAP);
                            }
                        }

                        elem->reDraw = true;

                        break;
                    }

                    elem->text.insert(CUR_POS, cmd->str);
                }

                elem->reDraw = true;
                append = true;
                break;
            }

            case CommandType::NewLine: {
                break;
            }

            case CommandType::Delete: {
                uint32_t newLines = 0;

                for (char c: cmd->str) {
                    if (c == '\n') { ++newLines; }
                }

                if (tid->textSelected) {
                    cmd = replaceSelectedText(elem, cmd);
                } else {
                    elem->text.erase(CUR_POS, 1);
                }


                if (newLines > 0 && elem->height > 32.0) {
                    elem->height -= static_cast<float>(elem->fontSize + LINE_GAP);
                    elem->reDraw = true;
                }
                append = true;
                break;
            }

            case CommandType::Backspace: {
                uint32_t newLines = 0;

                for (char c: cmd->str) {
                    if (c == '\n') { ++newLines; }
                }

                if (tid->textSelected) {
                    if (tid->textSelectStartCol == 0 && tid->textSelectEndCol == elem->text.length()) {
                        elem->text = "";
                        CUR_POS = 0;
                    } else {
                        cmd = replaceSelectedText(elem, cmd);
                    }
                } else {
                    elem->text.erase(CUR_POS-1, 1);
                }

                append = true;

                if (newLines > 0 && elem->height > 32.0) {
                    elem->height -= static_cast<float>(elem->fontSize + LINE_GAP);
                }

                elem->reDraw = true;
                break;
            }

            case CommandType::Undo:
                processUndo(elem);
                break;

            case CommandType::Redo:
                processRedo(elem);
                break;

            default:
                // Nothing!
                break;
        }

        if (append) {
            appendCommand(elem->textInputData->cmdList, cmd);
        }

        #ifdef DEBUG_COMMANDS
            printCommands(elem->textInputData->cmdList);
        #endif
    }
    #undef CUR_POS
}

string getIconLocation(Icon icon) {
    string path = "assets/images/";

    switch (icon) {
        case Icon::None: return "";
        case Icon::Plus: path += "plus.png"; break;
        case Icon::File: path += "file.png"; break;
    }

    return path;
}

bool isPointInside(float pointX, float pointY, float left, float right, float top, float bottom) {
    return pointX > left && pointX < right && pointY > top && pointY < bottom;
}

void getScrollbarDimensions(
    UIElement* elem,
    float& top, float& right, float& bottom, float& left,
    float& thumbStart, float& thumbEnd,
    bool vertical
) {
    if (vertical) {
        float height = elem->height;

        if (elem->scrollData->showHorizontalScrollbar) {
            height -= SCROLL_BAR_THICKNESS;
        }

        top = elem->top;
        right = elem->left + elem->width;
        left = right - SCROLL_BAR_THICKNESS;
        bottom = top + height;

        thumbStart = top + elem->scrollData->yOffset / elem->scrollData->height * height;
        thumbEnd = thumbStart + elem->height / elem->scrollData->height * height;

        if (thumbEnd > top + height) {
            float barWidth = thumbEnd - thumbStart;

            thumbEnd = top + height;
            thumbStart = thumbEnd - barWidth;
        }

        return;
    }

    float width = elem->width;

    if (elem->scrollData->showVerticalScrollbar) {
        width -= SCROLL_BAR_THICKNESS;
    }

    bottom = elem->top + elem->height;
    top = bottom - SCROLL_BAR_THICKNESS;
    right = elem->left + width;
    left = elem->left;

    thumbStart = left + elem->scrollData->xOffset / elem->scrollData->width * width;
    thumbEnd = thumbStart + elem->width / elem->scrollData->width * width;

    if (thumbEnd > elem->left + width) {
        float barWidth = thumbEnd - thumbStart;

        thumbEnd = left + width;
        thumbStart = thumbEnd - barWidth;
    }
}

void getCBScrollbarDimensions(
    ContentTab* tab,
    float& top, float& right, float& bottom, float& left,
    float& thumbStart, float& thumbEnd
) {
    float height = CONTENT_CONTAINER.height;
    top = CONTENT_CONTAINER.top;
    right = CONTENT_CONTAINER.left + CONTENT_CONTAINER.width;
    left = right - SCROLL_BAR_THICKNESS;
    bottom = top + height;

    thumbStart = top + tab->scrollOffsetY / tab->maxHeight * height;
    thumbEnd = thumbStart + CONTENT_CONTAINER.height / tab->maxHeight * height;

    if (thumbEnd > top + height) {
        float barWidth = thumbEnd - thumbStart;

        thumbEnd = top + height;
        thumbStart = thumbEnd - barWidth;
    }
}

string elementType(UIElement *elem) {
    switch(elem->type) {
        case UIElementType::Label: return "UI Element Type: Label";
        case UIElementType::Button: return "UI Element Type: Button";
        case UIElementType::IconButton: return "UI Element Type: Icon Button";
        case UIElementType::Container: return "UI Element Type: Container";
        case UIElementType::TextInput: return "UI Element Type: Text Input";
        case UIElementType::TextArea: return "UI Element Type: Text Area";
        case UIElementType::TabBar: return "UI Element Type: Tab Bar";
        case UIElementType::Tab: return "UI Element Type: Tab";
        case UIElementType::ScrollArea: return "UI Element Type: Scroll Area";
    }

    return "UI Element Type: Unknown";
}

#ifdef DEBUG_TEXT_SELECTION
    float debug_lastTextSelTime = 0;
    float debug_lastTid = 0;

    void printTextSelectionVertices(vector<float> &fvec) {
        if (!(debug_lastTextSelTime + 1.0 < glfwGetTime() || debug_lastTextSelTime == 0)) {
            return;
        }

        bool pair = false;

        cout << "\nSelection vertices: ";

        for (float c: fvec) {
            if (pair) {
                cout << ", " << c << "), ";
            } else {
                cout << "(" << c;
            }

            pair = !pair;
        }

        debug_lastTextSelTime = glfwGetTime();
    }

    void printTextInputData(TextInputData* tid, string token) {
        // if (!(debug_lastTid + 1.0 < glfwGetTime() || debug_lastTid == 0)) {
        //     return;
        // }

        if (token.empty()) {
            cout << "\nText Input Data:";
        } else {
            cout << "\nText Input Data(" << token << "):";
        }

        cout << " Selection Start: (" << tid->textSelectStartRow << ", "
            << tid->textSelectStartCol << ") Selection End: ("
            << tid->textSelectEndRow << ", " << tid->textSelectEndCol << ")";

        debug_lastTid = glfwGetTime();
    }
#endif

#ifdef DEBUG_CONTENT_BLOCKS
string to_string(ContentBlock cb) {
    string str = cb.rope->getString();
    return to_string(cb.type) + ": " + str.substr(0, 16) + "(" + to_string(str.length()) + ")";
}

string to_string(ContentBlockType cbType) {
    switch (cbType) {
        case ContentBlockType::Title: return "Title";
        case ContentBlockType::Heading1: return "Heading1";
        case ContentBlockType::Heading2: return "Heading2";
        case ContentBlockType::Heading3: return "Heading3";
        case ContentBlockType::Heading4: return "Heading4";
        case ContentBlockType::Heading5: return "Heading5";
        case ContentBlockType::Heading6: return "Heading6";
        case ContentBlockType::Paragraph: return "Paragraph";
    }

    return "";
}

void printContentBlocksFromVector(vector<ContentBlock> cblock) {
    for (auto itr = cblock.begin(); itr != cblock.end(); ++itr) {
        cout << "\n         |-> " << to_string((*itr).type) << ": " <<
            (*itr).rope->getString().substr(0, 16);

        if ((*itr).rope->length() > 16) {
            cout << "... (" << (*itr).rope->length() << " chars)";
        }
    }
}

void printContentBlocks(uint32_t tabIndex) {
    if (tabIndex >= tabs.size()) {
        cerr << "\nError: Invalid tab index.";
        return;
    }

    cout << "\nContent Blocks: ";

    printContentBlocksFromVector(tabs[tabIndex].contentBlocks);
}

#endif

