#include <format>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <stack>
#include <thread>
#include <chrono>
#include <filesystem>
#include <algorithm>

#ifdef DEBUG_TEXT_INPUT
    #include <string_view>
#endif

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #include <processthreadsapi.h>
    #include <psapi.h>
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
#include "http.hpp"
#include "logger.hpp"
#include "../lib/stb_image.h"
#include "../lib/stb_image_resize2.h"
#include "md.hpp"

#define TIME() static_cast<float>(glfwGetTime())
#define LAST_IN_LIST(item) while (item->next != nullptr) { item = item->next; }
#define REDRAW_UI() grInfo.redrawUI = true
#define REDRAW_CB() grInfo.redraw = true

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

#define SETUP_BUFFERS_TEXT(vao, vbo, vertices) do {\
        glGenVertexArrays(1, &vao);\
        glBindVertexArray(vao);\
        glGenBuffers(1, &vbo);\
        glBindBuffer(GL_ARRAY_BUFFER, vbo);\
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);\
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(TextVertex), (void*)0);\
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(TextVertex), (void*) (2 * sizeof(float)));\
        glVertexAttribIPointer(2, 1, GL_UNSIGNED_INT, sizeof(TextVertex), (void*) (4 * sizeof(float)));\
        glEnableVertexAttribArray(0);\
        glEnableVertexAttribArray(1);\
        glEnableVertexAttribArray(2);\
    } while (false)

#define SETUP_BUFFERS_RECT(vao, vbo, vertices, bufferSize) do {\
        glGenVertexArrays(1, &vao);\
        glBindVertexArray(vao);\
        glGenBuffers(1, &vbo);\
        glBindBuffer(GL_ARRAY_BUFFER, vbo);\
        glBufferData(GL_ARRAY_BUFFER, bufferSize, vertices, GL_DYNAMIC_DRAW);\
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(RectVertex), (void*)0);\
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(RectVertex), (void*)(2 * sizeof(float)));\
        glVertexAttribIPointer(2, 1, GL_UNSIGNED_INT, sizeof(RectVertex), (void*) (6 * sizeof(float)));\
        glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(RectVertex), (void*) (6 * sizeof(float) + sizeof(uint32_t)));\
        glEnableVertexAttribArray(0);\
        glEnableVertexAttribArray(1);\
        glEnableVertexAttribArray(2);\
        glEnableVertexAttribArray(3);\
    } while (false)

#define DRAW_RECT(vao, color) do {\
        glBindVertexArray(vao);\
        glUseProgram(shaders[SHADER_DEFAULT]);\
        glUniform4f(uniforms[U_DEFAULT_COLOR], color.r, color.g, color.b, color.a);\
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);\
    } while (false)

#define PUSH_ICON_VERTICES(vec, ch, x, y, w, h, colorIndex) do {\
    vec.insert(vec.end(), {\
        TextVertex { x,     y,     ch.textureX1, ch.textureY1, colorIndex },\
        TextVertex { x + w, y,     ch.textureX2, ch.textureY1, colorIndex },\
        TextVertex { x,     y + h, ch.textureX1, ch.textureY2, colorIndex },\
        TextVertex { x,     y + h, ch.textureX1, ch.textureY2, colorIndex },\
        TextVertex { x + w, y,     ch.textureX2, ch.textureY1, colorIndex },\
        TextVertex { x + w, y + h, ch.textureX2, ch.textureY2, colorIndex },\
    });\
} while (false)

#define MOUSE_ON_EXPLORER_LINE(x, y) x >= EXPLORER.width - 3\
    && x <= EXPLORER.width + 2 && y > EXPLORER.top

#define LINE_HEIGHT(fontSize) static_cast<float>(fontSize) * (1 - glyphDescender)

#define SLEEP_ms(ms) std::this_thread::sleep_for(chrono::milliseconds(ms))
#define SLEEP_s(s) std::this_thread::sleep_for(chrono::seconds(s))

using namespace std;

float iconButtonVertices[16] = {
    -1.0, -1.0, -1.0, -1.0,
    -1.0, -1.0, -1.0, -1.0,
    -1.0, -1.0, -1.0, -1.0,
    -1.0, -1.0, -1.0, -1.0,
};

bool darkTheme = false;
bool rerenderUI = false;

float contentLeft = CONTENT_HORIZONTAL_MARGIN;
float contentRight = 530.0f;

HighlightedContentBlock highlightedContentBlock = { 0 };

ContentBlockMoveMarker cbMove = { 0, 0, 0, false, true };

vector<ContentTab> tabs;
TabsDisplay tabsDisplay;
uint8_t activeTabIndex;
UIElement* activeElement = nullptr;
uint16_t windowHeight;
uint16_t windowWidth;
glm::vec2 resolution;
GLFWwindow* window;
GLFWmonitor* primaryMonitor;
bool minimized = false;
bool fullScreen = false;
bool focused = true;
bool mouse1Down = false;
bool ignoreMouse1Up = false;
bool showFps = false;
bool ctrlKeyDown = false;
bool vaultSelected = false;
bool cbTextDragging = false;
CursorShape currentCursorShape = CursorShape::Default;
uint8_t* moveCursorBitmap;
ExplorerNode* targetNode = nullptr;

string selectedVault = "";

GLFWcursor* textCursor;
GLFWcursor* pointerCursor;
GLFWcursor* hResizeCursor;
GLFWcursor* moveCursor;

UIElement elements[DEFAULT_ELEM_LENGTH];
Arena floatingArena;
Arena mainUIArena;
float lastFloatingUIShown = 0;
bool dialogOpen = false;

float mouseX = 25000;
float mouseY = 25000;
float mouseDownX = 25000;
float mouseDownY = 25000;

uint32_t untitledNotes = 0;
uint32_t untitledFolders = 0;

uint32_t colorUBO = 0;
uint32_t commonUBO = 0;

Explorer explorer = {
    0, 0, 0, 0, 0, 0, 0, false, false, false, false, vector<ExplorerNode>()
};

GlobalRenderInfo grInfo;
MouseEventInfo gmei;
MouseEventInfo uiMouseEvents;
Tooltip tooltip = { 0 };
CBImageLoader cbImageLoader = { 0 };

Cursor globalCursor = { 0 };

#ifdef DEBUG_TEXT
    float prevTextTime = 0.0f;
    float textTime = 0.0f;
    float cumulativeTextTime = 0.0f;
    uint32_t textTimeCalculated = 0;
#endif

void showOpenGLRequiredError() {
    stringstream errorSS;

    errorSS << "Patram requires OpenGL to draw it's UI."
        << " Please make sure your system supports OpenGL and has graphics"
        << " driver with atleast OpenGL version "
        << OPENGL_MAJOR << "." << OPENGL_MINOR << " installed.";

#ifdef _WIN32
    MessageBoxA(
        NULL,
        errorSS.str().c_str(),
        "Error initializing OpenGL",
        MB_OK | MB_ICONERROR
    );
#else
    cerr << "Error initializing OpenGL. " << errorSS.str();
#endif
}

void showUnsupportedOpenGLVersionError(string currentVersion) {
    stringstream errorSS;

    errorSS << "\nYour OpenGL version \""
        << currentVersion
        << "\" is not supported. Please install latest graphics driver with OpenGL version \""
        << OPENGL_MAJOR << "." << OPENGL_MINOR << "\".";
#ifdef _WIN32
    MessageBoxA(NULL, errorSS.str().c_str(), "Error: Unsupported OpenGL version", MB_OK | MB_ICONERROR);
#else
    cerr << "Error: " << errorSS.str();
#endif

    exit(1);
}

void verifyOpenGLVersion() {
    string version;
    istringstream versionStream((const char *) glGetString(GL_VERSION));
    versionStream >> version;

    info("OpenGL version: " + version);

    versionStream.seekg(0);

    string majorStr;
    string minorStr;

    std::getline(versionStream, majorStr, '.');
    std::getline(versionStream, minorStr, '.');

    uint32_t major = std::stoul(majorStr);
    uint32_t minor = std::stoul(minorStr);

    if (major < OPENGL_MAJOR || major >= OPENGL_MAJOR && minor < OPENGL_MINOR) {
        showUnsupportedOpenGLVersionError(version);
    }
}

void showWindow() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, OPENGL_MAJOR);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, OPENGL_MINOR);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    primaryMonitor = glfwGetPrimaryMonitor();
    window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "patram", NULL, NULL);

    if (window == NULL) {
        showOpenGLRequiredError();
        glfwTerminate();
        exit(-1);
    }

    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        cerr << "\nFailed to initialize GLAD";
        exit(1);
    }

    verifyOpenGLVersion();

    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

    glfwSetKeyCallback(window, keyCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetFramebufferSizeCallback(window, resizeCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetWindowPosCallback(window, windowPosCallback);
    glfwSetScrollCallback(window, scrollCallback);
    glfwSetWindowIconifyCallback(window, minimizedCallback);
    glfwSetWindowMaximizeCallback(window, maximizedCallback);
    glfwSetWindowFocusCallback(window, windowFocusCallback);

    glfwSetInputMode(window, GLFW_LOCK_KEY_MODS, GLFW_TRUE);
}

void addRectangle(
    float x, float y, float w, float h,
    uint32_t colorIndex, float borderRadius, vector<RectVertex>& rva
) {
    rva.insert(rva.end(), {
        { x,     y,     x, y, w, h, colorIndex, borderRadius },
        { x + w, y,     x, y, w, h, colorIndex, borderRadius },
        { x,     y + h, x, y, w, h, colorIndex, borderRadius },
        { x,     y + h, x, y, w, h, colorIndex, borderRadius },
        { x + w, y,     x, y, w, h, colorIndex, borderRadius },
        { x + w, y + h, x, y, w, h, colorIndex, borderRadius },
    });
}

inline void drawRectangles(RenderInfo& ri, vector<RectVertex>& rva, bool redraw) {
    glBindVertexArray(ri.vao);

    if (redraw && !rva.empty()) {
        #ifdef DEBUG_RENDER
        if (!rva.empty()) {
            cout << "\nDEBUG: redrawing rectangles (" << rva.size() << " vertices).";

            printRectVertex(rva);
        } else {
            cout << "\nDEBUG: empty rectangles!";
        }
        #endif

        glBindBuffer(GL_ARRAY_BUFFER, ri.vbo);

        if (ri.numVertices < rva.size()) {
            ri.numVertices = rva.size();
            ri.bufferSize = rva.size() * sizeof(RectVertex);
            glBufferData(GL_ARRAY_BUFFER, rva.size() * sizeof(RectVertex), &rva[0], GL_DYNAMIC_DRAW);
        } else {
            for (uint32_t i=rva.size(); i<ri.numVertices; i+=6) {
                rva.insert(rva.begin(), {
                    RectVertex { 0 },
                    RectVertex { 0 },
                    RectVertex { 0 },
                    RectVertex { 0 },
                    RectVertex { 0 },
                    RectVertex { 0 },
                });
            }

            if (rva.size() != ri.numVertices) {
                ri.numVertices = rva.size();
                ri.bufferSize = rva.size() * sizeof(RectVertex);
            }

            glBufferSubData(GL_ARRAY_BUFFER, 0, ri.bufferSize, &rva[0]);
        }
    }

    glUseProgram(shaders[SHADER_RECTANGLE]);
    // glUniform1f(uniforms[U_RECTANGLE_FADEIN_TIME], 1.0f);

    glDrawArrays(GL_TRIANGLES, 0, ri.numVertices);
}

void toggleFullscreen() {
    if (primaryMonitor != nullptr) {
        if (fullScreen) {
            glfwSetWindowMonitor(window, NULL, 100, 100, WINDOW_WIDTH, WINDOW_HEIGHT, 0);
            fullScreen = false;
        } else {
            int x, y, width, height;
            const GLFWvidmode* vMode = glfwGetVideoMode(primaryMonitor);
            glfwGetMonitorWorkarea(primaryMonitor, &x, &y, &width, &height);
            glfwSetWindowMonitor(window, primaryMonitor, 0, 0, width, height, vMode->refreshRate);
            fullScreen = true;
        }
    }
}

void updateCursorShape(CursorShape shape) {
    if (shape == currentCursorShape && shape != CursorShape::Auto) { return; }

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
        case CursorShape::Move:
            glfwSetCursor(window, moveCursor);
            currentCursorShape = CursorShape::Move;
            break;
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
    }
}

void setupWelcomeScreen();
void destroyWelcomeScreen();
void onVaultBackClicked();
void populateExplorerFromIndex();
void populateExplorerFromContent();

void setSelectedVault(string vaultName) {
    selectedVault = vaultName;
    explorer.nodes.clear();

    populateExplorerFromIndex();

    thread explorerLoadThread(populateExplorerFromContent);
    explorerLoadThread.detach();

    setDefaultVault(vaultName);
    vaultSelected = true;
    REDRAW_UI();
}

void onVaultSetupCreateVaultClicked() {
    debug("Creating vault...");
    string vaultName;
    string vaultPassword;

    bool nameInit = false;
    bool passwordInit = false;

    for (int i=0; i<WELCOME_MAIN_CONTAINER.totalChildren && (!nameInit || !passwordInit); ++i) {
        if (WELCOME_MAIN_CONTAINER.children[i]->id == VAULT_SETUP_VAULT_NAME_TEXT_INPUT_ID) {
            vaultName = WELCOME_MAIN_CONTAINER.children[i]->text;
            nameInit = true;
        } else if (WELCOME_MAIN_CONTAINER.children[i]->id == VAULT_SETUP_VAULT_PASSWORD_TEXT_INPUT_ID) {
            vaultPassword = WELCOME_MAIN_CONTAINER.children[i]->text;
            passwordInit = true;
        }
    }

    if (nameInit) {
        createVaultInfoFile(vaultName, vaultPassword);
        createVaultIndexFile(vaultName, vaultPassword);
        setDefaultVault(vaultName);

        setSelectedVault(vaultName);

        onVaultBackClicked();
        return;
    }

    // TODO: display some error like "Vault name cannot be empty!"
}

void onVaultBackClicked() {
    destroyWelcomeScreen();
    setupWelcomeScreen();
    globalCursor.show = false;
}

void onCreateANewVaultClicked() {
    float vertices[] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };

    destroyWelcomeScreen();

    UIElement* title                = &elements[8];
    UIElement* vaultSetup           = &elements[9];
    UIElement* vaultNameLabel       = &elements[10];
    UIElement* vaultNameInput       = &elements[11];
    UIElement* vaultPasswordLabel   = &elements[12];
    UIElement* vaultPasswordInput   = &elements[13];
    UIElement* createVaultButton    = &elements[14];
    UIElement* backButton           = &elements[15];

    initArena(mainUIArena, 8192);

    initElement(*title, UIElementType::Label, 0.0f, 0.0f, 0.0f, 0.0f, &mainUIArena);
    initElement(*vaultSetup, UIElementType::Label, 0.0f, 0.0f, 0.0f, 0.0f, &mainUIArena);
    initElement(*vaultNameLabel, UIElementType::Label, 0.0f, 0.0f, 0.0f, 0.0f, &mainUIArena);
    initElement(*vaultPasswordLabel, UIElementType::Label, 0.0f, 0.0f, 0.0f, 0.0f, &mainUIArena);
    initElement(*vaultNameInput, UIElementType::TextInput, 0.0f, 0.0f, 240.0f, 30.0f, &mainUIArena);
    initElement(*vaultPasswordInput, UIElementType::TextInput, 0.0f, 0.0f, 240.0f, 30.0f, &mainUIArena);
    initElement(*createVaultButton, UIElementType::Button, 0.0f, 0.0f, 0.0f, 0.0f, &mainUIArena);
    initElement(*backButton, UIElementType::Button, 0.0f, 0.0f, 0.0f, 0.0f, &mainUIArena);

    for (int i=8; i<16; ++i) {
        elements[i].preferences->horizontalAlign = UIAlignment::Center;
        elements[i].preferences->widthPreference = UISizePreference::Grow;
        elements[i].positioning = UIPositioning::Relative_sibling;
        elements[i].parent = &WELCOME_MAIN_CONTAINER;

        if (i != 0) {
            elements[i].fontSize = 16;
            elements[i].marginTop = 8;
        }
    }

    title->text                 = "patram";
    vaultSetup->text            = "Vault Setup";
    vaultNameLabel->text        = "Vault Name:";
    vaultPasswordLabel->text    = "Vault Password:";
    vaultNameInput->text        = "Untitled Vault";
    vaultPasswordInput->text    = "password";
    createVaultButton->text     = "Create Vault";
    backButton->text            = "Back";

    vaultNameInput->textInputData->alignment = TextInputAlignment::Center;
    vaultPasswordInput->textInputData->alignment = TextInputAlignment::Center;
    vaultPasswordInput->textInputData->password = true;

    createVaultButton->preferences->heightPreference = UISizePreference::Grow;
    backButton->preferences->heightPreference = UISizePreference::Grow;

    title->fontSize = 48;
    vaultSetup->fontSize = 16;

    vaultNameInput->id      = VAULT_SETUP_VAULT_NAME_TEXT_INPUT_ID;
    vaultPasswordInput->id  = VAULT_SETUP_VAULT_PASSWORD_TEXT_INPUT_ID;
    createVaultButton->id   = VAULT_SETUP_CREATE_VAULT_BUTTON_ID;
    backButton->id          = VAULT_BACK_BUTTON_ID;

    addChild(&WELCOME_MAIN_CONTAINER, title, &mainUIArena);
    addChild(&WELCOME_MAIN_CONTAINER, vaultSetup, &mainUIArena);
    addChild(&WELCOME_MAIN_CONTAINER, vaultNameLabel, &mainUIArena);
    addChild(&WELCOME_MAIN_CONTAINER, vaultNameInput, &mainUIArena);
    addChild(&WELCOME_MAIN_CONTAINER, vaultPasswordLabel, &mainUIArena);
    addChild(&WELCOME_MAIN_CONTAINER, vaultPasswordInput, &mainUIArena);
    addChild(&WELCOME_MAIN_CONTAINER, createVaultButton, &mainUIArena);
    addChild(&WELCOME_MAIN_CONTAINER, backButton, &mainUIArena);
}

void onSwitchVaultClicked() {
    #define ELEM_COUNT  4

    float vertices[] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };

    destroyWelcomeScreen();

    UIElement* title        = &elements[8];
    UIElement* switchVault  = &elements[9];
    UIElement* vaultList    = &elements[10];
    UIElement* backButton   = &elements[11];

    initElement(*title, UIElementType::Label, 0.0f, 0.0f, 0.0f, 0.0f);
    initElement(*switchVault, UIElementType::Label, 0.0f, 0.0f, 0.0f, 0.0f);
    initElement(*vaultList, UIElementType::ScrollArea, 0.0f, 0.0f, 300.0f, 300.0f);
    initElement(*backButton, UIElementType::Button, 0.0f, 0.0f, 0.0f, 0.0f);

    vector<string> vaults = getVaults();

    SETUP_BUFFERS(vaultList->scrollData->verticalScrollBarVAO, vaultList->scrollData->verticalScrollBarVBO, vertices);
    SETUP_BUFFERS(vaultList->scrollData->verticalScrollThumbVAO, vaultList->scrollData->verticalScrollThumbVBO, vertices);

    for (int i=8; i<12; ++i) {
        elements[i].positioning = UIPositioning::Relative_sibling;
        elements[i].preferences->horizontalAlign = UIAlignment::Center;
        elements[i].preferences->widthPreference = UISizePreference::Grow;
        elements[i].parent = &WELCOME_MAIN_CONTAINER;

        if (i != 0) {
            elements[i].fontSize = 16;
        }
    }

    for (string vaultName : vaults) {
        UIElement* vaultElem = new UIElement;
        initElement(*vaultElem, UIElementType::Button, 0.0f, 0.0f, 0.0f, 35.0f);

        vaultElem->positioning = UIPositioning::Relative_sibling;
        vaultElem->preferences->widthPreference = UISizePreference::Full;
        vaultElem->preferences->horizontalAlign = UIAlignment::Start;
        vaultElem->borderRadius = 0;
        vaultElem->text = vaultName;
        vaultElem->id = SWITCH_VAULT_OPTION_ID;

        addChild(vaultList, vaultElem);
    }

    title->text         = "patram";
    switchVault->text   = "Switch Vault";
    vaultList->text     = "SWITCH_VLT_SCR";
    backButton->text    = "Back";

    title->fontSize = 48;
    switchVault->fontSize = 16;

    vaultList->scrollData->showVerticalScrollbar = true;
    vaultList->marginTop = 8;
    vaultList->preferences->direction = false;

    backButton->preferences->heightPreference = UISizePreference::Grow;
    backButton->id = VAULT_BACK_BUTTON_ID;
    backButton->marginTop = 8;

    addChild(&WELCOME_MAIN_CONTAINER, title);
    addChild(&WELCOME_MAIN_CONTAINER, switchVault);
    addChild(&WELCOME_MAIN_CONTAINER, vaultList);
    addChild(&WELCOME_MAIN_CONTAINER, backButton);

    #undef ELEM_COUNT
}

tuple<ExplorerNodeType, filesystem::path> getActiveExplorerNodePath(vector<ExplorerNode>& nodes) {
    ExplorerNodeType ent;
    filesystem::path p;

    for (auto itr = nodes.begin(); itr != nodes.end(); ++itr) {
        if (GET_FLAG((*itr).flags, EXPLORER_FLAG_ACTIVE)) {
            return make_tuple((*itr).type, (*itr).name);
        }

        if ((*itr).type == ExplorerNodeType::Folder && GET_FLAG((*itr).flags, EXPLORER_FLAG_EXPANDED)) {
            tuple<ExplorerNodeType, filesystem::path> tup = getActiveExplorerNodePath((*itr).children);
            filesystem::path _p = get<1>(tup);

            if (!_p.empty()) {
                ent = get<0>(tup);
                p /= (*itr).name / _p;
                break;
            }
        }
    }

    return make_tuple(ent, p);
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
                    UNSET_FLAG((*itr).flags, EXPLORER_FLAG_ACTIVE);
                    #ifdef DEBUG_EXPLORER
                    cout << "\nDEBUG: explorer flag " << (int)((*itr).flags) << " unset for "
                        << (*itr).name << ", vertices:\n";
                    #endif
                } else {
                    SET_FLAG((*itr).flags, EXPLORER_FLAG_EXPANDED);
                    SET_FLAG((*itr).flags, EXPLORER_FLAG_ACTIVE);
                    #ifdef DEBUG_EXPLORER
                    cout << "\nDEBUG: explorer flag " << (int)((*itr).flags) << " set for "
                        << (*itr).name << ", vertices:\n";
                    #endif
                }
                REDRAW_UI();
            } else {
                SET_FLAG((*itr).flags, EXPLORER_FLAG_ACTIVE);
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
        UNSET_FLAG(itr->flags, flag);

        if (!itr->children.empty()) {
            unsetExplorerTreeFlag(itr->children, flag);
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
            explorer.hoverY = itr->top - 4.0f;
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

            if (updateMouseDownEvents(mouseX, mouseY, gmei)) { REDRAW_CB(); }
            if (updateMouseDownEvents(mouseX, mouseY, uiMouseEvents)) { rerenderUI = true; }

            if (gmei.mouseDown) {
                Bounds b = gmei.clickBounds;

                if (b.type == BoundSourceType::Link && ctrlKeyDown) {
                    ContentTab tab = tabs[b.source.cb.tabIndex];
                    ContentBlock cb = tab.contentBlocks[b.source.cb.index];
                    MarkdownParser mp(cb.rope);
                    Markdown md = mp.parse();

                    if (
                        md.cbType == ContentBlockType::Link && !md.fragments.empty()
                        && md.fragments[0].fragType == MarkdownFragmentType::Link
                    ) {
                        string url = cb.rope->substring(md.fragments[0].imgOrLink.url);
                        info("Opening URL");

                        if (!openLink(url)) {
                            error("Could not open the URL since this is not a web URL!");
                        }
                    }

                    return;
                }
            }

            if (uiMouseEvents.mouseDown) {
                bool actionFound = false;
                Bounds b = uiMouseEvents.clickBounds;

                switch (b.type) {
                    case BoundSourceType::None: break;
                    case BoundSourceType::ContentBlock: break;
                    case BoundSourceType::Link: break;
                    case BoundSourceType::Button: {
                        if (grInfo.activeLayer == 0) {
                            switch(b.source.elem->id) {
                                case CREATE_VAULT_BUTTON_ID:
                                    onCreateANewVaultClicked();
                                    actionFound = true;
                                    break;
                                case SWITCH_VAULT_BUTTON_ID:
                                    onSwitchVaultClicked();
                                    actionFound = true;
                                    break;
                                case VAULT_SETUP_CREATE_VAULT_BUTTON_ID:
                                    onVaultSetupCreateVaultClicked();
                                    actionFound = true;
                                    break;
                                case VAULT_BACK_BUTTON_ID:
                                    onVaultBackClicked();
                                    actionFound = true;
                                    break;
                                case NEW_NOTE_BUTTON_ID:
                                    newNote();
                                    actionFound = true;
                                    break;
                                case NEW_FOLDER_BUTTON_ID:
                                    showNewFolderDialog();
                                    actionFound = true;
                                    break;
                                case TOGGLE_EXPLORER_BUTTON_ID:
                                    toggleExplorer();
                                    actionFound = true;
                                    break;
                                case SWITCH_VAULT_OPTION_ID:
                                    setSelectedVault(b.source.elem->text);
                                    onVaultBackClicked();
                                    actionFound = true;
                                    break;
                                case EXPLORER_MENU_RENAME_ID:
                                    showRenameDialog();
                                    actionFound = true;
                                    break;
                                case EXPLORER_MENU_DELETE_ID:
                                    showDeleteDialog();
                                    actionFound = true;
                                    break;
                                case EXPLORER_MENU_NEW_NOTE_ID:
                                    showNewNoteDialog();
                                    actionFound = true;
                                    break;
                                case EXPLORER_MENU_NEW_FOLDER_ID:
                                    showNewFolderDialog();
                                    actionFound = true;
                                    break;
                            }
                        } else {
                            switch(b.source.elem->id) {
                                case RENAME_DIALOG_OK_BUTTON_ID:
                                    renameActiveExplorerNode();
                                    actionFound = true;
                                    break;
                                case DELETE_DIALOG_DELETE_BUTTON_ID:
                                    deleteActiveExplorerNode();
                                    actionFound = true;
                                    break;
                                case NEW_NOTE_DIALOG_CREATE_BUTTON_ID:
                                    createNoteInActiveExplorerNode();
                                    actionFound = true;
                                    break;
                                case NEW_FOLDER_DIALOG_CREATE_BUTTON_ID:
                                    createFolderInActiveExplorerNode();
                                    actionFound = true;
                                    break;
                                case RENAME_DIALOG_CANCEL_BUTTON_ID:
                                case DELETE_DIALOG_CANCEL_BUTTON_ID:
                                case NEW_FOLDER_DIALOG_CANCEL_BUTTON_ID:
                                case ERROR_DIALOG_OK_BUTTON_ID:
                                    unsetExplorerTreeFlag(explorer.nodes, EXPLORER_FLAG_ACTIVE);
                                    destroyFloatingUI();
                                    actionFound = true;
                                    break;
                            }
                        }
                    }
                    break;

                    case BoundSourceType::TabClose:
                        actionFound = true;
                        closeTab(b.source.tabIndex);
                        break;

                    case BoundSourceType::Tab: {
                        actionFound = true;
                        activeTabIndex = b.source.tabIndex;
                        TAB_BAR.active = false;

                        if (activeElement != nullptr) {
                            activeElement->active = false;
                        }

                        activeElement = &CONTENT_CONTAINER;
                        CONTENT_CONTAINER.active = true;

                        ContentTab* tab = CURRENT_TAB;
                        tab->redrawCursor = true;
                        REDRAW_CB();
                    }
                    break;

                    case BoundSourceType::ExplorerElement: {
                        auto node_tuple = getClickedExplorerNodePath(explorer.nodes, b.source.explorerY, mouseY);
                        ExplorerNodeType t = get<0>(node_tuple);
                        filesystem::path p = get<1>(node_tuple);

                        float time = TIME();

                        if (t == ExplorerNodeType::Note) {
                            if (time - explorer.lastClickedTime < 0.4f) {
                                explorer.lastClickedTime = 0.0f;

                                filesystem::path nodePath = getVaultPath(selectedVault) / p;
                                openFile(false, nodePath);
                            } else {
                                explorer.lastClickedTime = time;
                            }
                        }
                    }
                    break;
                }

                if (actionFound) {
                    if (b.type == BoundSourceType::Button && b.source.elem != nullptr) {
                        UIElement* prevActiveElement = activeElement;

                        if (activeElement != nullptr) {
                            activeElement->active = false;
                        }

                        activeElement = b.source.elem;
                        b.source.elem->active = true;
                    }

                    REDRAW_UI();
                    ignoreMouse1Up = true;
                    return;
                }
            }

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
                    case UIElementType::Explorer:
                        unsetExplorerTreeFlag(explorer.nodes, EXPLORER_FLAG_ACTIVE);
                        if (
                            elem->scrollData->showVerticalScrollbar
                            && mouseX > elem->width - SCROLL_BAR_THICKNESS
                        ) {
                            elem->scrollData->mouseDown = true;
                            elem->scrollData->mouseDownX = mouseX;
                            elem->scrollData->mouseDownY = mouseY;
                        }
                        break;
                }
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

            gmei.mouseDown = false;
            uiMouseEvents.mouseDown = false;

            if (ignoreMouse1Up) {
                ignoreMouse1Up = false;
                return;
            }

            if (highlightedContentBlock.show && highlightedContentBlock.mouseDown) {
                cout << "\nHighlighted content block RELEASE target: ";

                if (cbMove.targetIndex > 0 && cbMove.targetIndex < CURRENT_TAB->contentBlocks.size()) {
                    cout << tabs[activeTabIndex].contentBlocks[cbMove.targetIndex].rope->text.substr(0, 10);
                } else {
                    cout << "NULL";
                }

                processCBCommand({
                    CommandType::MoveContentBlock,
                    false,
                    0,
                    cbMove.targetIndex,
                    highlightedContentBlock.cbIndex,
                    ""
                });

                tabs[activeTabIndex].contentBlocks[highlightedContentBlock.cbIndex].flags &= ~(CONTENT_BLOCK_DRAGGING);
                REDRAW_CB();

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
            }
        }

        if (dialogOpen) {
            for (uint32_t i=0; i<FLOATING_CONTAINER.totalChildren; ++i) {
                UIElement* elem = FLOATING_CONTAINER.children[i];

                if (
                    elem->type == UIElementType::Container
                    && (
                        mouseX < elem->left || mouseX > elem->left + elem->width
                        || mouseY < elem->top || mouseY > elem->top + elem->height
                   )
                ) {
                    dialogOpen = false;
                    destroyFloatingUI();
                    break;
                }
            }
        }
    } else if (button == GLFW_MOUSE_BUTTON_2) {
        if (action == GLFW_PRESS) {
            UIElement* elem = getElementFromScreenCoords(&ROOT_CONTAINER, mouseX, mouseY);

            if (elem != nullptr) {
                switch (elem->id) {
                    case EXPLORER_PANE_ID: {
                        unsetExplorerTreeFlag(explorer.nodes, EXPLORER_FLAG_ACTIVE);
                        float y = EXPLORER.top + elem->scrollData->yOffset;

                        tuple<ExplorerNodeType, filesystem::path> _tuple =
                            getClickedExplorerNodePath(
                                explorer.nodes, y, mouseY
                            );

                        REDRAW_UI();
                        showExplorerContextMenu(mouseX, mouseY);
                        return;
                    }

                    case EXPLORER_MENU_RENAME_ID: return;
                }
            }

            destroyFloatingUI();
        }
    } else if (button == GLFW_MOUSE_BUTTON_3) {
        if (action == GLFW_PRESS) {
            UIElement* elem = getElementFromScreenCoords(&ROOT_CONTAINER, mouseX, mouseY);

            if (elem != nullptr) {
                switch (elem->type) {
                    case UIElementType::TabBar:
                        tabBarMouseDownCallback(elem, mouseX, mouseY, resolution, GLFW_MOUSE_BUTTON_3);
                        break;
                }
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

    if (key == GLFW_KEY_LEFT_CONTROL || key == GLFW_KEY_RIGHT_CONTROL && action == GLFW_PRESS) {
        ctrlKeyDown = true;
    } else {
        ctrlKeyDown = false;
    }

    if (activeElement != nullptr) {
        switch (activeElement->type) {
            case UIElementType::ContentContainer: cbKeyCallback(key, action, mods); break;
            case UIElementType::TextInput:
            case UIElementType::TextArea: textInputKeyCallback(activeElement, key, action, mods); break;
            default: defaultKeyCallback(activeElement, key, action, mods);
        }
    } else {
        defaultKeyCallback(nullptr, key, action, mods);
        return;
    }

    if (activeElement != nullptr && !activeElement->active) {
        activeElement = nullptr;
    }
}

void resizeCallback(GLFWwindow* window, int width, int height) {
    glClear(GL_COLOR_BUFFER_BIT);
    glViewport(0, 0, width, height);

    ROOT_CONTAINER.width = width;
    ROOT_CONTAINER.height = height;

    windowWidth = width;
    windowHeight = height;

    if (!minimized) {
        #ifdef DEBUG_LAYOUT
            cout << "\nRecalculating layout";
        #endif
        REDRAW_UI();

        recalculateLayout(&ROOT_CONTAINER); // TODO: get rid of this!
    }

    highlightedContentBlock.show = false;
    highlightedContentBlock.updated = true;

    resolution = glm::vec2(width, height);
    CommonUniforms c = { resolution, darkTheme };

    glBindBuffer(GL_UNIFORM_BUFFER, commonUBO);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(CommonUniforms), &c);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    glm::vec2 cursor = glm::vec2(mouseX, mouseY);

    if (CONTENT_CONTAINER.show && !tabs.empty()) {
        grInfo.redraw = true;
        recalculateContentWidth();

        ContentTab* tab = CURRENT_TAB;

        tab->redrawCursor = true;

        if (tab->textSelected) {
            tab->redrawSel = true;
        }

        if (tab->showCursor) {
            tab->redrawCursor = true;
        }

        tab->scrollbarUpdated = false;

        adjustTabOffset();

        if (tabs.size() > 1) {
            for (auto tabIter = tabs.begin(); tabIter != tabs.end(); ++tabIter) {
                (*tabIter).scrollbarUpdated = false;
            }
        }
    }

    hideContextMenus();

    render();

    glfwSwapBuffers(window);
}

void cursorPosCallback(GLFWwindow * window, double xpos, double ypos) {
    mouseX = xpos;
    mouseY = ypos;

    CursorShape cShape = CursorShape::Default;

    if (updateMouseHoverEvents(xpos, ypos, gmei)) { REDRAW_CB(); }
    if (updateMouseHoverEvents(xpos, ypos, uiMouseEvents)) { rerenderUI = true; }

    if (gmei.hover) {
        Bounds b = gmei.hoverBounds;

        if (b.type == BoundSourceType::Link && !tooltip.show) {
            ContentTab tab = tabs[b.source.cb.tabIndex];
            ContentBlock cb = tab.contentBlocks[b.source.cb.index];
            MarkdownParser mp(cb.rope);
            Markdown md = mp.parse();

            if (
                md.cbType == ContentBlockType::Link && !md.fragments.empty()
                && md.fragments[0].fragType == MarkdownFragmentType::Link
            ) {
                string url = cb.rope->substring(md.fragments[0].imgOrLink.url);
                cout << "\nDEBUG: Showing tooltip!";
                showTooltip(
                    b.x1, b.y1,
                    b.x2, b.y2,
                    TooltipDirection::Top,
                    true,
                    url + " (Ctrl + click)"
                );
            }
        }
    } else {
        hideTooltip();
    }

    if (!highlightedContentBlock.mouseDown && !tabs.empty()) {
        ContentTab* tab = CURRENT_TAB;

        float top;
        uint32_t oldIndex = highlightedContentBlock.cbIndex;
        bool oldShow = highlightedContentBlock.show;
        highlightedContentBlock.show = false;

        for (
            auto itr = tab->contentBlocks.begin();
            itr != tab->contentBlocks.end();
            ++itr
        ) {
            top = (*itr).top - tab->scrollOffsetY;

            if (ypos > top && ypos < top + (*itr).height) {
                if (mouseX > contentLeft) {
                    cShape = CursorShape::Text;
                } else if (mouseX > contentLeft - CONTENT_BLOCK_DRAGTAB_WIDTH) {
                    cShape = CursorShape::Move;
                }

                if ((*itr).type == ContentBlockType::Title) {
                    highlightedContentBlock.show = false;
                    break;
                }

                // TODO: Make sure the cursor is not over a scrollbar.
                highlightedContentBlock.show = true;
                highlightedContentBlock.left = contentLeft;
                highlightedContentBlock.top = top;
                highlightedContentBlock.bottom = top + (*itr).height - ((glyphDescender < 0) ? glyphDescender : 0);
                highlightedContentBlock.right = contentRight;
                uint32_t i = std::distance(tab->contentBlocks.begin(), itr);
                highlightedContentBlock.cbIndex = i;
                tab->activeCBIndex = i;
                break;
            }

            // TODO: remove this when 1 is completed
            if (top > windowHeight) {
                break;
            }
        }

        if (
            oldShow != highlightedContentBlock.show
            || highlightedContentBlock.show
            && oldIndex != highlightedContentBlock.cbIndex
        ) {
            REDRAW_CB();
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

                grInfo.redraw = true;

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

                grInfo.redraw = true;

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

            REDRAW_UI();
            explorer.redrawLine = true;

            recalculateLayout(&ROOT_CONTAINER);
            recalculateContentWidth();

            if (!tabs.empty()) {
                CURRENT_TAB->redrawCursor = true;
            }
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
                updateCursorShape(cShape);
                return;
            case UIElementType::TextArea:
            case UIElementType::TextInput:
                cShape = CursorShape::Text;
                textInputMouseDragCallback(activeElement, mouseX, mouseY, resolution);
                updateCursorShape(cShape);
                return;
        }
    }

    UIElement* elem = getElementFromScreenCoords(&ROOT_CONTAINER, mouseX, mouseY);

    if (elem != nullptr) {
        switch (elem->type) {
            case UIElementType::Explorer: {
                if (MOUSE_ON_EXPLORER_LINE(mouseX, mouseY)) {
                    REDRAW_UI();
                    if (!explorer.lineHover) {
                        explorer.lineHover = true;
                        explorer.redrawLine = true;
                    }
                    cShape = CursorShape::HorizontalResize;
                    updateCursorShape(cShape);
                    return;
                } else {
                    explorer.lineHover = false;
                }

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
                            REDRAW_UI();
                            elem->scrollData->renderedUpdate = false;
                            cout << "\nScroll offset: " << elem->scrollData->yOffset;
                        }
                        elem->scrollData->mouseDownY = mouseY;
                    }
                    updateCursorShape(CursorShape::Default);
                } else {
                    REDRAW_UI();
                    float y = EXPLORER.top + elem->scrollData->yOffset;

                    setExplorerTreeFlag(explorer.nodes, y, mouseY, elem->scrollData->yOffset, EXPLORER_FLAG_HOVER);
                    cShape = CursorShape::Default;
                }
            }
            break;

            case UIElementType::TextInput:
                cShape = CursorShape::Text;
                break;
        }
    } else if (explorer.elementHover) {
        unsetExplorerTreeFlag(explorer.nodes, EXPLORER_FLAG_HOVER);
        explorer.elementHover = false;
    }

    explorer.lineHover = false;
    updateCursorShape(cShape);
}

void windowPosCallback(GLFWwindow* window, int x, int y) {
    hideContextMenus();
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
                    REDRAW_UI();
                    elem->scrollData->lastScrolled = glfwGetTime();
                    elem->scrollData->renderedUpdate = false;

                    for (int i=0; i<elem->totalChildren; ++i) {
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

                REDRAW_CB();

                tab->updated = true;

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

#ifdef _WIN32
    if (minimized) {
        setEfficiencyMode(true);
        EmptyWorkingSet(GetCurrentProcess());
    } else {
        setEfficiencyMode(false);
    }
#endif
}

void maximizedCallback(GLFWwindow* window, int min) {
    // minimized = min == 0;
}

void windowFocusCallback(GLFWwindow *window, int isFocused) {
    focused = isFocused != 0;

#ifdef _WIN32
    setEfficiencyMode(!focused);
#endif
}

#ifdef _WIN32
void setEfficiencyMode(bool efficiencyModeOn) {
    PROCESS_POWER_THROTTLING_STATE ppts{};
    ppts.Version = PROCESS_POWER_THROTTLING_CURRENT_VERSION;
    ppts.ControlMask = PROCESS_POWER_THROTTLING_EXECUTION_SPEED | PROCESS_POWER_THROTTLING_IGNORE_TIMER_RESOLUTION;
    ppts.StateMask = efficiencyModeOn ? (PROCESS_POWER_THROTTLING_EXECUTION_SPEED | PROCESS_POWER_THROTTLING_IGNORE_TIMER_RESOLUTION) : 0;
    SetPriorityClass(GetCurrentProcess(), efficiencyModeOn ? IDLE_PRIORITY_CLASS : NORMAL_PRIORITY_CLASS);
    SetProcessInformation(GetCurrentProcess(), ProcessPowerThrottling, &ppts, sizeof(ppts));
}
#endif

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

        debugv("Time taken to get entries from index: {} microseconds", duration.count());

        stringstream ss;

        for (VaultIndexEntry entry : index) {
            ss << "\"" << entry.title << "\", ";
        }

        debugv("Index entries: {}", ss.str());
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

    if (index.empty()) {
        if (!explorer.nodes.empty()) {
            explorer.nodes.clear();
            REDRAW_UI();
        }
    } else {
        #ifdef DEBUG_EXPLORER
            auto t2 = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1);

            debugv("Time taken to get entries from content: {} microseconds", duration.count());

            stringstream ss;

            for (VaultIndexEntry entry : index) {
                ss << "\"" << entry.title << "\", ";
            }

            debugv("Content entries: {}", ss.str());
        #endif

        vector<ExplorerNode> newExplorerNodes = createExplorerNodesFromIndex(index);

        if (newExplorerNodes != explorer.nodes) {
            setVaultIndex(selectedVault, index);
            explorer.nodes = newExplorerNodes;
            REDRAW_UI();
#ifdef DEBUG_EXPLORER
        } else {
            debug("Entries in index and contents in vault are same. No changes made!");
#endif
        }

        #ifdef DEBUG_EXPLORER
            auto t3 = std::chrono::high_resolution_clock::now();
            duration = std::chrono::duration_cast<std::chrono::microseconds>(t3 - t2);

            debugv("Time taken to get, compare (and maybe also to assign) entries: {} microseconds", duration.count());
        #endif
    }

    #ifdef DEBUG_EXPLORER
        auto t4 = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(t4 - t1);
        debugv("Total Time taken in {}: {} microseconds", __func__, duration.count());
    #endif
}

void setupWelcomeScreen() {
    float vertices[] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };

    UIElement* title = &elements[8];
    UIElement* version = &elements[9];
    UIElement* line1 = &elements[10];
    UIElement* line2 = &elements[11];
    UIElement* line3 = &elements[12];
    UIElement* line4 = &elements[13];
    UIElement* line5 = &elements[14];
    UIElement* createVaultButton = &elements[15];
    UIElement* switchVaultButton = &elements[16];

    if (WELCOME_MAIN_CONTAINER.totalChildren) {
        destroyWelcomeScreen();
    }

    initArena(mainUIArena, 8192);

    initElement(*title, UIElementType::Label, 0.0f, 0.0f, 0.0f, 0.0f, &mainUIArena);
    initElement(*version, UIElementType::Label, 0.0f, 0.0f, 0.0f, 0.0f, &mainUIArena);
    initElement(*line1, UIElementType::Label, 0.0f, 0.0f, 0.0f, 0.0f, &mainUIArena);

    if (vaultSelected) {
        initElement(*line2, UIElementType::Button, 0.0f, 0.0f, 0.0f, 0.0f, &mainUIArena);
        initElement(*line3, UIElementType::Button, 0.0f, 0.0f, 0.0f, 0.0f, &mainUIArena);
        initElement(*line4, UIElementType::Button, 0.0f, 0.0f, 0.0f, 0.0f, &mainUIArena);
        initElement(*line5, UIElementType::Button, 0.0f, 0.0f, 0.0f, 0.0f, &mainUIArena);
    } else {
        initElement(*line2, UIElementType::Label, 0.0f, 0.0f, 0.0f, 0.0f, &mainUIArena);
        initElement(*line3, UIElementType::Label, 0.0f, 0.0f, 0.0f, 0.0f, &mainUIArena);
        initElement(*line4, UIElementType::Label, 0.0f, 0.0f, 0.0f, 0.0f, &mainUIArena);
        initElement(*line5, UIElementType::Label, 0.0f, 0.0f, 0.0f, 0.0f, &mainUIArena);
    }

    initElement(*createVaultButton, UIElementType::Button, 0.0f, 0.0f, 0.0f, 0.0f, &mainUIArena);
    initElement(*switchVaultButton, UIElementType::Button, 0.0f, 0.0f, 0.0f, 0.0f, &mainUIArena);

    for (uint32_t i=8; i<17; ++i) {
        elements[i].preferences->horizontalAlign = UIAlignment::Center;
        elements[i].preferences->widthPreference = UISizePreference::Grow;
        elements[i].positioning = UIPositioning::Relative_sibling;
        elements[i].fontSize = 16;
    }

    title->text = "patram";
    title->fontSize = 48;
    version->text = "v0.1.0";
    line1->marginTop = 8;

    if (vaultSelected) {
        for (uint32_t i=9; i<17; ++i) {
            elements[i].preferences->heightPreference = UISizePreference::Grow;
            elements[i].marginTop = 8;
        }

        line1->text = getDefaultVault();
        line2->text = "New note (Ctrl + N)";
        line3->text = "New Folder (Ctrl + Shift + N)";
        line4->text = "Save Note (Ctrl + S)";
        line5->text = "Toggle Explorer (Ctrl + E)";
        switchVaultButton->text = "Switch note vault";

        line1->fontSize = 24;

        line2->id = NEW_NOTE_BUTTON_ID;
        line3->id = NEW_FOLDER_BUTTON_ID;
        line5->id = TOGGLE_EXPLORER_BUTTON_ID;
        switchVaultButton->id = SWITCH_VAULT_BUTTON_ID;
    } else {
        line1->text = "Welcome to patram - your lightweight notes manager!";
        line2->text = "You begin by creating a note vault.";
        line3->text = "Note vaults contain notes and folders.";
        line4->text = "Folders can have other notes and folders.";
        line5->text = "You can password-protect a note vault - this encrypts all the notes in it.";
    }

    createVaultButton->text = "Create a note vault";
    createVaultButton->preferences->heightPreference = UISizePreference::Grow;
    createVaultButton->id = CREATE_VAULT_BUTTON_ID;
    createVaultButton->marginTop = 8;

    addChild(&WELCOME_MAIN_CONTAINER, title, &mainUIArena);
    addChild(&WELCOME_MAIN_CONTAINER, version, &mainUIArena);
    addChild(&WELCOME_MAIN_CONTAINER, line1, &mainUIArena);
    addChild(&WELCOME_MAIN_CONTAINER, line2, &mainUIArena);
    addChild(&WELCOME_MAIN_CONTAINER, line3, &mainUIArena);
    addChild(&WELCOME_MAIN_CONTAINER, line4, &mainUIArena);
    addChild(&WELCOME_MAIN_CONTAINER, line5, &mainUIArena);
    addChild(&WELCOME_MAIN_CONTAINER, createVaultButton, &mainUIArena);

    if (vaultSelected) {
        addChild(&WELCOME_MAIN_CONTAINER, switchVaultButton, &mainUIArena);
    }

    REDRAW_UI();
}

void destroyWelcomeScreen() {
    destroyArena(mainUIArena);

    for (int i=8; i<17; ++i) {
        elements[i].text.~string();
    }

    WELCOME_MAIN_CONTAINER.totalChildren = 0;
    WELCOME_MAIN_CONTAINER.capacity = 0;
    WELCOME_MAIN_CONTAINER.children = nullptr;
}

inline void appendVerticesForExplorerNode(
    float x,
    float& y,
    uint32_t textColor,
    uint32_t rectColor,
    vector<ExplorerNode>& nodes,
    vector<TextVertex>& vertices,
    vector<TextVertex>& iVertices,
    vector<RectVertex>& rv
) {
    float initialY = y;

    for (
        auto itr = nodes.begin();
        itr != nodes.end();
        ++itr, y += EXPLORER_ELEMENT_HEIGHT
    ) {
        appendTextVertices(x, y, itr->name, 0, textColor, 16, vertices);
        itr->top = y;

        uiMouseEvents.bounds.push_back(Bounds {
            0, y - 4.0f, EXPLORER.width, y - 4.0f + EXPLORER_ELEMENT_HEIGHT,
            BoundSourceType::ExplorerElement, true, true, { .explorerY = y }
        });

        if (GET_FLAG(itr->flags, EXPLORER_FLAG_ACTIVE)) {
            explorer.activeY = y - 4.0f;

            addRectangle(
                0, y - 4.0f, EXPLORER.width, EXPLORER_ELEMENT_HEIGHT,
                rectColor, 4, rv
            );
        }

        if (itr->type == ExplorerNodeType::Folder) {
            if (
                GET_FLAG(itr->flags, EXPLORER_FLAG_EXPANDED)
                && !itr->children.empty()
            ) {
                appendCharVertices(
                    x - 16.0f, y - 8.0f, icons[ICON_CHEVRON_DOWN],
                    textColor, iVertices
                );
                y += EXPLORER_ELEMENT_HEIGHT;
                appendVerticesForExplorerNode(
                    x + 24.0f, y, textColor, rectColor, itr->children,
                    vertices, iVertices, rv
                );
            } else {
                appendCharVertices(
                    x - 12.0f, y - 10.0f, icons[ICON_CHEVRON_RIGHT],
                    textColor, iVertices
                );
            }
        }
    }

    if (y != initialY) {
        y -= EXPLORER_ELEMENT_HEIGHT;
    }
}

void setupTabs(vector<TextVertex>& textVertices, vector<TextVertex>& xVertices) {
    float x = TAB_BAR.left;
    uint32_t i = 0;

    uint32_t textIndex = darkTheme ? WHITE_INDEX : BLACK_INDEX;
    float verticalPadding = (TAB_BAR_HEIGHT - 16 * (1 - glyphDescender)) / 2;

    for (ContentTab& tab: tabs) {
        float prevX = x;
        string tabtext = tab.filePath.filename().string();

        if (tabtext.length() > 24) {
            if (tab.updated) {
                tabtext = tabtext.substr(0, 23) + "*...";
            } else {
                tabtext = tabtext.substr(0, 24) + "...";
            }
        } else if (tab.updated) {
            tabtext += "*";
        }

        tab.xOffset = static_cast<uint16_t>(x);

        x += TAB_HORIZONTAL_PADDING;

        x += appendTextVertices(
            x - tabsDisplay.xOffset, verticalPadding, tabtext, 0, textIndex,
            16, textVertices
        ).width;

        x += TAB_HORIZONTAL_PADDING;

        float closeX = x;

        x += appendCharVertices(
            closeX, -2, icons[ICON_CLOSE], textIndex, xVertices
        );

        uiMouseEvents.bounds.push_back(Bounds {
            closeX - 5, verticalPadding + 2, x + 5, verticalPadding + 10 + x - closeX,
            BoundSourceType::TabClose, false, true, { .tabIndex = i }
        });

        x += TAB_HORIZONTAL_PADDING;

        tab.width = static_cast<uint16_t>(x - prevX);

        uiMouseEvents.bounds.push_back(Bounds {
            prevX, 0, x, TAB_BAR_HEIGHT,
            BoundSourceType::Tab, false, true, { .tabIndex = i }
        });

        ++i;
    }

    tabsDisplay.totalX = x;

    if (x > windowWidth - 2 * TAB_BAR_HEIGHT && TAB_BAR.totalChildren == 0) {
        float vertices[8] = { 0 };
        TextVertex textVertices[6] = { 0 };

        UIElement* buttons = new UIElement[2];

        initElement(buttons[0], UIElementType::IconButton, 0.0f, windowWidth - 2 * TAB_BAR_HEIGHT, TAB_BAR_HEIGHT, TAB_BAR_HEIGHT);
        initElement(buttons[1], UIElementType::IconButton, 0.0f, windowWidth - TAB_BAR_HEIGHT, TAB_BAR_HEIGHT, TAB_BAR_HEIGHT);

        buttons[0].icon = Icon::ChevronLeft;
        buttons[1].icon = Icon::ChevronRight;
        buttons[0].id = static_cast<uint8_t>(TAB_BAR_PREV_TAB_ID);
        buttons[1].id = static_cast<uint8_t>(TAB_BAR_NEXT_TAB_ID);

        initializeCapacity(&TAB_BAR, 2);
        addChild(&TAB_BAR, &buttons[0]);
        addChild(&TAB_BAR, &buttons[1]);
    }
}

void setupExplorer(
    vector<TextVertex>& vertices,
    vector<TextVertex>& iVertices,
    vector<RectVertex>& rv
) {
    if (explorer.nodes.empty()) { return; }

    const float y0 = EXPLORER.top + 4.0f - EXPLORER.scrollData->yOffset;
    float y = y0;

    vertices.reserve(1800);
    iVertices.reserve(240);

    appendVerticesForExplorerNode(
        24.0f, y,
        darkTheme ? WHITE_INDEX : BLACK_INDEX,
        darkTheme ? EXPLORER_HOVER_BG_DARK_INDEX : EXPLORER_HOVER_BG_INDEX,
        explorer.nodes, vertices, iVertices, rv
    );

    float scrollHeight = y - y0 + 32.0f;

    EXPLORER.scrollData->height = scrollHeight;
    EXPLORER.scrollData->showVerticalScrollbar = scrollHeight > EXPLORER.height;
}

void renameActiveExplorerNode() {
    UIElement* okButton = getElementById(&FLOATING_CONTAINER, RENAME_DIALOG_INPUT_ID);

    if (okButton == nullptr) { return; }

    string newName = okButton->text;

    filesystem::path p = get<1>(getActiveExplorerNodePath(explorer.nodes));

    if (p.empty()) {
        // TODO: show some error like "Some error occured, please retry".
        return;
    }

    p = getVaultPath(selectedVault) / p;

    filesystem::path pNew(p);
    pNew.replace_filename(newName);

    #ifdef DEBUG_EXPLORER
        cout << "\nRenaming path: " << p << " to: " << pNew;
    #endif

    try {
        renameFile(p, pNew);
    } catch (const filesystem::filesystem_error e) {
        // TODO: show some error dialog to user
        cout << "\nERROR: " << e.what();
    }

    thread explorerLoadThread(populateExplorerFromContent);
    unsetExplorerTreeFlag(explorer.nodes, EXPLORER_FLAG_ACTIVE);

    destroyFloatingUI();

    for (ContentTab &tab : tabs) {
        if (tab.filePath == p) {
            tab.filePath = pNew;
            cout << "\nDEBUG: Assigned new tab path";
            REDRAW_UI();
            break;
        }
    }

    explorerLoadThread.join();
}

void deleteActiveExplorerNode() {
    tuple<ExplorerNodeType, filesystem::path> tup = getActiveExplorerNodePath(explorer.nodes);
    ExplorerNodeType ent = get<0>(tup);
    filesystem::path p = get<1>(tup);

    filesystem::path vaultPath = getVaultPath(selectedVault);

    if (p.empty()) {
        #ifdef DEBUG_EXPLORER
        cout << "\nDEBUG: path is empty";
        #endif
        return;
    }

    p = vaultPath / p;

    #ifdef DEBUG_EXPLORER
    cout << "\nDEBUG: deleting" << p;
    #endif

    uint32_t i = 0;
    error_code e;
    bool error = false;
    string errorTitle;
    string errorMessage;

    for (auto itr=tabs.begin(); itr != tabs.end();) {
        bool shouldClose = false;

        if (ent == ExplorerNodeType::Folder) {
            filesystem::path parentPath = (*itr).filePath.parent_path();

            while (parentPath != vaultPath) {
                filesystem::path rel = filesystem::relative(parentPath, p);

                if (!error && rel == ".") {
                    if ((*itr).updated) {
                        errorTitle = "Cannot delete folder!";
                        errorMessage = "Some notes contained in this folder have unsaved changes. Save/discard all unsaved work on these notes before deleting.";
                        activeTabIndex = i;
                        error = true;
                    } else {
                        shouldClose = true;
                    }
                    break;
                }

                parentPath = parentPath.parent_path();
            }
        } else if ((*itr).filePath == p) {
            if (!error && (*itr).updated) {
                activeTabIndex = i;

                if (ent == ExplorerNodeType::Folder) {
                    errorTitle = "Cannot delete folder!";
                    errorMessage = "Some notes contained in this folder have unsaved changes. Save/discard all unsaved work on these notes before deleting.";
                } else {
                    errorTitle = "Cannot delete note!";
                    errorMessage = "Save/discard all unsaved work in this note before deleting.";
                }
            } else {
                shouldClose = true;
            }
        }

        if (shouldClose) {
            closeTab(i);

            if (tabs.empty()) {
                break;
            }

            itr = tabs.begin() + i;
            continue;
        }

        ++itr;
        ++i;
    }

    if (error) {
        showErrorDialog(errorTitle, errorMessage);
        return;
    }

    if (!tabs.empty() && activeTabIndex > 0 && activeTabIndex >= tabs.size()) {
        activeTabIndex = tabs.size() - 1;
    }

    deleteNode(p);

    REDRAW_UI();

    thread explorerLoadThread(populateExplorerFromContent);

    unsetExplorerTreeFlag(explorer.nodes, EXPLORER_FLAG_ACTIVE);

    destroyFloatingUI();

    explorerLoadThread.join();
}

void createNoteInActiveExplorerNode() {
    UIElement* elem = getElementById(&FLOATING_CONTAINER, NEW_NOTE_DIALOG_INPUT_ID);

    if (elem == nullptr) { return; }

    tuple<ExplorerNodeType, filesystem::path> tup = getActiveExplorerNodePath(explorer.nodes);
    ExplorerNodeType ent = get<0>(tup);
    filesystem::path p = get<1>(tup);

    if (p.empty()) {
        p = getVaultPath(selectedVault) / elem->text;
    } else {
        if (ent == ExplorerNodeType::Note) {
            p = p.parent_path();
        }

        p = getVaultPath(selectedVault) / p / elem->text;
    }

    createNote(p);
    newNote(p);

    thread explorerLoadThread(populateExplorerFromContent);

    unsetExplorerTreeFlag(explorer.nodes, EXPLORER_FLAG_ACTIVE);
    destroyFloatingUI();
    REDRAW_UI();

    explorerLoadThread.join();
}

void createFolderInActiveExplorerNode() {
    UIElement* elem = getElementById(&FLOATING_CONTAINER, NEW_FOLDER_DIALOG_INPUT_ID);

    tuple<ExplorerNodeType, filesystem::path> tup = getActiveExplorerNodePath(explorer.nodes);
    ExplorerNodeType ent = get<0>(tup);
    filesystem::path p = get<1>(tup);

    if (p.empty()) {
        if (elem == nullptr) {
            if (untitledFolders == 0) {
                p = getVaultPath(selectedVault) / "Untitled Folder";
            } else {
                p = getVaultPath(selectedVault) / ("Untitled Folder " + to_string(untitledFolders));
            }

            ++untitledFolders;
        } else {
            p = getVaultPath(selectedVault) / elem->text;
        }
    } else {
        if (ent == ExplorerNodeType::Note) {
            p = p.parent_path();
        }

        p = getVaultPath(selectedVault) / p / elem->text;
    }

    createFolder(p);

    thread explorerLoadThread(populateExplorerFromContent);

    unsetExplorerTreeFlag(explorer.nodes, EXPLORER_FLAG_ACTIVE);
    destroyFloatingUI();
    REDRAW_UI();

    explorerLoadThread.join();
}

void setupUBO() {
    glm::vec4 colors[] = {
        TRANSPARENT_COLOR,
        WHITE,
        BLACK,
        OVERLAY_COLOR,
        CODE_COLOR,
        CODE_COLOR_DARK,
        LINK_COLOR,
        LINK_COLOR_DARK,
        LIGHT_TEXT_COLOR,
        LIGHT_TEXT_COLOR_DARK,
        TOOL_BAR_BG,
        TOOL_BAR_BG_DARK,
        CURSOR_COLOR,
        CURSOR_COLOR_DARK,
        SELECT_COLOR,
        SELECT_COLOR_DARK,
        TEXT_INPUT_BG,
        TEXT_INPUT_BG_DARK,
        TEXT_INPUT_HOVER_BG,
        TEXT_INPUT_HOVER_BG_DARK,
        ROOT_BG,
        ROOT_BG_DARK,
        EXPLORER_HOVER_BG,
        EXPLORER_HOVER_BG_DARK,
        EXPLORER_LINE_COLOR,
        EXPLORER_LINE_COLOR_DARK,
        EXPLORER_BG,
        EXPLORER_BG_DARK,
        SCROLL_AREA_BG,
        SCROLL_AREA_BG_DARK,
        CM_DELETE_BUTTON_BG,
        CM_DELETE_BUTTON_BG_DARK,
        CONTENT_HIGHLIGHT_BG,
        CONTENT_HIGHLIGHT_BG_DARK,
        CONTENT_HIGHLIGHT_DRAG,
        CONTENT_HIGHLIGHT_DRAG_DARK,
    };

    CommonUniforms c = { resolution, darkTheme };

    glGenBuffers(1, &colorUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, colorUBO);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(colors), colors, GL_STATIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, COLORS_UBO_INDEX, colorUBO);

    glGenBuffers(1, &commonUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, commonUBO);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(c), &c, GL_STATIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, COMMON_UNIFORMS_UBO_INDEX, commonUBO);
}

void setupUI(bool isDarkTheme, Arena& arena, uint8_t* pngData) {
    if (darkTheme) {
        info("Setting dark theme");
    } else {
        info("Setting light theme");
    }

    windowHeight = WINDOW_HEIGHT;
    windowWidth = WINDOW_WIDTH;
    resolution = glm::vec2(windowWidth, windowHeight);

    textCursor = glfwCreateStandardCursor(GLFW_IBEAM_CURSOR);
    pointerCursor = glfwCreateStandardCursor(GLFW_HAND_CURSOR);
    hResizeCursor = glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR);

    grInfo = { 0 };
    grInfo.activeLayer = 0;
    grInfo.redraw = true;
    REDRAW_UI();
    grInfo.imagePBOBufferSize = CONTENT_MAX_WIDTH * CONTENT_IMAGE_MAX_HEIGHT * 3;
    grInfo.image.numVertices = 64;
    grInfo.image.bufferSize = 1024;

    gmei.hover = false;
    gmei.mouseDown = false;
    uiMouseEvents.hover = false;
    uiMouseEvents.mouseDown = false;

    initShaders();

    glEnable(GL_BLEND);
    glEnable(GL_SAMPLE_SHADING);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    setupUBO();

    SETUP_BUFFERS_RECT(grInfo.rect.vao, grInfo.rect.vbo, NULL, 6 * sizeof(RectVertex));
    grInfo.rect.numVertices = 6;
    grInfo.rect.bufferSize = 6 * sizeof(RectVertex);

    SETUP_BUFFERS_RECT(grInfo.uiRectLayer0.vao, grInfo.uiRectLayer0.vbo, NULL, 60 * sizeof(RectVertex));
    grInfo.uiRectLayer0.numVertices = 60;
    grInfo.uiRectLayer0.bufferSize = 60 * sizeof(RectVertex);

    SETUP_BUFFERS_RECT(grInfo.uiRectLayer1.vao, grInfo.uiRectLayer1.vbo, NULL, 60 * sizeof(RectVertex));
    grInfo.uiRectLayer1.numVertices = 60;
    grInfo.uiRectLayer1.bufferSize = 60 * sizeof(RectVertex);

    float vertices[8] = { 0 };
    float vertices_6[6] = { 0 };
    float vertices_24[24] = { 0 };
    float vertices_2400[2400] = { 0 };
    float vertices_12k[12000] = { 0 };
    TextVertex textVertices[6] = { 0 };
    TextVertex textVertices_10[60] = { 0 };
    TextVertex textVertices_100[600] = { 0 };

    SETUP_BUFFERS_RECT(globalCursor.vao, globalCursor.vbo, NULL, 4 * sizeof(RectVertex));
    SETUP_BUFFERS_RECT(globalCursor.selection.vao, globalCursor.selection.vbo, NULL, 6 * sizeof(RectVertex));
    globalCursor.selection.numVertices = 6;
    globalCursor.selection.bufferSize = 6 * sizeof(RectVertex);

    initElement(ROOT_CONTAINER, UIElementType::Container, 0.0f, 0.0f, WINDOW_WIDTH, WINDOW_HEIGHT);
    initElement(CONTENT_CONTAINER, UIElementType::ContentContainer, TAB_BAR_HEIGHT, 0.0f, WINDOW_WIDTH, WINDOW_HEIGHT - 64.0f);
    initElement(TAB_BAR, UIElementType::TabBar, 0.0f, 0.0f, WINDOW_WIDTH, TAB_BAR_HEIGHT);
    initElement(WELCOME_SCREEN, UIElementType::Container, 0.0f, 0.0f, WINDOW_WIDTH, WINDOW_HEIGHT);
    initElement(MAIN_CONTAINER, UIElementType::Container, 0.0f, 0.0f, WINDOW_WIDTH, WINDOW_HEIGHT);
    initElement(WELCOME_MAIN_CONTAINER, UIElementType::Container, 0.0f, 0.0f, WINDOW_WIDTH, WINDOW_HEIGHT);
    initElement(EXPLORER, UIElementType::Explorer, TAB_BAR_HEIGHT, 0.0f, 200.0f, WINDOW_HEIGHT - TAB_BAR_HEIGHT);
    initElement(FLOATING_CONTAINER, UIElementType::Container, 0.0f, 0.0f, 0.0f, 0.0f);

    TAB_BAR.show = false;
    CONTENT_CONTAINER.show = false;
    FLOATING_CONTAINER.show = false;

    SETUP_BUFFERS_TEXT(grInfo.icon.vao, grInfo.icon.vbo, textVertices_100);
    grInfo.icon.numVertices = 600;
    grInfo.icon.bufferSize = sizeof(textVertices_100);

    SETUP_BUFFERS_TEXT(grInfo.text.vao, grInfo.text.vbo, textVertices_100);
    grInfo.text.numVertices = 600;
    grInfo.text.bufferSize = sizeof(textVertices_100);

    SETUP_BUFFERS_TEXT(grInfo.uiTextLayer0.vao, grInfo.uiTextLayer0.vbo, textVertices_100);
    grInfo.uiTextLayer0.numVertices = 600;
    grInfo.uiTextLayer0.bufferSize = sizeof(textVertices_100);

    SETUP_BUFFERS_TEXT(grInfo.uiTextLayer1.vao, grInfo.uiTextLayer1.vbo, textVertices_100);
    grInfo.uiTextLayer1.numVertices = 600;
    grInfo.uiTextLayer1.bufferSize = sizeof(textVertices_100);

    SETUP_BUFFERS_TEXT(grInfo.uiIconLayer.vao, grInfo.uiIconLayer.vbo, textVertices_10);
    grInfo.uiIconLayer.numVertices = 60;
    grInfo.uiIconLayer.bufferSize = sizeof(textVertices_10);

    SETUP_BUFFERS_TEXT(grInfo.extraText.vao, grInfo.extraText.vbo, textVertices_100);
    grInfo.extraText.numVertices = 600;
    grInfo.extraText.bufferSize = sizeof(textVertices_100);

    glGenVertexArrays(1, &grInfo.image.vao);
    glBindVertexArray(grInfo.image.vao);
    glGenBuffers(1, &grInfo.image.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, grInfo.image.vbo);
    glBufferData(GL_ARRAY_BUFFER, grInfo.image.bufferSize, nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    glGenBuffers(2, grInfo.imagePBO);

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, grInfo.imagePBO[0]);
    glBufferData(
        GL_PIXEL_UNPACK_BUFFER,
        grInfo.imagePBOBufferSize,
        nullptr,
        GL_STREAM_DRAW
    );
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, grInfo.imagePBO[1]);
    glBufferData(
        GL_PIXEL_UNPACK_BUFFER,
        grInfo.imagePBOBufferSize,
        nullptr,
        GL_STREAM_DRAW
    );
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &grInfo.imageTexture);
    glBindTexture(GL_TEXTURE_2D, grInfo.imageTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, CONTENT_MAX_WIDTH, CONTENT_IMAGE_MAX_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);

    SETUP_BUFFERS_DYNAMIC(explorer.verticalScrollVao, explorer.verticalScrollVbo, vertices);
    SETUP_BUFFERS_DYNAMIC(explorer.horizontalScrollVao, explorer.horizontalScrollVao, vertices);

    // TODO: when restoring previous session reserve the previous tabs

    ROOT_CONTAINER.text = "ROOT";
    ROOT_CONTAINER.preferences->widthPreference = UISizePreference::Full;
    ROOT_CONTAINER.preferences->heightPreference = UISizePreference::Full;
    ROOT_CONTAINER.preferences->direction = true;

    TAB_BAR.text = "TAB_BAR";
    TAB_BAR.preferences->widthPreference = UISizePreference::Full;
    TAB_BAR.preferences->heightPreference = UISizePreference::Fixed;
    TAB_BAR.preferences->verticalAlign = UIAlignment::Start;
    TAB_BAR.preferences->horizontalAlign = UIAlignment::Center;

    MAIN_CONTAINER.text = "MAIN";
    MAIN_CONTAINER.preferences->widthPreference = UISizePreference::Stretch;
    MAIN_CONTAINER.preferences->heightPreference = UISizePreference::Full;
    MAIN_CONTAINER.preferences->horizontalAlign = UIAlignment::End;
    MAIN_CONTAINER.preferences->direction = false;
    MAIN_CONTAINER.show = false;

    EXPLORER.text = "EXPLORER";
    EXPLORER.show = false;
    EXPLORER.id = EXPLORER_PANE_ID;

    CONTENT_CONTAINER.text = "CONTENT";
    CONTENT_CONTAINER.scrollData->width = WINDOW_WIDTH;
    CONTENT_CONTAINER.scrollData->height = WINDOW_HEIGHT - TAB_BAR_HEIGHT;
    CONTENT_CONTAINER.preferences->widthPreference = UISizePreference::Full;
    CONTENT_CONTAINER.preferences->heightPreference = UISizePreference::Stretch;
    CONTENT_CONTAINER.preferences->verticalAlign = UIAlignment::End;
    CONTENT_CONTAINER.preferences->horizontalAlign = UIAlignment::End;
    CONTENT_CONTAINER.active = false;

    WELCOME_SCREEN.text = "WELCOME";
    WELCOME_SCREEN.preferences->widthPreference = UISizePreference::Stretch;
    WELCOME_SCREEN.preferences->heightPreference = UISizePreference::Full;
    WELCOME_SCREEN.preferences->horizontalAlign = UIAlignment::End;
    WELCOME_SCREEN.preferences->direction = false;

    WELCOME_MAIN_CONTAINER.preferences->verticalAlign = UIAlignment::Center;
    WELCOME_MAIN_CONTAINER.preferences->widthPreference = UISizePreference::Full;
    WELCOME_MAIN_CONTAINER.preferences->heightPreference = UISizePreference::Grow;
    WELCOME_MAIN_CONTAINER.preferences->direction = false;
    WELCOME_MAIN_CONTAINER.text = "WEL_MAIN";

    FLOATING_CONTAINER.text = "FLOATING";
    FLOATING_CONTAINER.positioning = UIPositioning::Absolute;
    FLOATING_CONTAINER.preferences->widthPreference = UISizePreference::Full;
    FLOATING_CONTAINER.preferences->heightPreference = UISizePreference::Full;
    FLOATING_CONTAINER.preferences->verticalAlign = UIAlignment::None;
    FLOATING_CONTAINER.preferences->horizontalAlign = UIAlignment::None;

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

    EXPLORER.preferences->widthPreference = UISizePreference::Fixed;
    EXPLORER.preferences->heightPreference = UISizePreference::Full;

    addChild(&WELCOME_SCREEN, &WELCOME_MAIN_CONTAINER);

    addChild(&MAIN_CONTAINER, &TAB_BAR);
    addChild(&MAIN_CONTAINER, &CONTENT_CONTAINER);

    addChild(&ROOT_CONTAINER, &EXPLORER);
    addChild(&ROOT_CONTAINER, &WELCOME_SCREEN);
    addChild(&ROOT_CONTAINER, &MAIN_CONTAINER);
    addChild(&ROOT_CONTAINER, &FLOATING_CONTAINER);

    initFontTexture(pngData);
    initIconTexture();

    GLFWimage moveCursorImage;
    const GlyphVariant mvIcon = icons[ICON_MOVE];
    moveCursorImage.width = mvIcon.width/2;
    moveCursorImage.height = mvIcon.height/2;
    moveCursorImage.pixels = moveCursorBitmap;

    moveCursor = glfwCreateCursor(&moveCursorImage, mvIcon.width/4, mvIcon.height/4);

    #ifdef DEBUG_CONTENT_BLOCKS
        printContentBlocks(0);
    #endif

    explorerLoadThread.join();
}

void enterRenderLoop() {
    float fpsPrevTime = 0.0f;
    uint16_t fps = 0;
    uint16_t newFps = 0;

    glm::vec4 color = darkTheme ? ROOT_BG_DARK : ROOT_BG;

    glClearColor(color.r, color.g, color.b, 1.0);

    std::jthread removeOldCBImagesWorker(removeOldCBImages);
    #ifdef _WIN32
        SetThreadDescription(
            removeOldCBImagesWorker.native_handle(),
            L"patram: removeOldCBImages Worker"
        );
    #endif

    #ifdef DEBUG_BOUNDS
    initBoundsRendering();
    #endif

    info("Starting UI rendering.");

    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT);

        if (minimized) {
            glfwWaitEventsTimeout(4000.0f);
        }

        if (!focused) {
            glfwWaitEventsTimeout(80.0f);
        }

        render();

        if (showFps) {
            float time = TIME();

            if (time - fpsPrevTime > 1.0f) {
                fpsPrevTime = time;
                fps = newFps;
                newFps = 0;
            } else {
                ++newFps;
            }

            // renderText("FPS: " + to_string(fps), glm::vec2(0.0f, 0.0f));
        }

        #ifdef DEBUG_TEXT
            if (glfwGetTime() - prevTextTime >= 1.0f) {
                cout << "\nDEBUG: Average text render time per second: " << cumulativeTextTime / textTimeCalculated << " milliseconds";

                textTime = 0.0f;
                cumulativeTextTime = 0.0f;
                textTimeCalculated = 0;
                prevTextTime = glfwGetTime();
            }
        #endif

        glfwSwapBuffers(window);
        glfwPollEvents();

        #ifdef DEBUG_TEXT
        debugTextFirstRenderPrint = false;
        #endif
        // glfwSwapInterval(1);
    }

    removeOldCBImagesWorker.request_stop();
    removeOldCBImagesWorker.join();
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

            case GLFW_KEY_ESCAPE: {
                if (activeElement != nullptr && activeElement->type != UIElementType::TextInput) {
                    activeElement->active = false;
                    activeElement = nullptr;
                }

                break;
            }

            case GLFW_KEY_F11: toggleFullscreen(); break;
        }
    }
}

void cbKeyCallback(int key, int action, int mod) {
    if (tabs.empty()) {
        return;
    }

    ContentTab* tab = &tabs[activeTabIndex];

    if (tab->cursorCBIndex >= tab->contentBlocks.size()) { return; }

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

                    grInfo.redraw = true;

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

                    grInfo.redraw = true;

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

                REDRAW_UI();
                recalculateLayout(&MAIN_CONTAINER);
                recalculateContentWidth();
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
                    uint32_t ropeLen = cb->rope->length();

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
                        uint32_t prevCursorPos = CUR_POS;

                        processCBCommand(ContentCommand {
                            CommandType::Backspace,
                            false,
                            0,
                            tab->cursorCBIndex, CUR_POS,
                            string(1, (*cb->rope)[CUR_POS - 1])
                        });

                        if (CUR_POS != 0 && prevCursorPos != ropeLen) {
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
                    tab->selStartCursorPosition = 0;
                    tab->selEndCursorPosition = len;
                    tab->selStartCBIndex = tab->cursorCBIndex;
                    tab->selEndCBIndex = tab->cursorCBIndex;
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
            case GLFW_KEY_F11: toggleFullscreen(); break;

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

                REDRAW_UI();
                break;
            }

            case GLFW_KEY_LEFT:
                if (elem->textInputData != nullptr && CUR_POS != 0) {
                    TextInputData* tid = elem->textInputData;

                    if (shift) {
                        REDRAW_UI();

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
                            globalCursor.showSelection = true;
                        }
                    } else {
                        textInputUnselect(tid);
                    }

                    DECR_CURSOR();
                    UPDATE_CURSOR_TIME();
                    REDRAW_UI();
                }
                break;

            case GLFW_KEY_RIGHT:
                if (elem->textInputData != nullptr && CUR_POS != elem->text.length()) {
                    TextInputData* tid = elem->textInputData;

                    if (shift) {
                        REDRAW_UI();

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
                            globalCursor.showSelection = true;
                        }
                    } else {
                        textInputUnselect(tid);
                    }

                    INCR_CURSOR();
                    UPDATE_CURSOR_TIME();
                    REDRAW_UI();
                }
                break;

            case GLFW_KEY_UP:
                if (elem->type == UIElementType::TextArea) {
                }
                break;

            case GLFW_KEY_DOWN:
                break;

            case GLFW_KEY_KP_ENTER:
            case GLFW_KEY_ENTER:
                #ifdef DEBUG_TEXT_INPUT
                    cout << "\nEnter pressed";
                #endif

                switch (elem->id) {
                    case RENAME_DIALOG_INPUT_ID: renameActiveExplorerNode(); break;
                    case NEW_NOTE_DIALOG_INPUT_ID: createNoteInActiveExplorerNode(); break;
                    case NEW_FOLDER_DIALOG_INPUT_ID: createFolderInActiveExplorerNode(); break;
                }

                if (shift && elem->type == UIElementType::TextArea) {
                    TYPE("\n");
                    INCR_CURSOR();
                }

                REDRAW_UI();
                break;

            case GLFW_KEY_END:
                CUR_POS = static_cast<uint32_t>(elem->text.length());
                UPDATE_CURSOR_TIME();
                REDRAW_UI();
                break;

            case GLFW_KEY_HOME:
                CUR_POS = 0;
                UPDATE_CURSOR_TIME();
                REDRAW_UI();
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
                    REDRAW_UI();
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
                    REDRAW_UI();
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
                    tid->cursorPosition = len;
                    globalCursor.showSelection = true;
                    REDRAW_UI();
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

            case GLFW_KEY_F11: toggleFullscreen(); break;

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

    deleteCommand(oldCommand, elem->textInputData->cmdList.arena == nullptr);

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

            grInfo.redraw = true;

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
        case ContentBlockType::Link:
        case ContentBlockType::Paragraph: return { 4.0f, 4.0f };
        case ContentBlockType::Title: return { 24.0f, 16.0f };
        case ContentBlockType::Heading1: return { 8.0f, 8.0f };
        case ContentBlockType::Heading2: return { 8.0f, 8.0f };
        case ContentBlockType::Heading3: return { 8.0f, 8.0f };
        case ContentBlockType::Heading4: return { 8.0f, 8.0f };
        case ContentBlockType::Heading5:
        case ContentBlockType::Heading6: return { 4.0f, 8.0f };
        case ContentBlockType::Numbering:
        case ContentBlockType::Bullet: return { 4.0f, 0.0f };
        case ContentBlockType::Code: return { 0.0f, 0.0f };

        default: return { 16.0f, 8.0f };
    }
}

SelectionData getCBTextCursor(glm::vec2 &resolution) {
    ContentTab* tab = &tabs[activeTabIndex];
    uint32_t tempTabIndex = tab->cursorCBIndex;

    if (tempTabIndex >= tab->contentBlocks.size()) {
        tempTabIndex = tab->contentBlocks.size() - 1;
    }

    ContentBlock* cb = &tab->contentBlocks[tempTabIndex];

    RopeTraverser rt(cb->rope);

    uint8_t fontSize = getCBFontSize(cb->type);

    float x = contentLeft;
    float y = (*cb).top;

    if (tempTabIndex > 0 && y == 0) {
        ContentBlock* prev = &tab->contentBlocks[tempTabIndex - 1];
        y = prev->top + getCBVerticalMargins(prev->type).y;
    }

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
            .cursorCBIndex = tab->cursorCBIndex,
            .x = xpos,
            .y = yNet + static_cast<float>(fontSize) - h,
            .height = height,
        };
    }

    while (rt.hasNext()) {
        Rope* r = rt.next();

        for (const char c : r->text) {

            float _tx1, _tx2, _ty1, _ty2, _h, _xw, _yh, advance;
            float glyphHeight, glyphWidth, bearingTop, bearingLeft;
            bool _;
            glm::vec2 pos = glm::vec2(x, y);

            getCharParams(
                characters[c >> 8]->glyphs[0][static_cast<uint8_t>(c)],
                x,
                y,
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
                    .cursorCBIndex = tab->cursorCBIndex,
                    .x = xpos,
                    .y = ypos + bearingTop - glyphHeight - h,
                    .height = height,
                };
            }

            ++i;
            xpos += advance;
        }
    }

    return SelectionData {
        .cursorPosition = i,
        .row = row,
        .col = col,
        .cursorCBIndex = tab->cursorCBIndex,
        .x = xpos,
        .y = yNet + static_cast<float>(fontSize) - h,
        .height = height,
    };
}

SelectionData getCBTextCursorFromScreenCoords(float mouseX, float mouseY, glm::vec2 resolution) {
    ContentTab* tab = &tabs[activeTabIndex];
    ContentBlock* cb = &tab->contentBlocks[tab->activeCBIndex];

    RopeTraverser rt(cb->rope);

    float fontSize = static_cast<float>(getCBFontSize(cb->type));

    float x = contentLeft;
    float y = (*cb).top - tab->scrollOffsetY;

    float xpos = x;
    float ypos = -1.0f;
    float yNet = y;
    float xprev = x;

    uint32_t index = 0;
    uint32_t row = 0;
    uint32_t col = 0;
    float h = fontSize * glyphDescender;
    float height = fontSize - fontSize * glyphDescender;

    uint32_t prevRow = row;
    uint32_t prevCol = col;
    float yposPrev = ypos;
    float glyphHeight = -1000000.0f, glyphWidth, bearingLeft, bearingTop = -1000000.0f;
    float _x1, _x2, _y1, _y2, xpos_plus_w, ypos_plus_h, advance;
    bool _;

    while (rt.hasNext()) {
        Rope* r = rt.next();
        uint32_t len = r->text.length();

        for (uint32_t i=0; i<len; ++i) {
            const char c = r->text[i];

            glm::vec2 pos = glm::vec2(x, y);

            prevRow = row;
            prevCol = col;
            yposPrev = ypos;

            getCharParams(
                characters[c >> 8]->glyphs[0][static_cast<uint8_t>(c)],
                x,
                y,
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
                && ypos >= mouseY + tab->scrollOffsetY && ypos_plus_h <= mouseY + tab->scrollOffsetY
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
                    .y = ypos + bearingTop - glyphHeight - h + tab->scrollOffsetY,
                    .height = static_cast<uint16_t>(fontSize - h),
                };
            } else if (
                xpos + (glyphWidth/2) <= mouseX && xpos_plus_w >= mouseX
                && ypos >= mouseY + tab->scrollOffsetY && ypos_plus_h <= mouseY + tab->scrollOffsetY
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
                    .y = ypos + bearingTop - glyphHeight - h + tab->scrollOffsetY,
                    .height = static_cast<uint16_t>(fontSize - h),
                };
            } else if (xpos_plus_w >= mouseX && ypos >= mouseY + tab->scrollOffsetY && ypos_plus_h <= mouseY + tab->scrollOffsetY) {
                return SelectionData {
                    .cursorPosition = index,
                    .row = row,
                    .col = col,
                    .cursorCBIndex = tab->activeCBIndex,
                    .x = xpos,
                    .y = ypos + bearingTop - glyphHeight - h + tab->scrollOffsetY,
                    .height = static_cast<uint16_t>(fontSize - h),
                };
            } else if (!rt.hasNext() && i == len - 1 && mouseX > xpos_plus_w) {
                return SelectionData {
                    .cursorPosition = index + 1,
                    .row = row,
                    .col = col + 1,
                    .cursorCBIndex = tab->activeCBIndex,
                    .x = xpos_plus_w,
                    .y = ypos + bearingTop - glyphHeight - h + tab->scrollOffsetY,
                    .height = static_cast<uint16_t>(fontSize - h),
                };
            } else if (yNet > mouseY + tab->scrollOffsetY && xprev < mouseX) {
                return SelectionData {
                    .cursorPosition = i!=0 && r->text[i-1] == ' ' ? index - 1 : index,
                    .row = prevRow,
                    .col = prevCol,
                    .cursorCBIndex = tab->activeCBIndex,
                    .x = xprev,
                    .y = yposPrev + bearingTop - glyphHeight - h + tab->scrollOffsetY,
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
        0,
        prevRow,
        prevCol,
        tab->activeCBIndex,
        xprev,
        yposPrev + bearingTop - glyphHeight - h + tab->scrollOffsetY,
        static_cast<uint16_t>(fontSize - h),
        SelectionCBPosition::None
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
        static_cast<float>(tab->cursorX),
        static_cast<float>(tab->cursorY),
        0,
        SelectionCBPosition::Current,
    };

    ContentBlock* cb = &tab->contentBlocks[tab->cursorCBIndex];

    if (page && !up) {
        sd.row = cb->rows - 1;
    }

    if (DOWN_AND_BOTTOM_ROW && tab->cursorCBIndex == tab->contentBlocks.size() - 1) {
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
    float y = (*cb).top;// - tab->scrollOffsetY;

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

            getCharParams(
                characters[c >> 8]->glyphs[0][static_cast<uint8_t>(c)],
                x,
                y,
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
                    << " bearingTop: " << bearingTop;
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

    if (cb->rope->length() == 0) {
        foundRow = true;
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
        uint32_t i = 0;

        for (auto itr = tabs.begin(); itr != tabs.end(); ++itr, ++i) {
            if ((*itr).filePath == path) {
                activeTabIndex = i;
                REDRAW_UI();
                return;
            }
        }
        vector<string> lines = readFileText(path);

        if (lines.empty()) {
            newNote(path);
        } else {
            newTab(path);

            ContentTab* tab = &tabs[activeTabIndex];

            tab->filePath = path;

            for (string line: lines) {
                tab->contentBlocks.emplace_back(ContentBlock {
                    ContentBlockType::Paragraph,
                    CONTENT_BLOCK_VISIBLE, 0,
                    0.0f, 0.0f, 0.0f, 1, 0, new Rope(line),
                });
            }

            #ifdef DEBUG_CONTENT_BLOCKS
                cout << "\nRead following content from file ("
                    << tab->contentBlocks.size() << "): ";

                printContentBlocksFromVector(tab->contentBlocks);
            #endif
        }

        updateWindowTitle();

        TAB_BAR.show = true;
        CONTENT_CONTAINER.show = true;
        REDRAW_UI();
        recalculateLayout(&ROOT_CONTAINER);
        recalculateContentWidth();
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

    for (auto itr = tab->contentBlocks.begin() + 1; itr != tab->contentBlocks.end(); ++itr) {
        if (itr != tab->contentBlocks.begin() + 1) {
            fout << "\n";
        }

        fout << (*itr).rope->getString();
    }

    fout.close();

    tab->updated = false;
    REDRAW_UI();
}

void newNote(filesystem::path p) {
    if (WELCOME_MAIN_CONTAINER.totalChildren) {
        destroyWelcomeScreen();
    }

    if (p.empty()) {
        string title;

        if (untitledNotes > 0) {
            title = "Untitled Note " + to_string(untitledNotes);
        } else {
            title = "Untitled Note";
        }

        ++untitledNotes;
        newTab(getVaultPath(selectedVault) / title);
    } else {
        newTab(p);
    }

    updateWindowTitle();

    ContentTab* tab = &tabs[activeTabIndex];

    tab->contentBlocks.emplace_back(ContentBlock {
        ContentBlockType::Paragraph,
        CONTENT_BLOCK_VISIBLE, 0,
        0.0f, 0.0f, 0.0f, 1, 0, new Rope()
    });
    tab->updated = true;
    REDRAW_UI();

    TAB_BAR.show = true;
    CONTENT_CONTAINER.show = true;
    recalculateLayout(&ROOT_CONTAINER);
    recalculateContentWidth();
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
    float x = elem->left + BUTTON_PADDING;
    float y = elem->top + BUTTON_PADDING;

    if (elem->textInputData->alignment != TextInputAlignment::Left) {
        RopeRenderResult rRes = getTextDimensions(
            elem->text,
            x, y, 0,
            elem->fontSize,
            elem->parent->left,
            elem->parent->width
        );

        switch (elem->textInputData->alignment) {
            case TextInputAlignment::Center:
                x = elem->left + (elem->width - rRes.width)/2;
                y = elem->top + TEXT_INPUT_PADDING;
                break;

            case TextInputAlignment::Right:
                x = elem->left + elem->width - rRes.width;
                y = elem->top + TEXT_INPUT_PADDING;
                break;
        }
    }

    // cout << "\nMouse x: " << mouseX << " y: " << mouseY;

    float xpos = x;
    float ypos = -1.0f;
    float yNet = y;

    uint32_t index = 0;
    uint32_t row = 0;
    uint32_t col = 0;
    float height = elem->fontSize - elem->fontSize * glyphDescender;

    uint32_t prevRow = row;
    uint32_t prevCol = col;
    float yposPrev = ypos;
    float glyphHeight = -1000000.0f, glyphWidth, bearingLeft, bearingTop = -1000000.0f;
    float _x1, _x2, _y1, _y2, xpos_plus_w, ypos_plus_h, advance;
    bool _;

    for (const char c: elem->text) {
        prevRow = row;
        prevCol = col;
        yposPrev = ypos;

        getCharParams(
            characters[c >> 8]->glyphs[0][static_cast<uint8_t>(c)],
            x,
            y,
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
            elem->fontSize,
            _
        );

        if (xpos + glyphWidth / 2 >= mouseX && yNet + glyphHeight >= mouseY) {
            break;
        }

        xpos += advance;

        ++index;
        ++col;
    }

    return { .cursorPosition = index, .row = row, .col = col };
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
    globalCursor.time = TIME();
    REDRAW_UI();
}

void tabBarMouseDownCallback(
    UIElement* elem,
    float mouseX,
    float mouseY,
    glm::vec2 res,
    int glfwButton
) {
    float totalWidth = elem->left + tabsDisplay.totalX + 2 * TAB_BAR_HEIGHT;

    if (totalWidth > windowWidth) {
        if (mouseX > windowWidth - TAB_BAR_HEIGHT) {

            if (totalWidth - tabsDisplay.xOffset > windowWidth) {
                float newOffset = tabsDisplay.xOffset + 160.0f;

                if (totalWidth - newOffset < windowWidth) {
                    newOffset = totalWidth - windowWidth;
                }

                tabsDisplay.xOffset = newOffset;

                REDRAW_UI();
            }

            return;
        } else if (mouseX > windowWidth - 2 * TAB_BAR_HEIGHT) {
            tabsDisplay.xOffset -= 160.0f;

            if (tabsDisplay.xOffset < 0.0f) {
                tabsDisplay.xOffset = 0.0f;
            }

            REDRAW_UI();
            return;
        }
    }

    for (int i=0; i<tabs.size(); ++i) {
        float x = static_cast<float>(tabs[i].xOffset + tabs[i].width) - tabsDisplay.xOffset;

        if (mouseX <= x) {
            if (
                glfwButton == GLFW_MOUSE_BUTTON_3
                || (
                    mouseX > static_cast<float>(tabs[i].xOffset + tabs[i].closeX - 5) - tabsDisplay.xOffset
                    && mouseX < x - 5
                )
            ) {
                closeTab(i);
            } else {
                activeTabIndex = i;
                REDRAW_UI();
                TAB_BAR.active = false;

                if (activeElement != nullptr) {
                    activeElement->active = false;
                }
                activeElement = &CONTENT_CONTAINER;
                CONTENT_CONTAINER.active = true;

                ContentTab* tab = CURRENT_TAB;

                tab->redrawCursor = true;
                REDRAW_CB();
            }

            break;
        }
    }

    REDRAW_UI();

    updateWindowTitle();
}

void textInputMouseDragCallback(UIElement* elem, float mouseX, float mouseY, glm::vec2 res) {
    if (elem->textInputData == nullptr) { return; }

    #ifdef DEBUG_TEXT_SELECTION
        debug("Drag Callback");
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

        globalCursor.showSelection = true;
        REDRAW_UI();

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
        REDRAW_UI();

        #ifdef DEBUG_TEXT_SELECTION
            printTextInputData(tid, "Text Selection 2");
        #endif
    } else {
        #ifdef DEBUG_TEXT_SELECTION
            cout << "\nMarked selected to false";
        #endif
        textInputUnselect(tid, "Text Input Mouse Drag Callback 2");
    }

    globalCursor.time = TIME();
    REDRAW_UI();
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

void textInputUnselect(TextInputData * tid, string token) {
    if (tid == nullptr) { return; }

    #ifdef DEBUG_TEXT_SELECTION
        cout << "\nMarked selected to false";
        if (!token.empty()) {
            cout << " (" << token << ")";
        }
    #endif

    tid->textSelected = false;
    tid->cursorPosition = tid->textSelectStartCol;
    tid->textSelectStartCol = tid->textSelectStartCol;
    tid->textSelectEndCol = tid->textSelectStartCol;
    tid->initialCursorPosition = tid->textSelectStartCol;
    tid->textSelectEndRow = tid->textSelectStartRow;
    globalCursor.showSelection = false;
    REDRAW_UI();
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

bool updateMouseHoverEvents(float x, float y, MouseEventInfo& mei) {
    uint32_t boundsIndex = 0;
    bool hover = false;

    for (
        vector<Bounds>::iterator b = mei.bounds.begin();
        b != mei.bounds.end();
        ++b, ++boundsIndex
    ) {
        if (
            b->enableHover
            && x > b->x1 && x < b->x2
            && y > b->y1 && y < b->y2
        ) {
            hover = true;
            break;
        }
    }

    if (hover != mei.hover || mei.hoverIndex != boundsIndex) {
        mei.hover = hover;
        mei.hoverIndex = boundsIndex;

        if (hover) {
            mei.hoverBounds = mei.bounds[mei.hoverIndex];
        } else {
            mei.hoverBounds = Bounds { 0 };
        }

        return true;
    }

    return false;
}

bool updateMouseDownEvents(float x, float y, MouseEventInfo& mei) {
    uint32_t boundsIndex = 0;
    bool updated = false;
    bool mouseDown = false;

    for (
        vector<Bounds>::iterator b = mei.bounds.begin();
        b != mei.bounds.end();
        ++b, ++boundsIndex
    ) {
        if (
            b->enableClick
            && mouseX > b->x1 && mouseX < b->x2
            && mouseY > b->y1 && mouseY < b->y2
        ) {
            mouseDown = true;
            break;
        }
    }

    if (mouseDown != mei.mouseDown || boundsIndex != mei.mouseDownIndex) {
        mei.mouseDown = mouseDown;
        mei.mouseDownIndex = boundsIndex;

        if (mouseDown) {
            mei.clickBounds = mei.bounds[mei.mouseDownIndex];
        }

        return true;
    }

    return false;
}

void setTooltipDirection(
    float bbx1,
    float bby1,
    float bbx2,
    float bby2,
    bool showArrow,
    RectVertex* arrow,
    uint32_t colorIndex,
    TooltipDirection direction
) {
    tooltip.direction = direction;
    switch (direction) {
        case TooltipDirection::Top:
            tooltip.arrowX = (bbx1 + bbx2) / 2;
            tooltip.arrowY = bby1;
            tooltip.x = tooltip.arrowX - tooltip.width / 2;
            tooltip.y = bby1 - TOOLTIP_ARROW_WIDTH - tooltip.height;

            if (showArrow) {
                arrow[0] = { tooltip.arrowX, tooltip.arrowY, 0, 0, 0, 0, colorIndex, 0 };
                arrow[1] = { tooltip.arrowX - TOOLTIP_ARROW_WIDTH, tooltip.arrowY - TOOLTIP_ARROW_WIDTH, 0, 0, 0, 0, colorIndex, 0 };
                arrow[2] = { tooltip.arrowX + TOOLTIP_ARROW_WIDTH, tooltip.arrowY - TOOLTIP_ARROW_WIDTH, 0, 0, 0, 0, colorIndex, 0 };
            }
            break;

        case TooltipDirection::Right:
            tooltip.arrowX = bbx2;
            tooltip.arrowY = (bby1 + bby2) / 2;
            tooltip.x = bbx2 + TOOLTIP_ARROW_WIDTH;
            tooltip.y = tooltip.arrowY - tooltip.height / 2;

            if (showArrow) {
                arrow[0] = { tooltip.arrowX, tooltip.arrowY, 0, 0, 0, 0, colorIndex, 0 };
                arrow[1] = { tooltip.arrowX + TOOLTIP_ARROW_WIDTH, tooltip.arrowY - TOOLTIP_ARROW_WIDTH, 0, 0, 0, 0, colorIndex, 0 };
                arrow[2] = { tooltip.arrowX + TOOLTIP_ARROW_WIDTH, tooltip.arrowY + TOOLTIP_ARROW_WIDTH, 0, 0, 0, 0, colorIndex, 0 };
            }
            break;

        case TooltipDirection::Bottom:
            tooltip.arrowX = (bbx1 + bby1) / 2;
            tooltip.arrowY = bby2;
            tooltip.x = tooltip.arrowX - tooltip.width / 2;
            tooltip.y = bby2 + TOOLTIP_ARROW_WIDTH;

            if (showArrow) {
                arrow[0] = { tooltip.arrowX, tooltip.arrowY, 0, 0, 0, 0, colorIndex, 0 };
                arrow[1] = { tooltip.arrowX - TOOLTIP_ARROW_WIDTH, tooltip.arrowY + TOOLTIP_ARROW_WIDTH, 0, 0, 0, 0, colorIndex, 0 };
                arrow[2] = { tooltip.arrowX + TOOLTIP_ARROW_WIDTH, tooltip.arrowY + TOOLTIP_ARROW_WIDTH, 0, 0, 0, 0, colorIndex, 0 };
            }
            break;

        case TooltipDirection::Left:
            tooltip.arrowX = bbx1;
            tooltip.arrowY = (bby1 + bby2) / 2;
            tooltip.x = bbx2 - TOOLTIP_ARROW_WIDTH;
            tooltip.y = tooltip.arrowY - tooltip.height / 2;

            if (showArrow) {
                arrow[0] = { tooltip.arrowX, tooltip.arrowY, 0, 0, 0, 0, colorIndex, 0 };
                arrow[1] = { tooltip.arrowX - TOOLTIP_ARROW_WIDTH, tooltip.arrowY - TOOLTIP_ARROW_WIDTH, 0, 0, 0, 0, colorIndex, 0 };
                arrow[2] = { tooltip.arrowX - TOOLTIP_ARROW_WIDTH, tooltip.arrowY + TOOLTIP_ARROW_WIDTH, 0, 0, 0, 0, colorIndex, 0 };
            }
            break;
    }
}

void showTooltip(
    float bbx1, float bby1, float bbx2, float bby2,
    TooltipDirection direction, bool showArrow, string text
) {
    tooltip.show = true;
    tooltip.delayShowing = true;
    tooltip.showArrow = showArrow;
    tooltip.text = text;
    tooltip.time = TIME();
    tooltip.opacity = 0.0f;

    RopeRenderResult r = getTextDimensions(
        text,
        contentLeft, 0, 0,
        16,
        contentLeft,
        contentRight - contentLeft
    );

    tooltip.width = r.width + 2 * TOOLTIP_PADDING;
    tooltip.height = r.height + 2 * TOOLTIP_PADDING;

    uint32_t colorIndex = darkTheme ? BLACK_INDEX : CODE_INDEX;
    uint32_t textVertices = text.length() * 6;
    uint32_t textBufferSize = sizeof(TextVertex) * textVertices;

    RectVertex box[9] = { 0 };

    vector<TextVertex> txt;
    txt.reserve(textVertices);

    setTooltipDirection(bbx1, bby1, bbx2, bby2, showArrow, box, colorIndex, direction);

    if (tooltip.x < 0.0f) {
        tooltip.x = 0.0f;
    }

    if (tooltip.y < 0.0f) {
        if (direction == TooltipDirection::Top) {
            direction = TooltipDirection::Bottom;
        } else {
            direction = TooltipDirection::Top;
        }

        setTooltipDirection(bbx1, bby1, bbx2, bby2, showArrow, box, colorIndex, direction);
    }

    box[3] = { tooltip.x,                 tooltip.y,                  tooltip.x, tooltip.y, tooltip.width, tooltip.height, colorIndex, 3 };
    box[4] = { tooltip.x + tooltip.width, tooltip.y,                  tooltip.x, tooltip.y, tooltip.width, tooltip.height, colorIndex, 3 };
    box[5] = { tooltip.x,                 tooltip.y + tooltip.height, tooltip.x, tooltip.y, tooltip.width, tooltip.height, colorIndex, 3 };
    box[6] = { tooltip.x,                 tooltip.y + tooltip.height, tooltip.x, tooltip.y, tooltip.width, tooltip.height, colorIndex, 3 };
    box[7] = { tooltip.x + tooltip.width, tooltip.y,                  tooltip.x, tooltip.y, tooltip.width, tooltip.height, colorIndex, 3 };
    box[8] = { tooltip.x + tooltip.width, tooltip.y + tooltip.height, tooltip.x, tooltip.y, tooltip.width, tooltip.height, colorIndex, 3 };

    appendTextVertices(
        tooltip.x + TOOLTIP_PADDING,
        tooltip.y + 2*TOOLTIP_PADDING,
        text,
        0,
        darkTheme ? WHITE_INDEX : BLACK_INDEX,
        16,
        txt
    );

    if (tooltip.box.vao == 0) {
        SETUP_BUFFERS_RECT(tooltip.box.vao, tooltip.box.vbo, box, sizeof(box));
    } else {
        glBindVertexArray(tooltip.box.vao);
        glBindBuffer(GL_ARRAY_BUFFER, tooltip.box.vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(box), box);
    }

    if (tooltip.txt.vao == 0) {
        glGenVertexArrays(1, &tooltip.txt.vao);
        glBindVertexArray(tooltip.txt.vao);
        glGenBuffers(1, &tooltip.txt.vbo);
        glBindBuffer(GL_ARRAY_BUFFER, tooltip.txt.vbo);
        glBufferData(GL_ARRAY_BUFFER, textBufferSize, &txt[0], GL_STATIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(TextVertex), (void *)0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(TextVertex), (void *)(2 * sizeof(float)));
        glVertexAttribIPointer(2, 1, GL_UNSIGNED_INT, sizeof(TextVertex), (void *)(4 * sizeof(float)));
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);

        tooltip.txt.numVertices = textVertices;
        tooltip.txt.bufferSize = textBufferSize;
    } else {
        glBindVertexArray(tooltip.txt.vao);
        glBindBuffer(GL_ARRAY_BUFFER, tooltip.txt.vbo);

        if (tooltip.txt.bufferSize != textBufferSize) {
            glBufferData(GL_ARRAY_BUFFER, textBufferSize, &txt[0], GL_STATIC_DRAW);
            tooltip.txt.bufferSize = textBufferSize;
            tooltip.txt.numVertices = textVertices;
        } else {
            glBufferSubData(GL_ARRAY_BUFFER, 0, textBufferSize, &txt[0]);
        }
    }
}

void hideTooltip() {
    tooltip.show = false;
}

void renderTooltip() {
    if (!tooltip.show) { return; }

    float time = TIME();

    if (tooltip.delayShowing) {
        if (time > tooltip.time + TOOLTIP_DELAY_TIME) {
            tooltip.delayShowing = false;
            tooltip.time = time;
            cout << "\nDEBUG: delay time finished!";
        } else {
            return;
        }
    }

    glUseProgram(shaders[SHADER_RECTANGLE_FADEIN]);

    float timeDiff = time - tooltip.time;

    if (timeDiff < 1.0f) {
        glUniform1f(uniforms[U_RECTANGLE_FADEIN_TIME], timeDiff);
    }

    if (tooltip.showArrow) {
        glBindVertexArray(tooltip.box.vao);
        glDrawArrays(GL_TRIANGLES, 0, 9);
    } else {
        glBindVertexArray(tooltip.box.vao);
        glDrawArrays(GL_TRIANGLES, 3, 6);
    }

    vector<TextVertex> _;

    drawTextBuffers(
        tooltip.txt.vao, tooltip.txt.vbo, tooltip.txt.bufferSize,
        tooltip.txt.numVertices, _, 1.0f, false
    );
}

void initElement(
    UIElement& elem, UIElementType type,
    float top, float left, float width, float height,
    Arena* arena
) {
    uint8_t borderRadius = 0;
    UIPositioning positioning = UIPositioning::Absolute;

    uint8_t fontSize = 16;

    UIPreferences* pref = nullptr;
    TextInputData* tid = nullptr;
    ScrollData* sd = nullptr;

    switch (type) {
        case UIElementType::Button: {
            pref = initPreferences(arena);
            borderRadius = 4;
            break;
        }

        case UIElementType::Label:
            pref = initPreferences(arena);
            pref->widthPreference = UISizePreference::Grow;
            break;

        case UIElementType::TabBar:
        case UIElementType::Container:
            pref = initPreferences(arena);
            break;

        case UIElementType::Explorer:
        case UIElementType::ContentContainer:
        case UIElementType::ScrollArea:
            pref = initPreferences(arena);
            pref->widthPreference = UISizePreference::Grow;

            if (arena == nullptr) {
                sd = new ScrollData;
            } else {
                sd = (ScrollData*) arenaAllocate(*arena, sizeof(ScrollData));
            }

            sd->xOffset = 0;
            sd->yOffset = 0;
            sd->width = 0;
            sd->height = 0;
            sd->mouseDownX = 0;
            sd->mouseDownY = 0;
            sd->lastScrolled = 0;
            sd->showVerticalScrollbar = false;
            sd->showHorizontalScrollbar = false;
            sd->renderedUpdate = false;
            sd->mouseDown = false;
            sd->verticalScrollBarVAO = 0;
            sd->verticalScrollBarVBO = 0;
            sd->horizontalScrollBarVAO = 0;
            sd->horizontalScrollBarVBO = 0;
            sd->verticalScrollThumbVAO = 0;
            sd->verticalScrollThumbVBO = 0;
            sd->horizontalScrollThumbVAO = 0;
            sd->horizontalScrollThumbVBO = 0;

            break;

        case UIElementType::TextArea:
        case UIElementType::TextInput:
            pref = initPreferences(arena);
            borderRadius = 4;

            if (arena == nullptr) {
                tid = new TextInputData;
                tid->cmdList = CommandList();
            } else {
                tid = (TextInputData*) arenaAllocate(*arena, sizeof(TextInputData));
                tid->cmdList = CommandList(arena);
            }

            tid->cursorPosition = 0;
            tid->startIndex = 0;
            tid->textSelectStartRow = 0;
            tid->textSelectStartCol = 0;
            tid->textSelectEndRow = 0;
            tid->textSelectEndCol = 0;
            tid->initialCursorPosition = 0;
            tid->initialRow = 0;
            tid->initialCol = 0;
            tid->prevRow = 0;
            tid->textSelected = false;
            tid->mouseDown = false;
            tid->textSelectUpdated = false;
            tid->password = false;
            tid->alignment = TextInputAlignment::Left;
            tid->mouseDownX = 0.0f;
            tid->mouseDownY = 0.0f;
            tid->lastActiveTime = 0.0f;

            break;
    }

    elem = {
        type,

        0,
        0,
        0,

        false,
        true,

        top,
        left,
        width,
        height,

        "",

        nullptr,

        0,
        0,
        nullptr,

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
        CONTENT_BLOCK_VISIBLE, 0,
        0.0f, 0.0f, 0.0f,
        0, 0,
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
        CONTENT_BLOCK_VISIBLE, 0,
        0.0f, 0.0f, 0.0f,
        0, 0,
        new Rope(text),
    });
}

void moveContentBlock(uint32_t tabIndex, uint32_t newIndex, uint32_t oldIndex) {
    if (tabIndex >= tabs.size()) { error("Invalid tab!"); return; }
    if (newIndex == oldIndex || newIndex == 0 || oldIndex == 0) {
        error("Trying to move the title content block");
        return;
    }

    ContentTab* tab = &tabs[tabIndex];
    uint32_t cbLength = tab->contentBlocks.size();

    if (newIndex >= cbLength || oldIndex > cbLength) {
        errorv(
            "Invalid content block move operation! newIndex: %d, oldIndex: %d",
            newIndex,
            oldIndex
        );
        return;
    }

    if (abs(static_cast<int64_t>(newIndex) - static_cast<int64_t>(oldIndex)) == 1) {
        if (oldIndex == cbLength) { return; }

        ContentBlock temp = tab->contentBlocks[newIndex];
        tab->contentBlocks[newIndex] = tab->contentBlocks[oldIndex];
        tab->contentBlocks[oldIndex] = temp;

        #ifdef DEBUG_CONTENT_BLOCKS
            printContentBlocks(activeTabIndex);
        #endif

        return;
    }

    if (newIndex < oldIndex) {
        uint32_t _oldIndex = oldIndex;

        if (oldIndex == cbLength) { --_oldIndex; }

        ContentBlock temp = tab->contentBlocks[newIndex];
        tab->contentBlocks[newIndex] = tab->contentBlocks[_oldIndex];

        for (
            auto itr = tab->contentBlocks.rbegin() + (cbLength - oldIndex);
            itr != tab->contentBlocks.rbegin() + (cbLength - newIndex);
            ++itr
        ) {
            ContentBlock temp2 = *itr;
            *itr = temp;
            temp = temp2;
        }

        #ifdef DEBUG_CONTENT_BLOCKS
            printContentBlocks(activeTabIndex);
        #endif

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

void newTab(filesystem::path path = getVaultPath(selectedVault) / "Untitled Note") {
    vector<ContentBlock> content;
    vector<ContentCommand> commands;
    vector<ContentCommandExt> extCommands;

    commands.reserve(8);

    extCommands.reserve(4);

    content.push_back(ContentBlock {
        ContentBlockType::Title,
        CONTENT_BLOCK_VISIBLE, 0,
        0.0f, 0.0f, 0.0f,
        1, 0,
        new Rope(path.filename().string()),
    });

    uint32_t vao[4];
    uint32_t vbo[4];

    float vertices[] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };

    for (int i=0; i<4; ++i) {
        SETUP_BUFFERS_DYNAMIC(vao[i], vbo[i], vertices);
    }

    commands.push_back(ContentCommand { CommandType::None, false, 0, 0, 0, "", });

    tabs.push_back(ContentTab {
        true, true, false, false, false, false, false, false, false,
        0.0f, 0.0f, 0.0f, 0.0f,
        1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        8, vao[0], vbo[0], vao[1], vbo[1], vao[2], vbo[2], vao[3], vbo[3],
        0, 0, 0, 0, 0, 0,
        commands, extCommands, content, vector<ContentBlockBounds>(), path
    });

    activeTabIndex = tabs.size() - 1;

    WELCOME_SCREEN.show = false;
    MAIN_CONTAINER.show = true;

    if (activeElement != nullptr) {
        activeElement->active = false;
    }
    activeElement = &CONTENT_CONTAINER;
    CONTENT_CONTAINER.active = true;
    REDRAW_UI();
}

void closeCurrentTab() {
    closeTab(activeTabIndex);
}

void adjustTabOffset() {
    tabsDisplay.recalcOffset = false;
    float totalWidth = TAB_BAR.left + tabsDisplay.totalX + 2 * TAB_BAR_HEIGHT;

    if (totalWidth > windowWidth) {
        if (totalWidth - tabsDisplay.xOffset < windowWidth) {
            tabsDisplay.xOffset = totalWidth - windowWidth;
            REDRAW_UI();
        }
        return;
    }

    tabsDisplay.xOffset = 0.0f;
    REDRAW_UI();
}

void closeTab(uint32_t index) {
    ContentTab* tab = &tabs[index];

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
            Rope* r = (*itr).rope;
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

    tabs.erase(tabs.begin() + index);

    if (activeTabIndex >= tabs.size() && activeTabIndex > 0) {
        activeTabIndex = tabs.size() - 1;
    }

    if (tabs.empty()) {
        WELCOME_SCREEN.show = true;
        MAIN_CONTAINER.show = false;
        TAB_BAR.show = false;
        CONTENT_CONTAINER.show = false;
        activeElement = nullptr;
    } else {
        tabsDisplay.recalcOffset = true;
    }

    updateWindowTitle();
    grInfo.redraw = true;
    setupWelcomeScreen();
    REDRAW_UI();
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

UIPreferences* initPreferences(Arena *arena) {
    UIPreferences* pref;

    if (arena == nullptr) {
        pref = new UIPreferences;
    } else {
        pref = (UIPreferences*) arenaAllocate(*arena, sizeof(UIPreferences));
    }

    pref->maxWidth = -1;
    pref->maxHeight = -1;
    pref->direction = true;
    pref->verticalAlign = UIAlignment::Start;
    pref->horizontalAlign = UIAlignment::Start;
    pref->heightPreference = UISizePreference::Fixed;
    pref->widthPreference = UISizePreference::Fixed;

    return pref;
}

void initializeCapacity(UIElement* parent, int capacity, Arena* arena) {
    if (parent->children == nullptr) {
        if (arena == nullptr) {
            parent->children = new UIElement*[capacity];
        } else {
            parent->children = (UIElement**)arenaAllocate(*arena, sizeof(UIElement*) * capacity);
        }
        parent->capacity = capacity;
    }
}

void growCapacity(UIElement* parent, Arena* arena) {
    if (parent->capacity > GROWTH_THRESHOLD) {
        parent->capacity += INCR_POST_GROWTH_THRESHOLD;
    } else {
        parent->capacity *= GROWTH_FACTOR;
    }

    UIElement** newlist;

    if (arena == nullptr) {
        newlist = new UIElement*[parent->capacity];
    } else {
        newlist = (UIElement**) arenaAllocate(*arena, sizeof(UIElement*) * parent->capacity);
    }

    for (uint32_t i=0; i<parent->totalChildren; ++i) {
        newlist[i] = parent->children[i];
    }

    if (arena == nullptr) {
        delete [] parent->children;
    }

    parent->children = newlist;
}

void addChild(UIElement* parent, UIElement * child, Arena* arena) {
    if (child->parent != nullptr) {
        cerr << "\nError: child already has a parent";
    }

    if (parent->capacity == 0) {
        initializeCapacity(parent, INITIAL_PARENT_CAPACITY, arena);
    } else if (parent->totalChildren + 1 > parent->capacity) {
        growCapacity(parent, arena);
    }

    parent->children[parent->totalChildren] = child;
    child->parent = parent;

    ++parent->totalChildren;
}

inline void toggleExplorer() {
    EXPLORER.show = !EXPLORER.show;

    #ifdef DEBUG_EXPLORER
        if (EXPLORER.show) {
            cout << "\nExplorer visible";
        } else {
            cout << "\nExplorer hidden";
        }
    #endif

    if (MAIN_CONTAINER.show) {
        highlightedContentBlock.show = false;
        highlightedContentBlock.updated = true;

        if (CONTENT_CONTAINER.show) {
            CURRENT_TAB->redrawCursor = true;
        }

        recalculateLayout(&MAIN_CONTAINER);
        recalculateContentWidth();
    }

    if (WELCOME_SCREEN.show) {
        recalculateLayout(&WELCOME_SCREEN);
    }

    REDRAW_UI();
}

inline bool updateElementDimensionsAccordingToPreferences(UIElement* elem) {
    bool updated = false;

    float oldLeft = elem->left;
    float oldTop = elem->top;
    float oldWidth = elem->width;
    float oldHeight = elem->height;

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
                                    ++stretchCandidates;
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

                        if (
                            siblingElem != elem
                            && siblingElem->show
                            && siblingElem->preferences != nullptr
                        ) {
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
                    elem->left = elem->parent->left + elem->parent->width - elem->width;
                }
                break;

            case UIAlignment::Center:
                if (elem->parent != nullptr) {
                    elem->left = elem->parent->left + (elem->parent->width - elem->width) / 2;
                }
                break;
        }
    }

    if (
        updated && (
            oldLeft != elem->left || oldTop != elem->top
            || oldWidth != elem->width || oldHeight != elem->height
        )
    ) {
        REDRAW_UI();
        rerenderUI = true;
    }

    return updated;
}

void recalculateLayout(UIElement * elem) {
    REDRAW_UI();

    if (updateElementDimensionsAccordingToPreferences(elem)) {
        #ifdef DEBUG_SCROLL_AREA
            cout << "\n    -> Updated dimensions: width: " << elem->width << ", height: " << elem->height;
        #endif

        if (elem->type == UIElementType::ScrollArea || elem->type == UIElementType::ContentContainer) {
            elem->scrollData->renderedUpdate = false;

            if (!tabs.empty() && !tabs[activeTabIndex].contentBlocks.empty()) {
                REDRAW_CB();

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
        DRAW_RECT(SCROLL_DATA->verticalScrollBarVAO, trackColor);
        DRAW_RECT(SCROLL_DATA->verticalScrollThumbVAO, thumbColor);
    }

    if (SCROLL_DATA->showHorizontalScrollbar) {
        DRAW_RECT(SCROLL_DATA->horizontalScrollBarVAO, trackColor);
        DRAW_RECT(SCROLL_DATA->horizontalScrollThumbVAO, thumbColor);
    }

    SCROLL_DATA->renderedUpdate = true;

    #undef SCROLL_DATA
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

    glm::vec4 color = darkTheme ? SCROLL_BAR_TRACK_COLOR_DARK : SCROLL_BAR_TRACK_COLOR;
    DRAW_RECT(tab->scrollbarVAO, color);

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

    color = darkTheme ? SCROLL_BAR_THUMB_COLOR_DARK: SCROLL_BAR_THUMB_COLOR;
    DRAW_RECT(tab->scrollThumbVAO, color);

    tab->scrollbarUpdated = true;
}

inline void recalculateCBCursor() {
    SelectionData sd = getCBTextCursor(resolution);
    updateCBCursor(CURRENT_TAB, sd);
}

void updateCBCursor(ContentTab* tab, SelectionData sd) {
    tab->showCursor = true;
    tab->redrawCursor = true;

    tab->cursorY = static_cast<uint16_t>(sd.y) + (static_cast<uint16_t>(sd.height) >> 1);

    if (sd.y - tab->scrollOffsetY > CONTENT_CONTAINER.height + CONTENT_CONTAINER.top) {
        tab->scrollOffsetY = sd.y - CONTENT_CONTAINER.height - CONTENT_CONTAINER.top;
        highlightedContentBlock.show = false;
        tab->scrollbarUpdated = true;

        uint32_t startIndex = (static_cast<int64_t>(tab->cursorCBIndex) - 64) > 0 ? tab->cursorCBIndex - 64 : 0;

        REDRAW_CB();
    } else if (sd.y - sd.height - tab->scrollOffsetY < CONTENT_CONTAINER.top && sd.y != 0 && sd.height != 0) {
        tab->scrollOffsetY = sd.y - sd.height - CONTENT_CONTAINER.top;
        if (tab->scrollOffsetY < 0) {
            tab->scrollOffsetY = 0;
        }
        tab->scrollbarUpdated = true;

        uint32_t startIndex = (static_cast<int64_t>(tab->cursorCBIndex) - 64) > 0 ? tab->cursorCBIndex - 64 : 0;

        REDRAW_CB();
    } else if (tab->cursorCBIndex == 0 && tab->scrollOffsetY != 0) {
        tab->scrollOffsetY = 0;
        tab->scrollbarUpdated = true;
        REDRAW_CB();
    }

    if (tab->cursorCBIndex < tab->contentBlocks.size()) {
        grInfo.redraw = true;
    }

    if (sd.cursorCBIndex < tab->contentBlocks.size()) {
        grInfo.redraw = true;
    }

    tab->cursorPosition = sd.cursorPosition;
    tab->lastCursorTime = TIME();
    tab->cursorRow = sd.row;
    tab->cursorCBIndex = sd.cursorCBIndex;
}

void render() {
    glm::vec2 cursor(mouseX, mouseY);
    float _ = 0.0f;

    vector<TextVertex> uiTextLayer0;
    vector<TextVertex> uiTextLayer1;
    vector<TextVertex> uiIconLayer;
    vector<RectVertex> uiRectLayer0;
    vector<RectVertex> uiRectLayer1;

    if (grInfo.redrawUI) {
        uiTextLayer0.reserve(grInfo.uiTextLayer0.numVertices);
        uiTextLayer1.reserve(grInfo.uiTextLayer1.numVertices);
        uiIconLayer.reserve(grInfo.uiIconLayer.numVertices);

        vector<Bounds> temp;
        std::swap(uiMouseEvents.bounds, temp);

        addRectangle(0, 0, 0, 0, 0, 0, uiRectLayer0);
        addRectangle(0, 0, 0, 0, 0, 0, uiRectLayer1);
    }

    renderElement(
        ROOT_CONTAINER,
        resolution,
        cursor,
        0,
        _,
        uiRectLayer0,
        uiRectLayer1,
        uiTextLayer0,
        uiTextLayer1,
        uiIconLayer
    );

    if (grInfo.redrawUI) {
        if (uiIconLayer.empty()) {
            uiIconLayer.insert(uiIconLayer.begin(), {
                TextVertex {0},
                TextVertex {0},
                TextVertex {0},
                TextVertex {0},
                TextVertex {0},
                TextVertex {0}
            });
        }

        if (uiTextLayer0.empty()) {
            uiTextLayer0.insert(uiTextLayer0.begin(), {
                TextVertex {0},
                TextVertex {0},
                TextVertex {0},
                TextVertex {0},
                TextVertex {0},
                TextVertex {0}
            });
        }

        if (uiTextLayer1.empty()) {
            uiTextLayer1.insert(uiTextLayer1.begin(), {
                TextVertex {0},
                TextVertex {0},
                TextVertex {0},
                TextVertex {0},
                TextVertex {0},
                TextVertex {0}
            });
        }
    }

    #ifdef DEBUG_RENDER
    if (grInfo.redrawUI) {
        cout << "\nLayer 0";
    }
    #endif

    drawRectangles(grInfo.uiRectLayer0, uiRectLayer0, grInfo.redrawUI);

    if (globalCursor.show && globalCursor.showSelection && globalCursor.layer == 0) {
        renderTextSelection(globalCursor);
    }

    drawTextBuffers(
        grInfo.uiTextLayer0.vao,
        grInfo.uiTextLayer0.vbo,
        grInfo.uiTextLayer0.bufferSize,
        grInfo.uiTextLayer0.numVertices,
        uiTextLayer0,
        1.0f,
        grInfo.redrawUI
    );

    drawTextBuffers(
        grInfo.uiIconLayer.vao,
        grInfo.uiIconLayer.vbo,
        grInfo.uiIconLayer.bufferSize,
        grInfo.uiIconLayer.numVertices,
        uiIconLayer,
        1.0f,
        grInfo.redrawUI,
        iconTextureID
    );

    #ifdef DEBUG_RENDER
    if (grInfo.redrawUI) {
        cout << "\nLayer 1";
    }
    #endif

    drawRectangles(grInfo.uiRectLayer1, uiRectLayer1, grInfo.redrawUI);

    if (globalCursor.show && globalCursor.showSelection && globalCursor.layer == 1) {
        renderTextSelection(globalCursor);
    }

    drawTextBuffers(
        grInfo.uiTextLayer1.vao,
        grInfo.uiTextLayer1.vbo,
        grInfo.uiTextLayer1.bufferSize,
        grInfo.uiTextLayer1.numVertices,
        uiTextLayer1,
        1.0f,
        grInfo.redrawUI
    );

    if (tooltip.show) {
        renderTooltip();
    }

    if (globalCursor.show) {
        renderTextCursor(globalCursor);
    }

    if (!rerenderUI) {
        grInfo.redrawUI = false;
    } else {
        rerenderUI = false;
        grInfo.redrawUI = true;
    }

#ifdef DEBUG_BOUNDS
    for (Bounds b: uiMouseEvents.bounds) {
        renderBounds(b);
    }
#endif
}

void updateTextCursor(Cursor& c, float x, float y, float height) {
    uint32_t colorIndex = darkTheme ? CURSOR_COLOR_DARK_INDEX : CURSOR_COLOR_INDEX;

    RectVertex vertices[4] = {
        x - 1, y,          0, 0, 0, 0, colorIndex, 0,
        x + 1, y,          0, 0, 0, 0, colorIndex, 0,
        x - 1, y + height, 0, 0, 0, 0, colorIndex, 0,
        x + 1, y + height, 0, 0, 0, 0, colorIndex, 0,
    };

    glBindVertexArray(c.vao);
    glBindBuffer(GL_ARRAY_BUFFER, c.vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
}

void updateTextSelection(Cursor& c, vector<RectVertex> vertices) {
    glBindVertexArray(c.selection.vao);
    glBindBuffer(GL_ARRAY_BUFFER, c.selection.vbo);

    if (vertices.empty()) {
        glBufferData(GL_ARRAY_BUFFER, 6 * sizeof(RectVertex), NULL, GL_STATIC_DRAW);
        c.selection.numVertices = 6;
        c.selection.bufferSize = 6 * sizeof(RectVertex);
        return;
    }

    #ifdef DEBUG_RENDER
    printRectVertex(vertices);
    #endif

    if (c.selection.numVertices < vertices.size()) {
        c.selection.numVertices = vertices.size();
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(RectVertex), &vertices[0], GL_DYNAMIC_DRAW);
    } else {
        for (uint32_t i=vertices.size(); i<c.selection.numVertices; i+=6) {
            vertices.insert(vertices.begin(), {
                RectVertex { 0 },
                RectVertex { 0 },
                RectVertex { 0 },
                RectVertex { 0 },
                RectVertex { 0 },
                RectVertex { 0 },
            });
        }

        if (vertices.size() > c.selection.numVertices) {
            c.selection.numVertices = vertices.size();
            c.selection.bufferSize = vertices.size() * sizeof(RectVertex);
        }

        glBufferSubData(GL_ARRAY_BUFFER, 0, c.selection.bufferSize, &vertices[0]);
    }
}

void renderTextCursor(Cursor& c) {
    glBindVertexArray(c.vao);
    glUseProgram(shaders[SHADER_TEXT_CURSOR]);
    glUniform1f(uniforms[U_TEXT_CURSOR_TIME], TIME() - c.time);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void renderTextSelection(Cursor& c) {
    glBindVertexArray(c.selection.vao);
    glUseProgram(shaders[SHADER_RECTANGLE]);
    glDrawArrays(GL_TRIANGLES, 0, c.selection.numVertices);
}

void renderElement(
    UIElement& elem,
    glm::vec2 &resolution,
    glm::vec2 &cursor,
    uint32_t childIndex,
    float& yPrev,
    vector<RectVertex>& uiRectLayer0,
    vector<RectVertex>& uiRectLayer1,
    vector<TextVertex>& uiTextLayer0,
    vector<TextVertex>& uiTextLayer1,
    vector<TextVertex>& uiIconLayer
) {
    switch (elem.type) {
        case UIElementType::Container: {
            if (!elem.show) { break; }

            if (elem.parent != nullptr && elem.id == HORIZONTAL_BUTTON_ROW_ID) {
                if (childIndex > 0) {
                    UIElement* lastVisibleSibling = nullptr;

                    int8_t i = childIndex - 1;

                    while (i > -1) {
                        if (elem.parent->children[i]->show) {
                            lastVisibleSibling = elem.parent->children[i];
                            break;
                        }
                        --i;
                    }

                    elem.top = lastVisibleSibling->top
                        + lastVisibleSibling->height
                        + elem.marginTop;
                }
            }

            if (grInfo.redrawUI) {
                updateElementDimensionsAccordingToPreferences(&elem);
            }

            if (
                &elem == &ROOT_CONTAINER
                || &elem == &WELCOME_SCREEN
                || elem.id == EXPLORER_CONTEXT_MENU_ID
                || elem.id == DIALOG_OVERLAY_ID
                || elem.id == DIALOG_CONTAINER_ID
            ) {
                glm::vec4 bg = darkTheme ? TOOL_BAR_BG_DARK : TOOL_BAR_BG;

                if (elem.id == EXPLORER_CONTEXT_MENU_ID || elem.id == DIALOG_CONTAINER_ID) {
                    if (grInfo.redrawUI) {
                        addRectangle(
                            elem.left, elem.top, elem.width, elem.height,
                            darkTheme ?
                                TOOL_BAR_BG_DARK_INDEX
                                :
                                TOOL_BAR_BG_INDEX,
                            static_cast<float>(elem.borderRadius),
                            uiRectLayer1
                        );
                    }
                } else {
                    bg = TRANSPARENT_COLOR;

                    if (&elem == &ROOT_CONTAINER) {
                        bg = darkTheme ? ROOT_BG_DARK : ROOT_BG;
                    } else if (&elem == &WELCOME_SCREEN) {
                        bg = darkTheme ? TOOL_BAR_BG_DARK : TOOL_BAR_BG;
                    } else if (elem.id == DIALOG_OVERLAY_ID) {
                        if (grInfo.redrawUI) {
                            addRectangle(
                                0, 0, windowWidth, windowHeight,
                                OVERLAY_INDEX,
                                0,
                                uiRectLayer1
                            );
                        }
                    }
                }
            }

            float oldHeight = elem.height;

            if (elem.totalChildren) {
                float yp = 0.0f;

                if (elem.id == HORIZONTAL_BUTTON_ROW_ID) {
                    float left = elem.left;

                    for (uint32_t i=0; i<elem.totalChildren; ++i) {
                        if (elem.children[i]->show) {
                            elem.children[i]->left = left;
                            elem.children[i]->top = elem.top + elem.children[i]->marginTop;

                            renderElement(
                                *elem.children[i],
                                resolution,
                                cursor,
                                i,
                                yp,
                                uiRectLayer0,
                                uiRectLayer1,
                                uiTextLayer0,
                                uiTextLayer1,
                                uiIconLayer
                            );

                            left += elem.children[i]->width + 8.0f;
                        }
                    }
                } else {
                    // REDRAW_UI();

                    for (uint32_t i=0; i<elem.totalChildren; ++i) {
                        if (elem.children[i]->show) {
                            renderElement(
                                *elem.children[i],
                                resolution,
                                cursor,
                                i,
                                yp,
                                uiRectLayer0,
                                uiRectLayer1,
                                uiTextLayer0,
                                uiTextLayer1,
                                uiIconLayer
                            );
                        }
                    }
                }

                if (
                    grInfo.redrawUI
                    && !elem.preferences->direction
                    && elem.preferences->heightPreference == UISizePreference::Grow
                ) {
                    elem.height = elem.children[elem.totalChildren - 1]->top - elem.children[0]->top
                        + elem.children[elem.totalChildren - 1]->height;
                }
            }

            if (oldHeight != elem.height) {
                rerenderUI = true;
                REDRAW_UI();
            }

            break;
        }

        case UIElementType::ContentContainer: {
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

            uint32_t i=0;

            uint32_t offset = 0;

            if (!tab->updated && tab->firstVisibleCBIndex != 0) {
                offset = tab->firstVisibleCBIndex;
                i = offset;
            }

            vector<TextVertex> iconVertices;
            vector<TextVertex> extraTextVertices;
            vector<TextVertex> textVertices;
            vector<RectVertex> rva;

            if (grInfo.redraw) {
                iconVertices.reserve(grInfo.icon.bufferSize > 9600 ? grInfo.icon.bufferSize : 9600);
                extraTextVertices.reserve(grInfo.extraText.bufferSize > 9600 ? grInfo.extraText.bufferSize : 9600);
                textVertices.reserve(grInfo.text.numVertices);
                rva.reserve(grInfo.rect.numVertices);
                cbImageLoader.renderQueue.clear();
                vector<Bounds> temp;
                std::swap(gmei.bounds, temp);

                addRectangle(0, 0, 0, 0, 0, 0, rva);
                appendCharVertices(0, 0, CHAR_EMPTY, 0, iconVertices);

                renderContentBlockHighlight(iconVertices, rva);
            }

            bool visible = false;

            renderCBTextSelection(tab);

            for (
                auto itr = tab->contentBlocks.begin() + offset;
                itr != tab->contentBlocks.end();
                ++itr, ++i
            ) {
                if (renderContentBlock(
                    tab, itr, resolution, cursor, i,
                    iconVertices, extraTextVertices, textVertices, rva
                )) {
                    if (!visible) {
                        tab->firstVisibleCBIndex = i;
                        visible = true;
                    }
                } else if (visible) {
                    #ifdef DEBUG_CONTENT_BLOCKS
                        // cout << "\nDEBUG: breaking render at " << to_string((ContentBlock)(*itr));
                    #endif
                    break;
                }
            }

            drawRectangles(grInfo.rect, rva, grInfo.redraw);

            drawTextBuffers(
                grInfo.icon.vao,
                grInfo.icon.vbo,
                grInfo.icon.bufferSize,
                grInfo.icon.numVertices,
                iconVertices,
                1.0f,
                grInfo.redraw,
                iconTextureID
            );

            drawTextBuffers(
                grInfo.extraText.vao,
                grInfo.extraText.vbo,
                grInfo.extraText.bufferSize,
                grInfo.extraText.numVertices,
                extraTextVertices,
                1.0f,
                grInfo.redraw
            );

            drawTextBuffers(
                grInfo.text.vao,
                grInfo.text.vbo,
                grInfo.text.bufferSize,
                grInfo.text.numVertices,
                textVertices,
                1.0f,
                grInfo.redraw
            );

            renderCBImages();

            grInfo.redraw = false;

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
                    float height;

                    if (tab->cursorCBIndex < tab->contentBlocks.size()) {
                        height = LINE_HEIGHT(getCBFontSize(
                            tab->contentBlocks[tab->cursorCBIndex].type
                        ));
                    } else {
                        height = sd.height;
                    }

                    float vertices[] = {
                        sd.x + 1.0f, sd.y - height - tab->scrollOffsetY,
                        sd.x + 1.0f, sd.y - tab->scrollOffsetY,
                        sd.x - 1.0f, sd.y - height - tab->scrollOffsetY,
                        sd.x - 1.0f, sd.y - tab->scrollOffsetY,
                    };

                    glBindVertexArray(tab->cursorVAO);
                    glBindBuffer(GL_ARRAY_BUFFER, tab->cursorVBO);
                    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
                }

                tab->redrawCursor = false;
            }

            if (tab->showCursor && !tab->textSelected) {
                glBindVertexArray(tab->cursorVAO);

                glUseProgram(shaders[SHADER_TEXT_CURSOR]);

                glm::vec4 color = darkTheme ? CURSOR_COLOR_DARK : CURSOR_COLOR;

                glUniform4f(uniforms[U_TEXT_CURSOR_COLOR], color.x, color.y, color.z, color.w);
                glUniform1f(uniforms[U_TEXT_CURSOR_TIME], TIME() - tab->lastCursorTime);

                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            }

            glDisable(GL_SCISSOR_TEST);

            if (showScrollbar) {
                renderCBScrollbar();
            }
            break;
        }

        case UIElementType::Label: {
            if (!elem.show) { break; }

            float top = elem.top;
            float left = elem.left;

            if (elem.positioning == UIPositioning::Relative) {
                top = elem.parent->top + elem.top;
                left = elem.parent->left + elem.left;
            } else if (
                elem.positioning == UIPositioning::Relative_sibling
                && elem.parent != nullptr
            ) {
                if (childIndex == 0) {
                    top = elem.parent->top + elem.marginTop;
                    left = elem.parent->left;
                } else {
                    if (elem.parent->preferences->direction) {
                        left = elem.parent->left
                            + elem.parent->children[childIndex - 1]->left
                            + elem.parent->children[childIndex - 1]->width;
                        top = elem.parent->top;
                    } else {
                        top = yPrev + elem.marginTop;
                        left = elem.left;
                    }
                }
            }

            if (elem.parent->type == UIElementType::ScrollArea) {
                top -= elem.parent->scrollData->yOffset;
                left -= elem.parent->scrollData->xOffset;
            }

            if (grInfo.redrawUI) {
                RopeRenderResult rRes = getTextDimensions(
                    elem.text, left, top, 0, elem.fontSize,
                    elem.parent->left, elem.parent->width, false
                );

                elem.width = rRes.width;
                elem.height = rRes.height;

                updateElementDimensionsAccordingToPreferences(&elem);

                if (elem.parent != nullptr && !elem.parent->preferences->direction) {
                    left = elem.left;
                }

                if (elem.parent->type == UIElementType::ScrollArea) {
                    if (elem.parent->scrollData->width < left + elem.width) {
                        elem.parent->scrollData->width = left + elem.width;
                    } else if (elem.parent->scrollData->height < top + elem.height) {
                        elem.parent->scrollData->height = top + elem.height;
                    }

                    elem.parent->scrollData->renderedUpdate = false;
                    REDRAW_UI();
                }
            }

            if (grInfo.redrawUI) {
                appendTextVertices(
                    left,
                    top,
                    elem.text,
                    0,
                    darkTheme ? WHITE_INDEX : BLACK_INDEX,
                    elem.fontSize,
                    elem.layer == 0 ? uiTextLayer0 : uiTextLayer1
                );
            }

            if (
                elem.positioning == UIPositioning::Relative_sibling
                && elem.parent != nullptr
            ) {
                elem.top = top;
                yPrev = top + elem.height;
            }
            break;
        }

        // TODO: handle hover
        case UIElementType::Button: {
            if (!elem.show) { return; }

            float top = elem.top;
            float left = elem.left;
            float padding = BUTTON_PADDING;

            if (elem.parent->id == EXPLORER_CONTEXT_MENU_ID) {
                padding = EXPLORER_CONTEXT_MENU_BUTTON_PADDING;
            }

            if (
                elem.positioning == UIPositioning::Relative && elem.parent != nullptr
            ) {
                top = elem.parent->top + elem.marginTop;
            }

            if (
                elem.positioning == UIPositioning::Relative_sibling
                && elem.parent != nullptr
            ) {
                if (childIndex == 0) {
                    top = elem.parent->top + elem.marginTop;
                } else {
                    if (elem.parent->preferences->direction) {
                        left = elem.parent->left
                            + elem.parent->children[childIndex - 1]->left
                            + elem.parent->children[childIndex - 1]->width
                            + elem.left;
                        top = elem.parent->top + elem.marginTop;
                    } else {
                        top = yPrev + elem.marginTop;
                    }
                }
            }

            if (elem.parent->type == UIElementType::ScrollArea) {
                top -= elem.parent->scrollData->yOffset;
            }

            if (grInfo.redrawUI) {
                if (
                    elem.parent->type == UIElementType::ScrollArea
                    && (
                        top < elem.parent->top
                        || top > elem.parent->top + elem.parent->height
                    )
               ) {
                    elem.show = false;
                    return;
                }

                if (
                    elem.preferences->widthPreference == UISizePreference::Grow
                    || elem.preferences->heightPreference == UISizePreference::Grow
                ) {
                    RopeRenderResult rRes = getTextDimensions(
                        elem.text, elem.left + padding, top + padding, 0, elem.fontSize,
                        elem.parent->left, elem.parent->width, false
                    );

                    if (elem.preferences->widthPreference == UISizePreference::Grow) {
                        elem.width = padding + rRes.width + padding;
                    }

                    if (elem.preferences->heightPreference == UISizePreference::Grow) {
                        elem.height = padding + rRes.height + padding;
                    }
                }

                updateElementDimensionsAccordingToPreferences(&elem);

                if (
                    elem.positioning == UIPositioning::Relative_sibling
                    && elem.parent != nullptr
                ) {
                    if (elem.parent->preferences->direction) {
                        elem.left = left;
                    } else {
                        left = elem.left;

                        if (elem.parent->id == EXPLORER_CONTEXT_MENU_ID) {
                            left += EXPLORER_CONTEXT_MENU_PADDING;

                            if (childIndex == 0) {
                                top += EXPLORER_CONTEXT_MENU_PADDING;
                            }

                            elem.width = elem.parent->width - 2 * EXPLORER_CONTEXT_MENU_PADDING;
                        }
                    }
                }

                #ifdef DEBUG_RENDER
                    cout << "\nRe-drawing";
                #endif

                uint32_t colorIndex = darkTheme ? TEXT_INPUT_BG_DARK_INDEX : TEXT_INPUT_BG_INDEX;

                if (uiMouseEvents.hover) {
                    Bounds b = uiMouseEvents.hoverBounds;

                    if (
                        b.type == BoundSourceType::Button
                        && b.source.elem == &elem
                    ) {
                        if (b.source.elem->id == EXPLORER_MENU_DELETE_ID) {
                            colorIndex = darkTheme ? CM_DELETE_BUTTON_BG_DARK_INDEX : CM_DELETE_BUTTON_BG_INDEX;
                        } else {
                            colorIndex = darkTheme ? TEXT_INPUT_HOVER_BG_DARK_INDEX : TEXT_INPUT_HOVER_BG_INDEX;
                        }
                    }
                }

                if (elem.id == SWITCH_VAULT_OPTION_ID) {
                    colorIndex = TRANSPARENT_INDEX;
                }

                uiMouseEvents.bounds.push_back(Bounds {
                    left, top, left + elem.width, top + elem.height,
                    BoundSourceType::Button, true, elem.id != 0,
                    { .elem = &elem }
                });

                addRectangle(
                    left,
                    top,
                    elem.width,
                    elem.height,
                    colorIndex,
                    static_cast<float>(elem.borderRadius),
                    elem.layer == 0 ? uiRectLayer0 : uiRectLayer1
                );

                appendTextVertices(
                    left + padding,
                    top + padding,
                    elem.text,
                    0,
                    darkTheme ? WHITE_INDEX : BLACK_INDEX,
                    elem.fontSize,
                    elem.layer == 0 ? uiTextLayer0 : uiTextLayer1
                );
            } else {
                if (
                    elem.positioning == UIPositioning::Relative_sibling
                    && elem.parent != nullptr
                ) {
                    if (elem.parent->preferences->direction) {
                        elem.left = left;
                    } else {
                        left = elem.left;

                        if (elem.parent->id == EXPLORER_CONTEXT_MENU_ID) {
                            left += EXPLORER_CONTEXT_MENU_PADDING;

                            if (childIndex == 0) {
                                top += EXPLORER_CONTEXT_MENU_PADDING;
                            }

                            elem.width = elem.parent->width - 2 * EXPLORER_CONTEXT_MENU_PADDING;
                        }
                    }
                }
            }

            if (
                elem.positioning == UIPositioning::Relative_sibling
                && elem.parent != nullptr
            ) {
                elem.top = top;
                yPrev = top + elem.height;
            }

            break;
        }

        case UIElementType::ScrollArea: {
            if (elem.totalChildren) {
                float top = elem.top;
                float left = elem.left;
                float width = elem.width;
                float height = elem.height;

                if (grInfo.redrawUI) {
                    if (
                        elem.positioning == UIPositioning::Relative_sibling
                        && elem.parent != nullptr
                    ) {
                        if (childIndex == 0) {
                            top = elem.parent->top + elem.marginTop;
                            left = elem.parent->left + elem.left;
                        } else {
                            if (elem.parent->preferences->direction) {
                                top = elem.top;
                                left = elem.parent->left
                                    + elem.parent->children[childIndex - 1]->left
                                    + elem.parent->children[childIndex - 1]->width
                                    + elem.left;
                            } else {
                                top = yPrev + elem.marginTop;
                                left = elem.left;
                            }
                        }
                    }
                }

                updateElementDimensionsAccordingToPreferences(&elem);

                if (
                    elem.positioning == UIPositioning::Relative_sibling
                    && elem.parent != nullptr
                ) {
                    if (!elem.parent->preferences->direction) {
                        left = elem.left;
                    } else {
                        elem.left = left;
                    }
                }

                if (elem.scrollData->showVerticalScrollbar) {
                    width -= SCROLL_BAR_THICKNESS;
                }

                if (elem.scrollData->showHorizontalScrollbar) {
                    height -= SCROLL_BAR_THICKNESS;
                }

                glScissor(left, resolution.y - top - elem.height, width, height);
                glEnable(GL_SCISSOR_TEST);

                float yp = 0.0f;
                elem.top = top;

                for (uint32_t i=0; i<elem.totalChildren; ++i) {
                    if (elem.children[i]->show || grInfo.redrawUI) {
                        renderElement(
                            *elem.children[i],
                            resolution,
                            cursor,
                            i,
                            yp,
                            uiRectLayer0,
                            uiRectLayer1,
                            uiTextLayer0,
                            uiTextLayer1,
                            uiIconLayer
                        );
                    }

                    if (grInfo.redrawUI) {
                        elem.scrollData->height += elem.children[i]->height;
                    }
                }

                glDisable(GL_SCISSOR_TEST);

                if (grInfo.redrawUI) {
                    elem.scrollData->showVerticalScrollbar = elem.scrollData->height > elem.height;
                    elem.scrollData->showHorizontalScrollbar = elem.scrollData->width > elem.width;

                    #ifdef DEBUG_SCROLL_AREA
                        cout << "\nShow vertical scrollbar: " << elem.scrollData->showVerticalScrollbar;
                        cout << "\nShow horizontal scrollbar: " << elem.scrollData->showHorizontalScrollbar;
                    #endif

                    elem.scrollData->height = 0.0f;
                }

                renderScrollbars(&elem);

                if (elem.positioning == UIPositioning::Relative_sibling) {
                    yPrev = top + elem.height;
                }

                if (grInfo.redrawUI) {
                    addRectangle(
                        elem.left, elem.top, elem.width, elem.height,
                        darkTheme ? SCROLL_AREA_BG_DARK_INDEX : SCROLL_AREA_BG_INDEX,
                        4.0f, uiRectLayer0
                    );
                }
            }
            break;
        }

        case UIElementType::IconButton: {
            uint32_t colorIndex = darkTheme ? TEXT_INPUT_BG_DARK_INDEX : TEXT_INPUT_BG_INDEX;
            uint32_t textColorIndex = darkTheme ? WHITE_INDEX : BLACK_INDEX;

            if (elem.id == TAB_BAR_PREV_TAB_ID || elem.id == TAB_BAR_NEXT_TAB_ID) {
                colorIndex = darkTheme ? TOOL_BAR_BG_DARK_INDEX : TOOL_BAR_BG_INDEX;
            }

            if (grInfo.redrawUI) {
                GlyphVariant c;

                switch (elem.icon) {
                    case Icon::ChevronRight:    c = icons[ICON_CHEVRON_RIGHT]; break;
                    case Icon::ChevronLeft:     c = icons[ICON_CHEVRON_LEFT]; break;
                    default: return;
                }

                float dimension = TAB_BAR_HEIGHT;

                float width = static_cast<float>(c.width) * 0.5;
                float height = static_cast<float>(c.height) * 0.5;

                float left = elem.left + (dimension - width) * 0.5;
                float top = elem.top + (dimension - height) * 0.5;

                PUSH_ICON_VERTICES(uiIconLayer, c, left, top, width, height, textColorIndex);

                addRectangle(
                    elem.left, elem.top, dimension, dimension,
                    colorIndex, dimension * 0.5, uiRectLayer0
                );
            }
            break;
        }

        // TODO: handle hover
        case UIElementType::TextInput: {
            if (!elem.show) { return; }

            float top = elem.top;
            float left = elem.left;

            if (
                elem.positioning == UIPositioning::Relative_sibling
                && elem.parent != nullptr
            ) {
                if (childIndex == 0) {
                    top = elem.parent->top + elem.marginTop;
                } else {
                    if (elem.parent->preferences->direction) {
                        left = elem.parent->left
                            + elem.parent->children[childIndex - 1]->left
                            + elem.parent->children[childIndex - 1]->width
                            + elem.left;
                    } else {
                        top = yPrev + elem.marginTop;
                    }
                }

                elem.top = top;
            }

            if (elem.parent->type == UIElementType::ScrollArea) {
                top -= elem.parent->scrollData->yOffset;
            }

            if (grInfo.redrawUI) {
                if (
                    elem.parent->type == UIElementType::ScrollArea
                    && (
                        top < elem.parent->top
                        || top > elem.parent->top + elem.parent->height
                    )
               ) {
                    elem.show = false;
                    return;
                }

                #ifdef DEBUG_RENDER
                    cout << "\nRe-drawing Text Input";
                #endif

                updateElementDimensionsAccordingToPreferences(&elem);

                if (
                    elem.positioning == UIPositioning::Relative_sibling
                    && elem.parent != nullptr
                ) {
                    if (elem.parent->preferences->direction) {
                        elem.left = left;
                    } else {
                        left = elem.left;
                    }
                } else {
                    left = elem.left;
                    top = elem.top;
                }

                addRectangle(
                    left,
                    top,
                    elem.width,
                    elem.height,
                    darkTheme ? TEXT_INPUT_BG_DARK_INDEX : TEXT_INPUT_BG_INDEX,
                    static_cast<float>(elem.borderRadius),
                    elem.layer == 0 ? uiRectLayer0 : uiRectLayer1
                );

                TextInputData* tid = elem.textInputData;

                glm::vec2 pos = glm::vec2(left + TEXT_INPUT_PADDING, top + TEXT_INPUT_PADDING);

                string text;
                text.reserve(elem.text.length());

                if (tid->password) {
                    for (int i=0; i < elem.text.length(); ++i) {
                        text += "*";
                    }
                } else {
                    text = elem.text.substr(tid->startIndex, elem.text.length() - tid->startIndex);
                }

                float dx = pos.x;
                float dy = pos.y;

                TextRenderResult rRes = getInputTextDimensions(
                    text,
                    tid,
                    left + TEXT_INPUT_PADDING,
                    top + TEXT_INPUT_PADDING,
                    0,
                    elem.fontSize,
                    left + TEXT_INPUT_PADDING,
                    elem.width - 2 * TEXT_INPUT_PADDING,
                    false
                );

                switch (tid->alignment) {
                    case TextInputAlignment::Center:
                        pos.x = left + (elem.width - rRes.width)/2;
                        pos.y = top + TEXT_INPUT_PADDING;
                        break;

                    case TextInputAlignment::Right:
                        pos.x = left + elem.width - rRes.width;
                        pos.y = top + TEXT_INPUT_PADDING;
                        break;
                }

                dx = pos.x - dx;
                dy = pos.y - dy;

                rRes.x += dx;
                rRes.cursorX += dx;
                rRes.selStartX += dx;

                rRes.y += dy;
                rRes.cursorY += dy;
                rRes.selStartY += dy;

                uint32_t currentIndex = 0;
                float x = left + TEXT_INPUT_PADDING;
                float y = top + TEXT_INPUT_PADDING;

                float cursorX = rRes.cursorX;
                float cursorY = rRes.cursorY - static_cast<float>(elem.fontSize) * glyphDescender;
                float selectStartX = rRes.selStartX;
                float selectEndX = selectStartX + rRes.selWidth;

                if (elem.active && tid->textSelected) {
                    globalCursor.showSelection = true;

                    vector<RectVertex> rv;
                    rv.reserve(6);

                    addRectangle(
                        selectStartX,
                        cursorY - rRes.height,
                        selectEndX - selectStartX,
                        rRes.height,
                        darkTheme ? SELECT_COLOR_DARK_INDEX : SELECT_COLOR_INDEX,
                        0,
                        rv
                    );

                    updateTextSelection(globalCursor, rv);

                    #ifdef DEBUG_TEXT_SELECTION
                        debugv("Text selection: CursorY: {}", cursorY);
                    #endif
                }

                if (!text.empty()) {
                    appendTextVertices(
                        pos.x,
                        pos.y,
                        (rRes.columns < text.length()) ? text.substr(0, rRes.columns) : text,
                        0,
                        darkTheme ? WHITE_INDEX : BLACK_INDEX,
                        elem.fontSize,
                        elem.layer == 0 ? uiTextLayer0 : uiTextLayer1
                    );
                }

                // When text input is active, draw the cursor.
                if (elem.active) {
                    globalCursor.show = true;

                    if (text.empty()) {
                        cursorY = elem.top + elem.height - TEXT_INPUT_PADDING;
                    }

                    if (grInfo.redrawUI) {
                        float height = static_cast<float>(elem.fontSize) * (1 - glyphDescender);

                        updateTextCursor(globalCursor, cursorX, cursorY - height, height);
                    }
                }
            }

            if (
                elem.positioning == UIPositioning::Relative_sibling
                && elem.parent != nullptr
            ) {
                yPrev = top + elem.height;
            }
            break;
        }

        // TODO: make similar changes as TextInput
        case UIElementType::TextArea: {
            if (grInfo.redrawUI) {
                addRectangle(
                    elem.left,
                    elem.top,
                    elem.width,
                    elem.height,
                    darkTheme ? TEXT_INPUT_BG_DARK_INDEX : TEXT_INPUT_BG_INDEX,
                    0,
                    uiRectLayer0
                );
            }

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

            vector<RectVertex> selectionVertices;
            selectionVertices.reserve(18);

            const float newLineSize = static_cast<float>(elem.fontSize)
                + static_cast<float>(BUTTON_PADDING);

            const float heightIncrement = static_cast<float>(elem.fontSize + LINE_GAP);

            if (grInfo.redrawUI && !elem.text.empty()) {
                appendTextVertices(
                    elem.left,
                    elem.top,
                    elem.text,
                    0,
                    darkTheme ? WHITE_INDEX : BLACK_INDEX,
                    elem.fontSize,
                    elem.layer == 0 ? uiTextLayer0 : uiTextLayer1
                );
            }

            if (!elem.active) { break; }

            for (const char c: elem.text) {
                if (c == '\n') {
                    if (tid->textSelected && selectStarted && !selectEnded) {
                        addRectangle(
                            selectStartX,
                            y,
                            x - selectStartX,
                            static_cast<float>(elem.fontSize) * (1 - glyphDescender),
                            darkTheme ? SELECT_COLOR_DARK_INDEX : SELECT_COLOR_INDEX,
                            0,
                            selectionVertices
                        );
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

                        addRectangle(
                            selectStartX,
                            y,
                            x - selectStartX,
                            static_cast<float>(elem.fontSize) * (1 - glyphDescender),
                            darkTheme ? SELECT_COLOR_DARK_INDEX : SELECT_COLOR_INDEX,
                            0,
                            selectionVertices
                        );
                    }
                }

                // x += renderGlyph(c, x, y, elem.fontSize); // TODO: remove this!
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

                addRectangle(
                    selectStartX,
                    y,
                    x - selectStartX,
                    static_cast<float>(elem.fontSize) * (1 - glyphDescender),
                    darkTheme ? SELECT_COLOR_DARK_INDEX : SELECT_COLOR_INDEX,
                    0,
                    selectionVertices
                );
            }

            if (grInfo.redrawUI) {
                updateTextCursor(globalCursor, cursorX, cursorY, elem.fontSize);
                updateTextSelection(globalCursor, selectionVertices);
            }

            break;
        }

        case UIElementType::TabBar: {
            if (tabs.empty()) { return; }

            glScissor(elem.left, 0, windowWidth, windowHeight);

            glEnable(GL_SCISSOR_TEST);

            if (grInfo.redrawUI) {
                addRectangle(
                    elem.left, elem.top, elem.width, elem.height,
                    darkTheme ? TOOL_BAR_BG_DARK_INDEX : TOOL_BAR_BG_INDEX,
                    0, uiRectLayer0
                );

                setupTabs(uiTextLayer0, uiIconLayer);

                addRectangle(
                    static_cast<float>(tabs[activeTabIndex].xOffset) - tabsDisplay.xOffset,
                    0.0f,
                    static_cast<float>(tabs[activeTabIndex].width),
                    TAB_BAR_HEIGHT,
                    darkTheme ? ROOT_BG_DARK_INDEX : ROOT_BG_INDEX,
                    0,
                    uiRectLayer0
                );
            }

            glDisable(GL_SCISSOR_TEST);

            if (
                tabsDisplay.totalX > windowWidth - 2 * TAB_BAR_HEIGHT
                && elem.children != nullptr
                && elem.totalChildren
            ) {
                float _y;
                glm::vec2 cursor = glm::vec2(mouseX, mouseY);

                if (grInfo.redrawUI) {
                    elem.children[0]->left = windowWidth - 2*TAB_BAR_HEIGHT;
                    elem.children[1]->left = windowWidth - TAB_BAR_HEIGHT;
                }

                renderElement(
                    *elem.children[0],
                    resolution,
                    cursor,
                    0,
                    _y,
                    uiRectLayer0,
                    uiRectLayer1,
                    uiTextLayer0,
                    uiTextLayer1,
                    uiIconLayer
                );

                renderElement(
                    *elem.children[1],
                    resolution,
                    cursor,
                    1,
                    _y,
                    uiRectLayer0,
                    uiRectLayer1,
                    uiTextLayer0,
                    uiTextLayer1,
                    uiIconLayer
                );
            }

            if (tabsDisplay.recalcOffset) {
                adjustTabOffset();
            }

            break;
        }

        case UIElementType::Tab: {
            break;
        }

        case UIElementType::Explorer: {
            if (!elem.show) {
                break;
            }

            glScissor(0, 0, elem.width, elem.height);
            glEnable(GL_SCISSOR_TEST);

            if (grInfo.redrawUI) {
                updateElementDimensionsAccordingToPreferences(&elem);

                addRectangle(
                    elem.left, elem.top, elem.width, elem.height,
                    darkTheme ? EXPLORER_BG_DARK_INDEX : EXPLORER_BG_INDEX,
                    0, uiRectLayer0
                );

                if (explorer.elementHover) {
                    addRectangle(
                        0, explorer.hoverY, EXPLORER.width, EXPLORER_ELEMENT_HEIGHT,
                        darkTheme ? EXPLORER_HOVER_BG_DARK_INDEX : EXPLORER_HOVER_BG_INDEX,
                        4,
                        uiRectLayer0
                    );
                }

                if (explorer.lineHover) {
                    addRectangle(
                        EXPLORER.width - 2.0f, EXPLORER.top,
                        2.0f, EXPLORER.height,
                        darkTheme ? EXPLORER_LINE_COLOR_DARK_INDEX : EXPLORER_LINE_COLOR_INDEX,
                        0.0f, uiRectLayer0
                    );
                }

                setupExplorer(uiTextLayer0, uiIconLayer, uiRectLayer0);
            }

            glDisable(GL_SCISSOR_TEST);

            if (
                elem.scrollData->showVerticalScrollbar
                || elem.scrollData->showHorizontalScrollbar
            ) {
                renderScrollbars(&elem);
            }
            break;
        }

        default: break;
    }
}

inline glm::vec2 getActiveContentDimensions() {
    return glm::vec2 {
        windowWidth - SCROLL_BAR_THICKNESS - CONTENT_BLOCK_DRAGTAB_WIDTH,
        windowHeight - TAB_BAR_HEIGHT
    }; // width, height
}

inline void setCBDimensions(vector<ContentBlock>::iterator cb, bool first, bool calculateHeight) {
    if (first) {
        cb->top = CONTENT_CONTAINER.top + getCBVerticalMargins(cb->type)[0];
        cb->left = CONTENT_CONTAINER.left;
    } else {
        auto prev = cb - 1;

        glm::vec2 prevMargins = getCBVerticalMargins((*prev).type);
        glm::vec2 currMargins = getCBVerticalMargins((*cb).type);

        cb->top = prev->top + prev->height + prevMargins[1] + currMargins[0];
        cb->left = prev->left;
    }

    if (calculateHeight) {
        RopeRenderResult rRes = getTextDimensions(
            cb->rope->getString(),
            contentLeft, cb->top, 0,
            getCBFontSize(cb->type),
            contentLeft,
            contentRight - contentLeft,
            false
        );

        cb->height = rRes.height;
    }
}

inline bool renderContentBlock(
    ContentTab* tab,
    vector<ContentBlock>::iterator cb,
    glm::vec2 &resolution,
    glm::vec2 &cursor,
    uint32_t index,
    vector<TextVertex>& iconVertices,
    vector<TextVertex>& extraTextVertices,
    vector<TextVertex>& textVertices,
    vector<RectVertex>& rva
) {
    if (!GET_FLAG(cb->flags, CONTENT_BLOCK_VISIBLE)) {
        cout << "\nDEBUG: Content block " << index << " not visible";
        return false;
    }

    if (CONTENT_CONTAINER.parent == nullptr) {
        cout << "\nRender Error: UIElement's parent is NULL!";
        return false;
    }

    if (grInfo.redraw) {
        setCBDimensions(cb, cb == tab->contentBlocks.begin(), false);
    }

    float top = cb->top - tab->scrollOffsetY;
    float left = contentLeft;
    float bottom = top + cb->height;

    bool visible = top >= 0 && top <= windowHeight
        || bottom >= 0 && bottom <= windowHeight
        || top < 0 && bottom > windowHeight;

    bool isActive = tab->cursorCBIndex == index;
    bool isCodeStart = false;

    glm::vec2 pos = glm::vec2(left, top);
    uint32_t textColorIndex = darkTheme ? WHITE_INDEX : BLACK_INDEX;

    if (grInfo.redraw) {
        MarkdownParser parser(cb->rope);
        Markdown md = parser.parse();

        if (index && cb->type != ContentBlockType::Code) {
            cb->type = md.cbType;
            cb->startIndex = md.startIndex;
        }

        bool renderRope = true;
        uint32_t lightTextIndex = darkTheme ? LIGHT_TEXT_COLOR_DARK_INDEX : LIGHT_TEXT_COLOR_INDEX;

        switch (cb->type) {
            case ContentBlockType::Bullet:
                if (isActive) {
                    PUSH_ICON_VERTICES(
                        iconVertices, icons[ICON_BULLET_CIRCLE], 0, 0, 0, 0,
                        lightTextIndex
                    );
                } else {
                    pos.x += 16.0f;

                    PUSH_ICON_VERTICES(
                        iconVertices, icons[ICON_BULLET_CIRCLE], left + 4.0f, top + 8.0f,
                        4.0f, 4.0f, lightTextIndex
                    );
                }
                break;

            case ContentBlockType::Numbering:
                if (isActive) {
                    PUSH_ICON_VERTICES(
                        extraTextVertices, CHAR_EMPTY, 0, 0, 0, 0,
                        lightTextIndex
                    );
                } else {
                    pos.x += 16.0f;

                    appendTextVertices(
                        left, top, cb->rope->substring(0, md.startIndex), 0,
                        lightTextIndex, getCBFontSize(cb->type),
                        extraTextVertices
                    );
                }
                // TODO: Check for the maximum width of the number characters.
                // TODO: append the text vertices with the numbering text.
                break;

            case ContentBlockType::Code:
                ContentBlockBounds bounds;

                // If the cursor is in the code bounds, show the start and end of the block
                if (!isActive) {
                    for (auto itr=tab->codeBlockBounds.begin(); itr!=tab->codeBlockBounds.end(); ++itr) {
                        if (itr->start <= tab->cursorCBIndex && itr->end >= tab->cursorCBIndex) {
                            isActive = true;
                            break;
                        }

                        if (itr->end == index) {
                            return visible;
                        }

                        if (itr->start == index) {
                            isCodeStart = true;
                            renderRope = false;
                            bounds = *itr;
                            break;
                        }
                    }

                    pos.x += 8.0f;
                }

                // if (isActive) {
                    // addRectangle(0, 0, 0, 0, 0, darkTheme ? CODE_DARK_INDEX : CODE_INDEX, 0, rva);
                // } else {
                    if (
                        isCodeStart && (
                            tab->cursorCBIndex < bounds.start
                            || tab->cursorCBIndex > bounds.end
                        )
                    ) {
                        cout << "\nFound bounds: { start: " << bounds.start
                            << ", end: " << bounds.end << " }";

                        string languageText;

                        if (md.cbType == ContentBlockType::Code) {
                            languageText = cb->rope->substring(md.startIndex, cb->rope->length() - md.startIndex);
                        }

                        uint32_t length;

                        if (bounds.end == 0) {
                            length = tab->contentBlocks.size() - tab->cursorCBIndex;
                        } else {
                            length = bounds.end - bounds.start + 1;
                        }

                        float height = static_cast<float>(length)
                            * LINE_HEIGHT(getCBFontSize(
                                ContentBlockType::Code
                            ));

                        addRectangle(
                            left,
                            top,
                            contentRight - contentLeft,
                            height,
                            darkTheme ? CODE_DARK_INDEX : CODE_INDEX,
                            0,
                            rva
                        );

                        if (!languageText.empty()) {
                            RopeRenderResult dimensions = getTextDimensions(
                                languageText,
                                left, top, 0,
                                16,
                                contentLeft,
                                contentRight - contentLeft,
                                false
                            );

                            appendTextVertices(
                                contentRight - dimensions.width - 8.0f,
                                top,
                                languageText,
                                0,
                                lightTextIndex,
                                getCBFontSize(cb->type),
                                extraTextVertices
                            );
                        }
                    }
                // }
                break;

            case ContentBlockType::Image: {
                if (!visible) { return false; }

                if (isActive) {
                    if (!md.fragments.empty()) {
                        cout << "\nDEBUG: image url: "
                            << cb->rope->substring(md.fragments[0].imgOrLink.url)
                            << ", altText: "
                            << cb->rope->substring(md.fragments[0].imgOrLink.text);
                    } else {
                        cout << "\nDEBUG: fragments: " << md.fragments.size();
                    }
                    // TODO: don't render image. Set the height to the text height.
                    cb->height = 16.0f - glyphDescender;
                } else {
                    renderRope = false;

                    if (cb->imgID == 0) {
                        cb->imgID = addCBImage(cb->rope->substring(md.fragments[0].imgOrLink.url), cb->imgID);
                        cout << "\nDEBUG: added CB image with ID: " << cb->imgID;
                    }

                    CBImageRenderQueueResult cirqr = queueImageForRender(contentLeft, top, cb->imgID);

                    if (cirqr.loaded) {
                        cb->height = cirqr.height;
                    }
                }
            }
            break;

            case ContentBlockType::Link: {
                if (isActive) {
                    if (!md.fragments.empty()) {
                        cout << "\nDEBUG: link url: "
                            << cb->rope->substring(md.fragments[0].imgOrLink.url)
                            << ", text: "
                            << cb->rope->substring(md.fragments[0].imgOrLink.text);
                    } else {
                        cout << "\nDEBUG: fragments: " << md.fragments.size();
                    }
                    cb->height = 16.0f - glyphDescender;
                } else {
                    renderRope = false;

                    if (!md.fragments.empty()) {
                        cout << "\nDEBUG: link url: "
                            << cb->rope->substring(md.fragments[0].imgOrLink.url)
                            << ", text: "
                            << cb->rope->substring(md.fragments[0].imgOrLink.text);

                        uint32_t colorIndex = darkTheme ? LINK_COLOR_DARK_INDEX : LINK_COLOR_INDEX;

                        RopeRenderResult r = appendTextVertices(
                            pos.x,
                            pos.y,
                            cb->rope->substring(md.fragments[0].imgOrLink.text),
                            0,
                            colorIndex,
                            getCBFontSize(cb->type),
                            textVertices
                        );

                        gmei.bounds.push_back(Bounds {
                            pos.x, pos.y, pos.x + r.width, pos.y + r.height,
                            BoundSourceType::Link, true, true,
                            { .cb = { .tabIndex = activeTabIndex, .index = index } },
                        });
                    }
                }
            }
            break;
        }

        if (renderRope) {
            RopeRenderResult res = appendRopeVertices(
                pos.x,
                pos.y,
                cb->rope,
                0,
                isActive ? 0 : cb->startIndex,
                getCBFontSize(cb->type),
                textColorIndex,
                textVertices
            );

            cb->height = (res.height == 0.0f && glyphDescender < 0) ?
                static_cast<float>(getCBFontSize(cb->type)) * (1.0f - glyphDescender)
                :
                res.height;

            cb->rows = res.rows;
        }
    // } else {
        // if (cb->type == ContentBlockType::Code && !isActive) {
        //     for (auto itr=tab->codeBlockBounds.begin(); itr!=tab->codeBlockBounds.end(); ++itr) {
        //         if (itr->start <= tab->cursorCBIndex && itr->end >= tab->cursorCBIndex) {
        //             break;
        //         }

        //         if (itr->end == index) {
        //             return visible;
        //         }
        //     }

        //     pos.x += 8.0f;
        // }
    } else {
        if (cb->type == ContentBlockType::Link) {
            addRectangle(0, 0, 0, 0, 0, 0, rva);
        }
    }

    if (tab->maxHeight < cb->top + cb->height) {
        #ifdef DEBUG_SCROLL_AREA
            cout << "\nScroll height less than dim.z " << dim.z;
        #endif

        tab->maxHeight = cb->top + cb->height;
        tab->scrollbarUpdated = false;
        rerenderUI = true;
    }

    return visible;
}

void renderContentBlockHighlight(vector<TextVertex>& iconVerts, vector<RectVertex>& rv) {
    if (!highlightedContentBlock.show) { return; }

    float height = highlightedContentBlock.bottom - highlightedContentBlock.top;
    float iconHeight = 16.0f * (1 - glyphDescender);
    float y = highlightedContentBlock.top;

    addRectangle(
        highlightedContentBlock.left - CONTENT_BLOCK_DRAGTAB_WIDTH, y,
        highlightedContentBlock.right - highlightedContentBlock.left + CONTENT_BLOCK_DRAGTAB_WIDTH,
        height,
        darkTheme ? CONTENT_HIGHLIGHT_BG_DARK_INDEX : CONTENT_HIGHLIGHT_BG_INDEX,
        0, rv
    );

    addRectangle(
        highlightedContentBlock.left - CONTENT_BLOCK_DRAGTAB_WIDTH,
        y,
        CONTENT_BLOCK_DRAGTAB_WIDTH,
        height,
        darkTheme ? CONTENT_HIGHLIGHT_DRAG_DARK_INDEX : CONTENT_HIGHLIGHT_DRAG_INDEX,
        3, rv
    );

    appendCharVertices(
        highlightedContentBlock.left - CONTENT_BLOCK_DRAGTAB_WIDTH * 0.5,
        y + height * 0.5 - iconHeight,
        icons[ICON_MORE_VERTICAL],
        darkTheme ? WHITE_INDEX : BLACK_INDEX,
        iconVerts
    );
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

            float initialX = contentLeft;
            float initialY = (*cbIter).top;

            float xpos = initialX;
            float yNet = initialY;

            while (rt.hasNext()) {
                Rope* rope = rt.next();

                for (char c: rope->text) {
                    GlyphVariant ch = characters[c >> 8]->glyphs[0][static_cast<uint8_t>(c)];

                    float aspectRatio = static_cast<float>(ch.width) / static_cast<float>(ch.height);

                    float glyphHeight = static_cast<float>(ch.height) * fontHeightRatio;
                    float glyphWidth = glyphHeight * aspectRatio;

                    if (glyphWidth != glyphWidth) {
                        if (ch.width != 0) {
                            glyphWidth = static_cast<float>(ch.width) / fontSize;
                        } else {
                            glyphWidth = 0.0f;
                        }
                    }

                    float widthRatio = glyphWidth / ch.width;
                    float heightRatio = glyphHeight / ch.height;

                    float bearingTop = ch.bearingTop * heightRatio;
                    float bearingLeft = ch.bearingLeft * widthRatio;

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

                    xpos += (static_cast<uint32_t>(ch.advance * fontHeightRatio) >> 6) - bearingLeft;
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

    glUseProgram(shaders[SHADER_CB_TEXT_HIGHLIGHT]);

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
    tab->cbTextMouseDown = true;
    grInfo.redraw = true;
    updateCBCursor(CURRENT_TAB, sd);
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
            ContentTab* tab = &tabs[activeTabIndex];

            float top;

            if (cbMove.targetIndex == 0) {
                uint32_t index = tab->contentBlocks.size() - 1;
                ContentBlock* last = &tab->contentBlocks[index];

                top = GET_TOP(last) + last->height;
            } else {
                ContentBlock* last;

                if (cbMove.targetIndex < tab->contentBlocks.size()) {
                    ContentBlock* block = &tab->contentBlocks[cbMove.targetIndex];
                    top = GET_TOP(block);
                } else {
                    ContentBlock* block = &tab->contentBlocks[cbMove.targetIndex - 1];
                    top = GET_TOP(block) + block->height;
                }
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

        glm::vec4 color = RED;
        DRAW_RECT(cbMove.vao, color);
    }

    #undef GET_TOP
}

uint16_t addCBImage(string location, uint16_t id) {
    uint16_t newId;
    bool oldId = false;

    if (id == 0) {
        newId = ++cbImageLoader.currentId;
    } else {
        oldId = true;
        newId = id;
    }

    for (CBImage img: cbImageLoader.images) {
        if (oldId && img.id == newId) {
            newId = ++cbImageLoader.currentId;
            break;
        }

        if (img.location == location) {
            return img.id;
        }
    }

    CBImageLocationType locationType;
    CBImageError error = CBImageError::None;
    LocationType lType = getLocationType(location);

    switch (lType) {
        case LocationType::File: locationType = CBImageLocationType::Disk; break;
        case LocationType::URL: locationType = CBImageLocationType::Internet; break;
        default:
            locationType = CBImageLocationType::NotFound;
            error = CBImageError::Unsupported;
            return 0;
    }

    cbImageLoader.images.push_back({
        locationType,
        error,
        false,
        false,
        newId,
        0,
        0,
        0,
        TIME(),
        location,
        "",
        nullptr
    });

    return newId;
}

void loadCBImagesFromDisk() {
    do {
        if (cbImageLoader.lockImageAccess) {
            SLEEP_ms(50);
            cbImageLoader.rerunWorker = true;
            continue;
        }

        cbImageLoader.rerunWorker = false;

        bool loadedSomething = false;

        for (
            vector<CBImage>::iterator imgItr = cbImageLoader.images.begin();
            imgItr != cbImageLoader.images.end();
            ++imgItr
        ) {
            if (cbImageLoader.lockImageAccess) {
                cbImageLoader.rerunWorker = true;
                break;
            }

            if (!imgItr->loaded && !imgItr->stale && imgItr->error == CBImageError::None) {
                if (imgItr->locationType == CBImageLocationType::Internet && imgItr->diskLocation.empty()) {
                    cout << "\nDEBUG: Downloading file \"" << imgItr->location << "\"";

                    string newLocation = downloadFileToTemp(imgItr->location);

                    cout << "\nDEBUG: Downloaede file to \"" << newLocation << "\"";

                    imgItr->locationType = CBImageLocationType::Disk;
                    imgItr->diskLocation = newLocation;

                    // TODO: if the location is an http(s) url, first download it into
                    //      filesystem::temp_directory_path() and update the location
                    //      to the temporary location
                }

                int width, height, nrChannels;

                uint8_t* img = stbi_load(imgItr->location.c_str(), &width, &height, &nrChannels, 3);

                if (img == nullptr) {
                    // TODO: display the failure reason:
                    const char* failure = stbi_failure_reason();

                    cout << "\nERROR: Encountered error while loading image: " << failure;

                    if (strcmp(failure, "can't fopen") == 0) {
                        imgItr->error = CBImageError::NotFound;
                    } else if (strcmp(failure, "outofmem") == 0) {
                        imgItr->error = CBImageError::OutOfMemory;
                    } else if (
                        strcmp(failure, "bad bits_per_channel") == 0
                        || strcmp(failure, "bad BMP") == 0
                        || strcmp(failure, "bad bpp") == 0
                        || strcmp(failure, "bad compression") == 0
                        || strcmp(failure, "bad file") == 0
                        || strcmp(failure, "bad format") == 0
                        || strcmp(failure, "bad header") == 0
                        || strcmp(failure, "bad Image Descriptor") == 0
                        || strcmp(failure, "bad masks") == 0
                        || strcmp(failure, "bad offset") == 0
                        || strcmp(failure, "bad palette") == 0
                        || strcmp(failure, "bad PNM") == 0
                        || strcmp(failure, "bad req_comp") == 0
                        || strcmp(failure, "BMP JPEG/PNG") == 0
                        || strcmp(failure, "BMP RLE") == 0
                        || strcmp(failure, "corrupt") == 0
                        || strcmp(failure, "invalid") == 0
                        || strcmp(failure, "illegal code in raster") == 0
                        || strcmp(failure, "missing color table") == 0
                        || strcmp(failure, "no clear code") == 0
                        || strcmp(failure, "not BMP") == 0
                        || strcmp(failure, "not GIF") == 0
                        || strcmp(failure, "not PSD") == 0
                        || strcmp(failure, "too many codes") == 0
                        || strcmp(failure, "unknown BMP") == 0
                        || strcmp(failure, "unknown code") == 0
                        || strcmp(failure, "unknown image type") == 0
                        || strcmp(failure, "unsupported bit depth") == 0
                        || strcmp(failure, "wrong version") == 0
                        || strcmp(failure, "wrong channel count") == 0
                        || strcmp(failure, "wrong color format") == 0
                    ) {
                        imgItr->error = CBImageError::Unsupported;
                    } else if (strcmp(failure, "too large") == 0) {
                        imgItr->error = CBImageError::TooLarge;
                    } else {
                        imgItr->error = CBImageError::UnknownError;
                    }

                    continue;
                }

                float targetWidth = contentRight - contentLeft;

                if (width > targetWidth || height > CONTENT_IMAGE_MAX_HEIGHT) {
                    float aspectRatio = static_cast<float>(width) / static_cast<float>(height);

                    if (width > targetWidth) {
                        imgItr->width = targetWidth;
                        imgItr->height = static_cast<float>(width) / aspectRatio;
                    }

                    if (height > CONTENT_IMAGE_MAX_HEIGHT) {
                        imgItr->height = CONTENT_IMAGE_MAX_HEIGHT;
                        imgItr->width = aspectRatio * imgItr->height;
                    }

                    imgItr->imageSize = static_cast<uint32_t>(imgItr->width) * static_cast<uint32_t>(imgItr->height) * 3;
                    imgItr->data = new uint8_t[imgItr->imageSize];

                    stbir_resize_uint8_linear(
                        img, width, height, 0, imgItr->data, static_cast<uint32_t>(imgItr->width),
                        static_cast<uint32_t>(imgItr->height), 0, STBIR_RGB
                    );

                    delete img;
                } else {
                    imgItr->imageSize = width * height * 3;
                    imgItr->data = img;
                    imgItr->width = width;
                    imgItr->height = static_cast<float>(height);
                }

                loadedSomething = true;

                imgItr->loaded = true;
                imgItr->stale = false;

                cout << "\nDEBUG: Image \"" << imgItr->location
                    << "\" is loaded (width: " << imgItr->width
                    << ", height: " << height << ", channels: " << nrChannels
                    << ", size: " << imgItr->imageSize << ")";
            }
        }

        if (loadedSomething) {
            grInfo.redraw = true;
        }
    } while (cbImageLoader.rerunWorker);

    cbImageLoader.workerRunning = false;
}

void loadCBImages() {
    if (!cbImageLoader.workerRunning) {
        cbImageLoader.workerRunning = true;
        jthread worker(loadCBImagesFromDisk);
        #ifdef _WIN32
            SetThreadDescription(worker.native_handle(), L"cbImageLoader Worker");
        #endif
        return;
    }

    cbImageLoader.rerunWorker = true;
}

CBImageRenderQueueResult queueImageForRender(float x, float y, uint32_t imageID) {
    bool loaded = false;
    float height = 0.0f;
    uint32_t index = 0;
    uint32_t i = 0;

    cbImageLoader.lockImageAccess = true;

    for (
        vector<CBImage>::iterator imgItr = cbImageLoader.images.begin();
        imgItr != cbImageLoader.images.end();
        ++imgItr, ++i
    ) {
        if (imgItr->stale || imgItr->data == nullptr) {
            imgItr->stale = false;
            break;
        }

        if (imgItr->id == imageID && imgItr->loaded) {
            height = imgItr->height;
            loaded = true;
            index = i;
            break;
        }
    }

    cbImageLoader.lockImageAccess = false;

    if (loaded) {
        CBImage img = cbImageLoader.images[index];
        float contentWidth = contentRight - contentLeft;

        x += (contentWidth - img.width) / 2;

        #ifdef DEBUG_CB_IMAGES
            cout << "\nDEBUG: Queueing image id: " << img.id << ", location: " << img.location;
        #endif
        cbImageLoader.renderQueue.push_back(CBImageRenderQueueEntry { x, y, index });
    } else {
        loadCBImages();
    }

    return { loaded, height };
}

inline CBImageBufferSwapResult loadCBImageIntoPBOBuffer(CBImage* image, uint32_t bufferIndex) {
    uint32_t nextIndex = bufferIndex;
    bool shouldLoad = false;

    if (image->stale || image->data == nullptr) {
        image->stale = false;
        shouldLoad = true;
    } else if (image->loaded) {
        nextIndex = (bufferIndex + 1) % 2;
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, grInfo.imagePBO[bufferIndex]);

        void* pboImgPtr = glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);

        if (pboImgPtr != nullptr) {
            std::memcpy(pboImgPtr, image->data, image->imageSize);
            glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
            #ifdef DEBUG_CB_IMAGES
                cout << "\nSwapped buffer(" << bufferIndex << ") for image: " << image->id;
            #endif
        } else {
            // TODO: report error!
        }

        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    }

    return { image->loaded, shouldLoad, nextIndex };
}

inline void renderCBImage(
    CBImageRenderQueueEntry imgEntry,
    CBImage* img,
    uint32_t pboIndex,
    uint32_t len
) {
    glBindVertexArray(grInfo.image.vao);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, grInfo.imageTexture);
    glUseProgram(shaders[SHADER_IMAGE]);

    if (grInfo.redraw || len > 1) {
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, grInfo.imagePBO[pboIndex]);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, img->width, img->height, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

        float x2 = imgEntry.x + img->width;
        float y2 = imgEntry.y + img->height;
        float s = img->width / CONTENT_MAX_WIDTH;
        float t = img->height / CONTENT_IMAGE_MAX_HEIGHT;

        float vertices[16] = {
            imgEntry.x, imgEntry.y, 0, 0,
            imgEntry.x, y2,         0, t,
            x2,         imgEntry.y, s, 0,
            x2,         y2,         s, t,
        };

        glBindBuffer(GL_ARRAY_BUFFER, grInfo.image.vbo);
        glBufferSubData(GL_ARRAY_BUFFER, imgEntry.bufferOffset, sizeof(vertices), vertices);

        #ifdef DEBUG_CB_IMAGES
            cout << "\nDEBUG: rendered image: " << img->id << " in buffer index: "
                << pboIndex << ", width: " << img->width << ", height: " << img->height;
        #endif
    }

    img->lastRendered = TIME();

    glDrawArrays(GL_TRIANGLE_STRIP, imgEntry.vertexOffset, 4);
}

void renderCBImages() {
    if (cbImageLoader.images.empty() || cbImageLoader.renderQueue.empty()) { return; }

    uint32_t totalLen = cbImageLoader.renderQueue.size();

    vector<CBImageRenderQueueEntry>::iterator lastItr = cbImageLoader.renderQueue.begin();

    CBImageRenderQueueEntry lastEntry = *lastItr;
    CBImage* prevImg = &cbImageLoader.images[lastEntry.imageIndex];

    if (totalLen == 1 && !grInfo.redraw) {
        renderCBImage(lastEntry, prevImg, 0, totalLen);
        return;
    }

    if (totalLen == 2 && !grInfo.redraw) {
        renderCBImage(lastEntry, prevImg, 0, totalLen);
        lastEntry = cbImageLoader.renderQueue[1];
        prevImg = &cbImageLoader.images[lastEntry.imageIndex];
        renderCBImage(lastEntry, prevImg, 1, totalLen);
        return;
    }

    if (grInfo.redraw) {
        uint32_t reqSize = 4 * totalLen;

        if (reqSize > grInfo.image.numVertices) {
            while (grInfo.image.numVertices < reqSize) {
                grInfo.image.numVertices *= 2;
            }

            grInfo.image.bufferSize = grInfo.image.numVertices * 4 * sizeof(float);

            glBindVertexArray(grInfo.image.vao);
            glBindBuffer(GL_ARRAY_BUFFER, grInfo.image.vbo);
            glBufferData(GL_ARRAY_BUFFER, grInfo.image.bufferSize, nullptr, GL_DYNAMIC_DRAW);
        }
    }

    CBImageBufferSwapResult cibsr = loadCBImageIntoPBOBuffer(prevImg, 0);
    CBImageBufferSwapResult prevCibsr = cibsr;

    uint32_t pboIndex = 0;
    uint32_t vertexOffset = 0;
    uint32_t bufferOffset = 0;
    bool shouldLoad = cibsr.shouldLoad;

    for (
        vector<CBImageRenderQueueEntry>::iterator itr = cbImageLoader.renderQueue.begin() + 1;
        itr != cbImageLoader.renderQueue.end();
        ++itr
    ) {
        if (itr->imageIndex < totalLen) {
            CBImage* img = &*(cbImageLoader.images.begin() + itr->imageIndex);
            shouldLoad = shouldLoad || cibsr.shouldLoad;

            if (prevCibsr.imageLoaded) {
                uint32_t prevIndex = pboIndex;
                pboIndex = prevCibsr.nextBufferIndex;

                if (grInfo.redraw) {
                    lastItr->bufferOffset = bufferOffset;
                    lastItr->vertexOffset = vertexOffset;
                    vertexOffset += 4;
                    bufferOffset += 64;
                }

                cibsr = loadCBImageIntoPBOBuffer(img, pboIndex);
                renderCBImage(*lastItr, prevImg, prevIndex, totalLen);
            } else {
                cibsr = loadCBImageIntoPBOBuffer(img, pboIndex);
                shouldLoad = true;
            }

            prevCibsr = cibsr;
            prevImg = img;
            lastEntry = *itr;
            lastItr = itr;
        }
    }

    if (prevCibsr.imageLoaded) {
        if (grInfo.redraw) {
            lastItr->bufferOffset = bufferOffset;
            lastItr->vertexOffset = vertexOffset;
        }

        renderCBImage(*lastItr, prevImg, pboIndex, totalLen);
    }

    if (shouldLoad) {
        loadCBImages();
    }
}

void removeOldCBImages(std::stop_token stopToken) {
    #ifdef DEBUG_CB_IMAGES
        cout << "\nDEBUG: Running the remove old cb images thread.";
    #endif

    while (true) {
        if (stopToken.stop_requested()) {
            #ifdef DEBUG_CB_IMAGES
                cout << "\nDEBUG: Stop requested on remove old cb images thread.";
            #endif
            return;
        }

        if (cbImageLoader.workerRunning || cbImageLoader.lockImageAccess) {
            SLEEP_ms(100);
            continue;
        }

        float time = TIME();

        for (
            vector<CBImage>::iterator imgItr = cbImageLoader.images.begin();
            imgItr != cbImageLoader.images.end();
        ) {
            if (cbImageLoader.lockImageAccess) { break; }

            if (imgItr->stale) {
                if (time - imgItr->lastRendered > 300) {
                    imgItr = cbImageLoader.images.erase(imgItr);
                    continue;
                }
            } else {
                if (time - imgItr->lastRendered > 10) {
                    imgItr->loaded = false;
                    imgItr->stale = true;
                    imgItr->imageSize = 0;

                    if (imgItr->data) {
                        delete imgItr->data;
                        imgItr->data = nullptr;
                    }

                    #ifdef DEBUG_CB_IMAGES
                        cout << "\nDEBUG: Image \"" << imgItr->location
                            << "\" cleared from memory since it was last rendered "
                            << imgItr->lastRendered
                            << " seconds ago.";
                    #endif
                }
            }

            ++imgItr;
        }

        if (minimized) {
            SLEEP_s(1);
        } else {
            SLEEP_ms(200);
        }
    }
}

void hideCBMoveMarker() {
    cbMove.show = false;
    glDeleteVertexArrays(1, &cbMove.vao);
    glDeleteBuffers(1, &cbMove.vbo);

    cbMove.vao = 0;
    cbMove.vbo = 0;
    cbMove.targetIndex = 0;
}

bool isCBInCodeBlock(ContentTab* tab, uint32_t contentIndex) {
    for (auto itr=tab->codeBlockBounds.begin(); itr!=tab->codeBlockBounds.end(); ++itr) {
        if (itr->start <= contentIndex && itr->end > contentIndex) {
            return true;
        }
    }

    return false;
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
        delete (*cbIter).rope;

        extCmd->deletedBlocks.emplace_back(ContentBlock {
            (*cbIter).type, 0, 0,
            0.0f, 0.0f, 0.0f, 0, 0, Rope::deepCopy((*cbIter).rope)
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
                REDRAW_UI();
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

UIElement* getElementById(UIElement * root, uint16_t id) {
    stack<UIElement*> elemStack;

    for (uint8_t i=0; i<root->totalChildren; ++i) {
        UIElement* e = root->children[(int)i];
        elemStack.push(e);
    }

    while (!elemStack.empty()) {
        UIElement* elem = elemStack.top();

        if (elem->id == id) {
            return elem;
        }

        elemStack.pop();

        if (elem->totalChildren) {
            for (uint8_t i=0; i<elem->totalChildren; ++i) {
                UIElement* e = elem->children[(int)i];
                elemStack.push(e);
            }
        }
    }

    return nullptr;
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
    UIElement* container = nullptr;

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

            if (elem->type == UIElementType::Container && elem->id == DIALOG_OVERLAY_ID) {
                container = elem;
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

    if (scrollElem != nullptr) {
        return scrollElem;
    } else if (container != nullptr) {
        return container;
    }

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

    REDRAW_CB();
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
            0, 0, 0.0f, 0.0f, 0.0f, 0, 0,
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
            CONTENT_BLOCK_VISIBLE, 0,
            0.0f, 0.0f, 0.0f, 1, 0, new Rope()
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
            0, 0, 0.0f, 0.0f, 0.0f, 0, 0,
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
            (*firstCB).flags, 0,
            0.0f, 0.0f, 0.0f, 0, 0,
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
        0, 0, 0.0f, 0.0f, 0.0f, 0, 0,
        new Rope(firstRopeString.substr(cmd->startCursorPos, firstRopeString.length() - cmd->startCursorPos))
    });

    extCmd->deletedBlocks.insert(extCmd->deletedBlocks.begin() + 1, firstCB + 1, lastCB);

    extCmd->deletedBlocks.push_back(ContentBlock {
        (*lastCB).type,
        0, 0, 0.0f, 0.0f, 0.0f, 0, 0,
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
                    0, 0, 0.0f, 0.0f, 0.0f, 1, 0, new Rope(str)
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
                0, 0, 0.0f, 0.0f, 0.0f, 1, 0, new Rope(str)
            });
        }
    }

    return blocks;
}

bool processCBTypeCommand(
    ContentTab* tab, ContentCommand* cmd, ContentBlock* cb, bool newCmd
) {
    if (cmd->str != "") {
        bool increment = true;

        cb->rope->insert(cmd->startCursorPos, cmd->str);

        if (cb->rows == 0) {
            cb->rows = 1;
        }

        if (
            newCmd && !tab->commands.empty()
            && tab->currentCommand == tab->commands.size() - 1
            && tab->commands.back().type == CommandType::Type
        ) {
            tab->commands.back().str += cmd->str;
            increment = false;
        }

        if (cmd->startCBIndex != 0) {
            MarkdownParser parser(cb->rope);
            Markdown md = parser.parse();

            if (
                cb->type != ContentBlockType::Code && md.cbType == ContentBlockType::Code
                || cb->type == ContentBlockType::Code && cmd->str == "`"
                    && containsCodeBlockBound(cb->rope)
            ) {
                cb->type = ContentBlockType::Code;

                if (tab->codeBlockBounds.empty()) {
                    tab->codeBlockBounds.push_back(ContentBlockBounds { cmd->startCBIndex, 0 });
                } else {
                    bool foundMissingStart = false;
                    bool foundMissingEnd = false;

                    vector<ContentBlockBounds>::iterator cbbIter = tab->codeBlockBounds.begin();

                    for (; cbbIter != tab->codeBlockBounds.end(); ++cbbIter) {
                        if (cbbIter->start == 0) {
                            foundMissingStart = true;
                            break;
                        }

                        if (cbbIter->end == 0) {
                            foundMissingEnd = true;
                            break;
                        }
                    }

                    if (foundMissingEnd) {
                        cbbIter->end = cmd->startCBIndex;
                    } else if (foundMissingStart) {
                        cbbIter->start = cmd->startCBIndex;
                    } else {
                        tab->codeBlockBounds.push_back(ContentBlockBounds {
                            cmd->startCBIndex, 0
                        });
                    }
                }
            } else {
                for (auto itr=tab->codeBlockBounds.begin(); itr!=tab->codeBlockBounds.end();++itr) {
                    if (itr->start <= cmd->startCBIndex && itr->end > cmd->startCBIndex) {
                        if (cb->type == ContentBlockType::Paragraph) {
                            cb->type = ContentBlockType::Code;
                            break;
                        }
                    }
                }
            }
        }

        return increment;
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

            for (auto pbItr = pastedBlocks.begin(); pbItr != pastedBlocks.end(); ++pbItr) {
                pbItr->rope = Rope::deepCopy(pbItr->rope);
            }

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

        uint32_t pastedCount = 0;
        const uint32_t pastedBlocksLen = pastedBlocks.size();

        for (
            auto itr=tab->contentBlocks.begin() + cursorCBIndex;
            itr != tab->contentBlocks.end() && pastedCount < pastedBlocksLen;
            ++itr, ++pastedCount
        ) {
            SET_FLAG((*itr).flags, CONTENT_BLOCK_VISIBLE);
            setCBDimensions(itr, itr == tab->contentBlocks.begin(), true);
        }
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

    REDRAW_CB();

    #undef START
    #undef END

    return true;
}

bool processCBNewLineCommand(
    ContentTab* tab, ContentCommand* cmd, ContentBlock* cb, bool newCmd
) {
    if (tab->cursorCBIndex == 0) {
        if (tab->contentBlocks.size() > 1) {
            tab->cursorCBIndex = 1;
            tab->cursorPosition = 0;
        }

        return false;
    }

    bool isCodeEndBound = false;

    for (auto itr=tab->codeBlockBounds.begin(); itr!=tab->codeBlockBounds.end(); ++itr) {
        if (itr->end == cmd->startCBIndex) {
            isCodeEndBound = true;
            break;
        }

        if (itr->start <= cmd->startCBIndex && itr->end > cmd->startCBIndex) {
            ++itr->end;
            break;
        }
    }

    if (cmd->startCursorPos == cb->rope->length() || cmd->startCursorPos == 0) {
        ++tab->cursorCBIndex;

        ContentBlockType type = cb->type;

        if (cmd->startCursorPos == cb->rope->length() && isCodeEndBound) {
            type = ContentBlockType::Paragraph;
        }

        tab->contentBlocks.insert(
            tab->contentBlocks.begin() + tab->cursorCBIndex,
            ContentBlock {
                type,
                CONTENT_BLOCK_VISIBLE, 0,
                0.0f, 0.0f, 0.0f,
                0, 0,
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
                        CONTENT_BLOCK_VISIBLE, 0,
                        0.0f, 0.0f, 0.0f,
                        0, 0,
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
                CONTENT_BLOCK_VISIBLE, 0,
                0.0f, 0.0f, 0.0f,
                0, 0,
                nullptr
            });
    }

    tab->cursorPosition = 0;

    REDRAW_CB();

    return true;
}

bool processCBBackspaceCommand(
    ContentTab* tab, ContentCommand* cmd, ContentBlock* cb, bool newCmd
) {
    if (cmd->startCursorPos > 0) {
        cb->rope->remove(cmd->startCursorPos - 1, cmd->str.length());
        REDRAW_CB();

        if (
            newCmd && !tab->commands.empty()
            && tab->currentCommand == tab->commands.size() - 1
            && tab->commands.back().type == CommandType::Backspace
            && tab->commands.back().startCursorPos == cmd->startCursorPos + 1
        ) {
            tab->commands.back().str = cmd->str + tab->commands.back().str;
            --tab->commands.back().startCursorPos;
            return false;
        }

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

        REDRAW_CB();

        return true;
    }

    return false;
}

bool processCBDeleteSelectedCommand(
    ContentTab* tab, ContentCommand* cmd, ContentBlock* cb, bool newCmd
) {
    auto extCmd = tab->extCommands.rbegin();
    (*extCmd).delType = deleteSelectedCBText(tab, cmd, &*extCmd);
    REDRAW_CB();
    return true;
}

void processCBCommand(ContentCommand cmd) {
    ContentTab* tab = &tabs[activeTabIndex];
    ContentBlock* cb = cmd.startCBIndex >= tab->contentBlocks.size() ? nullptr : &tab->contentBlocks[cmd.startCBIndex];

    #define CONCAT(a, b, c) a ## b ## c
    #define PROC(cmdType) CONCAT(processCB, cmdType, Command)(tab, &cmd, cb, true);

    #ifdef DEBUG_COMMANDS
    #ifdef DEBUG_TEXT_INPUT
        cout << "\nProcessing command \"" << to_string(cmd.type)
            << "\"\n  -> str: \"" << cmd.str << "\", startCBIndex: "
            << cmd.startCBIndex << ", startCursorPos: " << cmd.startCursorPos;

        if (cmd.ext) {
            cout << ", endCBIndex: " << tab->extCommands[cmd.extIndex].endCBIndex
                << ", endCursorPos: " << tab->extCommands[cmd.extIndex].endCursorPos;
        }
    #endif
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
        case CommandType::MoveContentBlock: {
            append = true;
            moveContentBlock(activeTabIndex, cmd.moveCBTo, cmd.moveCBFrom);
            break;
        }
    }

    updateCBCursor(tab, getCBTextCursor(resolution));

    if (append) {
        if (!tab->updated) {
            rerenderUI = true;
            // REDRAW_UI();
        }

        tab->updated = true;

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

        uint32_t i = 0;

        for (auto cmd: tab->commands) {
        //     auto cmd = tab->commands.begin();
        //     cmd != tab->commands.end();
        //     ++cmd
        // ) {
            if (i == tab->currentCommand) {
                cout << "\n  -> ";
            } else {
                cout << "\n     ";
            }

            if (cmd.ext) {
                printContentCommand(&cmd, &tab->extCommands[cmd.extIndex]);
                #ifdef DEBUG_CONTENT_BLOCKS
                    printContentBlocksFromVector(tab->extCommands[cmd.extIndex].deletedBlocks);
                    if (!tab->extCommands[cmd.extIndex].insertedBlocks.empty()) {
                        cout << "\n     |-> Inserted Content Blocks:";
                        printContentBlocksFromVector(tab->extCommands[cmd.extIndex].insertedBlocks);
                    }
                #endif
            } else {
                printContentCommand(&cmd);
            }

            ++i;
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

                    #ifdef DEBUG_CONTENT_BLOCKS
                    cout << "\nDEBUG: End rope is: \"" << (*(END)).rope->getString() << "\"";
                    cout << "\nDEBUG: Last pasted rope is: \"" << extCmd->insertedBlocks.back().rope->getString() << "\"";
                    cout << "\nDEBUG: End rope type is: " << to_string((*(END)).type);
                    #endif

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

                deleteCBs(tab, cbIter, cbIter + 1);
            } else {
                auto cbIter = tab->contentBlocks.begin() + cmd->startCBIndex + 1;
                string newLineString = (*cbIter).rope->getString();

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

        case CommandType::MoveContentBlock: {
            decrement = true;

            if (cmd->moveCBFrom < cmd->moveCBTo) {
                if (cmd->moveCBTo + 1 == tab->contentBlocks.size()) {
                    moveContentBlock(activeTabIndex, cmd->moveCBFrom, cmd->moveCBTo);
                } else {
                    moveContentBlock(activeTabIndex, cmd->moveCBFrom, cmd->moveCBTo + 1);
                }
            } else {
                moveContentBlock(activeTabIndex, cmd->moveCBFrom - 1, cmd->moveCBTo);
            }
            break;
        }
    }

    if (decrement) {
        --tab->currentCommand;
    }

    REDRAW_CB();

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

        case CommandType::MoveContentBlock: {
            increment = true;
            moveContentBlock(activeTabIndex, cmd->moveCBTo, cmd->moveCBFrom);
            REDRAW_CB();
            break;
        }
    }

    if (increment) {
        ++tab->currentCommand;
    }

    tab->updated = true;

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
                REDRAW_UI();

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

                        REDRAW_UI();
                        break;
                    }

                    elem->text.insert(CUR_POS, cmd->str);
                }

                REDRAW_UI();
                append = true;
                break;
            }

            case CommandType::NewLine: break;

            case CommandType::Delete: {
                uint32_t newLines = 0;
                REDRAW_UI();

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
                    REDRAW_UI();
                }
                append = true;
                break;
            }

            case CommandType::Backspace: {
                uint32_t newLines = 0;
                REDRAW_UI();

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

                REDRAW_UI();
                break;
            }

            case CommandType::Undo:
                REDRAW_UI();
                processUndo(elem);
                break;

            case CommandType::Redo:
                REDRAW_UI();
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

void initFloatingUI() {
    initArena(floatingArena, FLOATING_ARENA_CAPACITY);
    initializeCapacity(&FLOATING_CONTAINER, 8, &floatingArena);

    FLOATING_CONTAINER.show = true;
    grInfo.activeLayer = 1;
    globalCursor.layer = 1;
    REDRAW_UI();
}

void deleteElementBuffersAndStrings(UIElement* elem) {
    elem->text.~string();

    if (elem->totalChildren) {
        for (int i=0; i<elem->totalChildren; ++i) {
            deleteElementBuffersAndStrings(elem->children[i]);
        }
    }
}

void destroyFloatingUI() {
    for (int i=0; i<FLOATING_CONTAINER.totalChildren; ++i) {
        deleteElementBuffersAndStrings(FLOATING_CONTAINER.children[i]);
    }

    destroyArena(floatingArena);

    FLOATING_CONTAINER.totalChildren = 0;
    FLOATING_CONTAINER.capacity = 0;
    FLOATING_CONTAINER.children = nullptr;
    FLOATING_CONTAINER.show = false;

    dialogOpen = false;

    if (CONTENT_CONTAINER.show) {
        activeElement = &CONTENT_CONTAINER;
    } else {
        activeElement = nullptr;
    }

    grInfo.activeLayer = 0;
    globalCursor.layer = 0;
    globalCursor.show = false;
    globalCursor.showSelection = false;
}

void showExplorerContextMenu(float x, float y) {
    #define BUTTON(e, name, _id, margin) do {\
        e->text = name;\
        e->layer = 1;\
        e->id = _id;\
        e->fontSize = 14;\
        e->marginTop = margin;\
        e->positioning = UIPositioning::Relative_sibling;\
        e->preferences->widthPreference = UISizePreference::Full;\
        e->preferences->heightPreference = UISizePreference::Grow;\
    } while (false)

    destroyFloatingUI();
    initFloatingUI();

    UIElement* contextContainer = (UIElement*) arenaAllocate(floatingArena, sizeof(UIElement));
    UIElement* renameButton = (UIElement*) arenaAllocate(floatingArena, sizeof(UIElement));
    UIElement* deleteButton = (UIElement*) arenaAllocate(floatingArena, sizeof(UIElement));
    UIElement* newNoteButton = (UIElement*) arenaAllocate(floatingArena, sizeof(UIElement));
    UIElement* newFolderButton = (UIElement*) arenaAllocate(floatingArena, sizeof(UIElement));

    initElement(*contextContainer, UIElementType::Container, y, x, 160, 130);
    initElement(*renameButton, UIElementType::Button, 0, 0, 200, 50);
    initElement(*deleteButton, UIElementType::Button, 0, 0, 200, 50);
    initElement(*newNoteButton, UIElementType::Button, 0, 0, 200, 50);
    initElement(*newFolderButton, UIElementType::Button, 0, 0, 200, 50);

    float vertices[8] = { 0 };

    contextContainer->id = EXPLORER_CONTEXT_MENU_ID;
    contextContainer->positioning = UIPositioning::Relative_sibling;
    contextContainer->preferences->direction = false;
    contextContainer->borderRadius = 5;
    contextContainer->preferences->widthPreference = UISizePreference::Fixed;
    contextContainer->preferences->heightPreference = UISizePreference::Fixed;
    contextContainer->preferences->horizontalAlign = UIAlignment::None;
    contextContainer->preferences->verticalAlign = UIAlignment::None;

    BUTTON(renameButton, "Rename", EXPLORER_MENU_RENAME_ID, 0);
    BUTTON(deleteButton, "Delete", EXPLORER_MENU_DELETE_ID, 2);
    BUTTON(newNoteButton, "New Note", EXPLORER_MENU_NEW_NOTE_ID, 8);
    BUTTON(newFolderButton, "New Folder", EXPLORER_MENU_NEW_FOLDER_ID, 2);

    addChild(contextContainer, renameButton, &floatingArena);
    addChild(contextContainer, deleteButton, &floatingArena);
    addChild(contextContainer, newNoteButton, &floatingArena);
    addChild(contextContainer, newFolderButton, &floatingArena);
    addChild(&FLOATING_CONTAINER, contextContainer);

    dialogOpen = true;

    #undef BUTTON
}

vector<UIElement*> initBasicDialog(
    float dialogWidth,
    float dialogHeight,
    string title,
    string message,
    string button1Text,
    uint16_t button1ID,
    string button2Text,
    uint16_t button2ID,
    string inputText,
    uint16_t inputID
) {
    destroyFloatingUI();
    initFloatingUI();

    vector<UIElement*> list;

    UIElement* elems = (UIElement*) arenaAllocate(floatingArena, 8*sizeof(UIElement));

    float top = static_cast<float>(windowHeight)/2 - dialogHeight/2;
    float left = static_cast<float>(windowWidth)/2 - dialogWidth/2;

    initElement(elems[0], UIElementType::Container, 0, 0, windowWidth, windowHeight, &floatingArena);
    initElement(elems[1], UIElementType::Container, top, left, dialogWidth, dialogHeight, &floatingArena);
    initElement(elems[2], UIElementType::Label, 0, 0, 200, 50, &floatingArena);
    initElement(elems[3], UIElementType::Label, 0, 0, dialogWidth - 50, 50, &floatingArena);
    initElement(elems[4], UIElementType::TextInput, 0, 0, 200, 32, &floatingArena);
    initElement(elems[5], UIElementType::Container, 0, 0, 140, 50, &floatingArena);
    initElement(elems[6], UIElementType::Button, 50, 0, 60, 40, &floatingArena);
    initElement(elems[7], UIElementType::Button, 50, 0, 75, 40, &floatingArena);

    float vertices[8] = { 0 };

    for (int i=0; i<8; ++i) {
        elems[i].positioning = UIPositioning::Absolute;

        if (elems[i].preferences != nullptr) {
            elems[i].preferences->horizontalAlign = UIAlignment::Center;
            elems[i].preferences->verticalAlign = UIAlignment::Center;
            elems[i].preferences->widthPreference = UISizePreference::Grow;
            elems[i].preferences->heightPreference = UISizePreference::Grow;
        }

        list.push_back(&elems[i]);
    }

    elems[0].id = DIALOG_OVERLAY_ID;
    elems[0].layer = 1;
    elems[0].text = "OVERLAY";
    elems[0].preferences->widthPreference = UISizePreference::Full;
    elems[0].preferences->heightPreference = UISizePreference::Full;
    elems[0].borderRadius = 0;

    elems[1].id = DIALOG_CONTAINER_ID;
    elems[1].layer = 1;
    elems[1].width = dialogWidth;
    elems[1].height = dialogHeight;
    elems[1].text = "DIALOG";
    elems[1].preferences->direction = false;
    elems[1].borderRadius = 5;
    elems[1].preferences->widthPreference = UISizePreference::Fixed;
    elems[1].preferences->heightPreference = UISizePreference::Fixed;

    elems[2].layer = 1;
    elems[2].text = title;
    elems[2].fontSize = 20;
    elems[2].marginTop = 16;
    elems[2].positioning = UIPositioning::Relative_sibling;
    elems[2].preferences->heightPreference = UISizePreference::Fixed;
    elems[2].preferences->verticalAlign = UIAlignment::Start;

    if (!message.empty()) {
        elems[3].text = message;
        elems[3].layer = 1;
        elems[3].marginTop = 16;
        elems[3].positioning = UIPositioning::Relative_sibling;
        elems[3].preferences->heightPreference = UISizePreference::Fixed;
        elems[3].preferences->verticalAlign = UIAlignment::Start;
    } else {
        elems[3].show = false;
    }

    if (!inputText.empty()) {
        elems[4].text = inputText;
        elems[4].marginTop = 16;
        elems[4].id = inputID;
        elems[4].layer = 1;
        elems[4].positioning = UIPositioning::Relative_sibling;
        elems[4].preferences->heightPreference = UISizePreference::Fixed;
        elems[4].textInputData->alignment = TextInputAlignment::Center;
        elems[4].textInputData->textSelected = true;
        elems[4].textInputData->textSelectStartCol = 0;
        elems[4].textInputData->textSelectEndCol = inputText.length();
        elems[4].textInputData->cursorPosition = inputText.length();
        elems[4].textInputData->textSelectStartRow = 0;
        elems[4].textInputData->textSelectEndRow = 0;
        elems[4].textInputData->textSelectUpdated = true;
    } else {
        elems[4].show = false;
    }

    elems[5].text = "BTN_CONT";
    elems[5].id = HORIZONTAL_BUTTON_ROW_ID;
    elems[5].positioning = UIPositioning::Relative_sibling;
    elems[5].preferences->direction = true;
    elems[5].marginTop = 16;
    elems[5].preferences->widthPreference = UISizePreference::Fixed;
    elems[5].preferences->heightPreference = UISizePreference::Fixed;

    if (!button1Text.empty()) {
        elems[6].text = button1Text;
        elems[6].id = button1ID;
        elems[6].layer = 1;
    } else {
        elems[6].show = false;
    }

    if (!button2Text.empty()) {
        elems[7].text = button2Text;
        elems[7].id = button2ID;
        elems[7].layer = 1;
    } else {
        elems[7].show = false;
    }

    addChild(&elems[5], &elems[6], &floatingArena);
    addChild(&elems[5], &elems[7], &floatingArena);

    addChild(&elems[1], &elems[2], &floatingArena);
    addChild(&elems[1], &elems[3], &floatingArena);
    addChild(&elems[1], &elems[4], &floatingArena);
    addChild(&elems[1], &elems[5], &floatingArena);

    addChild(&elems[0], &elems[1], &floatingArena);

    addChild(&FLOATING_CONTAINER, &elems[0], &floatingArena);
    lastFloatingUIShown = glfwGetTime();
    dialogOpen = true;
    activeElement = &elems[4];
    elems[4].active = true;

    return list;
}

void showRenameDialog() {
    filesystem::path p = get<1>(getActiveExplorerNodePath(explorer.nodes));

    if (p.empty()) { return; }

    initBasicDialog(
        300,
        160,
        "Rename",
        "",
        "Ok",
        RENAME_DIALOG_OK_BUTTON_ID,
        "Cancel",
        RENAME_DIALOG_CANCEL_BUTTON_ID,
        p.filename().string(),
        RENAME_DIALOG_INPUT_ID
    );
}

void showDeleteDialog() {
    tuple<ExplorerNodeType, filesystem::path> tup = getActiveExplorerNodePath(explorer.nodes);
    ExplorerNodeType ent = get<0>(tup);
    filesystem::path p = get<1>(tup);

    if (p.empty()) { return; }

    initBasicDialog(
        300,
        160,
        "Delete " + string(ent == ExplorerNodeType::Note ? "note" : "folder"),
        "Delete \"" + p.filename().string() + "\"?" ,
        "Delete",
        DELETE_DIALOG_DELETE_BUTTON_ID,
        "Cancel",
        DELETE_DIALOG_CANCEL_BUTTON_ID,
        "",
        0
    );
}

void showNewNoteDialog() {
    string newNoteName = "Untitled Note";

    if (untitledNotes > 0) {
        newNoteName += " " + to_string(untitledNotes);
    }

    ++untitledNotes;

    initBasicDialog(
        300,
        160,
        "New Note",
        "",
        "Create",
        NEW_NOTE_DIALOG_CREATE_BUTTON_ID,
        "Cancel",
        NEW_NOTE_DIALOG_CANCEL_BUTTON_ID,
        newNoteName,
        NEW_NOTE_DIALOG_INPUT_ID
    );
}

void showNewFolderDialog() {
    initBasicDialog(
        300,
        160,
        "New Folder",
        "",
        "Create",
        NEW_FOLDER_DIALOG_CREATE_BUTTON_ID,
        "Cancel",
        NEW_FOLDER_DIALOG_CANCEL_BUTTON_ID,
        "Untitled Folder",
        NEW_FOLDER_DIALOG_INPUT_ID
    );
}

void showErrorDialog(string title, string message) {
    initBasicDialog(
        300, 160, title, message, "Ok", ERROR_DIALOG_OK_BUTTON_ID, "", 0, "", 0
    );
}

void hideContextMenus() {
    if (FLOATING_CONTAINER.totalChildren) {
        for (int i=0; i<FLOATING_CONTAINER.totalChildren; ++i) {
            if (FLOATING_CONTAINER.children[i]->id == EXPLORER_CONTEXT_MENU_ID) {
                destroyFloatingUI();
                return;
            }
        }
    }
}

#ifdef DEBUG_BOUNDS
uint32_t boundsVAO = 0;
uint32_t boundsVBO = 0;

void initBoundsRendering() {
    glGenVertexArrays(1, &boundsVAO);
    glBindVertexArray(boundsVAO);
    glGenBuffers(1, &boundsVBO);
    glBindBuffer(GL_ARRAY_BUFFER, boundsVBO);
    glBufferData(GL_ARRAY_BUFFER, 8 * sizeof(float), NULL, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
}

void renderBounds(Bounds b) {
    float vertices[8] = {
        b.x1, b.y1,
        b.x2, b.y1,
        b.x2, b.y2,
        b.x1, b.y2,
    };
    glBindVertexArray(boundsVAO);
    glBindBuffer(GL_ARRAY_BUFFER, boundsVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    glUseProgram(shaders[SHADER_DEFAULT]);
    glm::vec4 color = RED;
    glUniform4f(uniforms[U_DEFAULT_COLOR], color.r, color.g, color.b, 1.0f);
    glDrawArrays(GL_LINE_LOOP, 0, 4);
}
#endif

#ifdef DEBUG_RENDER
void printRectVertex(vector<RectVertex> rv) {
    uint32_t i = 0;
    for (
        vector<RectVertex>::iterator j = rv.begin();
        j != rv.end();
        ++j, ++i
    ) {
        if (i % 6 == 0) {
            cout << "\nDEBUG: RECTANGLE:";
        }

        cout << "\n    RectVertex { x: " << j->x << ", y: " << j->y
            << ", w: " << j->w << ", h: " << j->h
            << ", colorIndex: " << j->colorIndex << ", borderRadius: "
            << j->borderRadius << " }";
    }
}
#endif

#ifdef DEBUG_TEXT_SELECTION
    float debug_lastTextSelTime = 0;
    float debug_lastTid = 0;

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

