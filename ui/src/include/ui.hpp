#ifndef PATRAM_UI
#define PATRAM_UI

#include <filesystem>
#include <string>
#include <vector>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "command.hpp"
#include "rope.hpp"
#include "arena.hpp"

#define WINDOW_WIDTH 550
#define WINDOW_HEIGHT 600

#define INITIAL_PARENT_CAPACITY     8
#define GROWTH_FACTOR               2
#define GROWTH_THRESHOLD            1000
#define INCR_POST_GROWTH_THRESHOLD  128

#define TOTAL_ICONS                 3

#define TAB_BAR_HEIGHT              48.0f
#define TAB_VERTICAL_PADDING        13.165f
#define TAB_HORIZONTAL_PADDING      16.0f
#define CONTENT_BLOCK_DRAGTAB_WIDTH 20.0f
#define CONTENT_MAX_WIDTH           760.0f
#define CONTENT_BREAKPOINT          800
#define CONTENT_HORIZONTAL_MARGIN   20.0f

#define CONTENT_BLOCK_VISIBLE       0b00000001
#define CONTENT_BLOCK_REDRAW        0b00000010
#define CONTENT_BLOCK_DRAGGING      0b00000100
#define CONTENT_BLOCK_EDITING       0b00001000

#define CURRENT_TAB                 (&tabs[activeTabIndex])

// Generic colors
#define WHITE               glm::vec4(1.0, 1.0, 1.0, 1.0)
#define BLACK               glm::vec4(0.0, 0.0, 0.0, 1.0)
#define TRANSPARENT_COLOR   glm::vec4(0.0, 0.0, 0.0, 0.0)

// Theme colors
//// Light theme colors
#define ROOT_BG                     glm::vec4(1.0, 1.0, 1.0, 1.0)
#define TOOL_BAR_BG                 glm::vec4(0.93, 0.84, 0.85, 1.0)

#define TEXT_INPUT_BG               glm::vec4(1.0, 1.0, 1.0, 1.0)
#define TEXT_INPUT_HOVER_BG         glm::vec4(0.8, 0.7, 0.71, 1.0)

#define ICON_BUTTON_BG              glm::vec4(0.93, 0.84, 0.85, 1.0)
#define ICON_BUTTON_HOVER_BG        glm::vec4(0.8, 0.7, 0.71, 1.0)
#define ICON_BUTTON_COLOR           glm::vec4(0.5, 0.45, 0.4, 1.0)

#define BUTTON_BG                   glm::vec4(0.93, 0.7, 0.7, 1.0)
#define CM_DELETE_BUTTON_BG         glm::vec4(0.66, 0.0, 0.0, 1.0)
#define BUTTON_HOVER_BG             glm::vec4(0.8, 0.7, 0.71, 1.0)
#define BUTTON_COLOR                glm::vec4(0.5, 0.45, 0.4, 1.0)
#define EXPLORER_BG                 glm::vec4(0.82, 0.82, 0.82, 1.0)
#define EXPLORER_HOVER_BG           glm::vec4(0.89, 0.827, 0.827, 1.0)

#define SCROLL_BAR_TRACK_COLOR      glm::vec4(0.9f, 0.9f, 0.9f, 1.0f)
#define SCROLL_BAR_THUMB_COLOR      glm::vec4(0.6f, 0.6f, 0.6f, 1.0f)

#define CURSOR_COLOR                glm::vec4(0.0, 0.38, 0.65, 1.0)

#define CONTENT_HIGHLIGHT_BG        glm::vec4(0.85f, 0.85f, 0.85f, 1.0)
#define CONTENT_HIGHLIGHT_DRAG      glm::vec4(0.75f, 0.75f, 0.75f, 1.0)

//// Dark theme colors
#define ROOT_BG_DARK                glm::vec4(0.16, 0.15, 0.15, 1.0) // #292626
#define TOOL_BAR_BG_DARK            glm::vec4(0.24, 0.22, 0.22, 1.0) // #3D3838

#define TEXT_INPUT_BG_DARK          glm::vec4(0.24, 0.20, 0.20, 1.0) // #3C3434
#define TEXT_INPUT_HOVER_BG_DARK    glm::vec4(0.31, 0.27, 0.28, 1.0)

#define ICON_BUTTON_BG_DARK         glm::vec4(0.19, 0.17, 0.17, 1.0)
#define ICON_BUTTON_HOVER_BG_DARK   glm::vec4(0.31, 0.27, 0.28, 1.0)
#define ICON_BUTTON_COLOR_DARK      glm::vec4(0.8, 0.75, 0.79, 1.0)

#define BUTTON_BG_DARK              glm::vec4(0.19, 0.17, 0.17, 1.0)
#define CM_DELETE_BUTTON_BG_DARK    glm::vec4(0.66, 0.0, 0.0, 1.0)
#define BUTTON_HOVER_BG_DARK        glm::vec4(0.31, 0.27, 0.28, 1.0)
#define BUTTON_COLOR_DARK           glm::vec4(0.8, 0.75, 0.79, 1.0)
#define EXPLORER_BG_DARK            glm::vec4(0.12, 0.1, 0.1, 1.0)
#define EXPLORER_HOVER_BG_DARK      glm::vec4(0.19, 0.18, 0.18, 1.0)

#define SCROLL_BAR_TRACK_COLOR_DARK glm::vec4(0.1f, 0.1f, 0.1f, 1.0f)
#define SCROLL_BAR_THUMB_COLOR_DARK glm::vec4(0.6f, 0.6f, 0.6f, 1.0f)

#define CURSOR_COLOR_DARK           glm::vec4(0.0, 0.93, 1.0, 1.0)

#define CONTENT_HIGHLIGHT_BG_DARK   glm::vec4(0.15f, 0.15f, 0.15f, 1.0)
#define CONTENT_HIGHLIGHT_DRAG_DARK glm::vec4(0.25f, 0.25f, 0.25f, 1.0)

#define EXPLORER_CONTEXT_MENU_PADDING               4

//Floating Container IDs
#define EXPLORER_CONTEXT_MENU_ID                    1

// Button IDs
#define CREATE_VAULT_BUTTON_ID                      11
#define SWITCH_VAULT_BUTTON_ID                      12
#define VAULT_SETUP_CREATE_VAULT_BUTTON_ID          13
#define VAULT_BACK_BUTTON_ID                        14
#define NEW_NOTE_BUTTON_ID                          15
#define NEW_FOLDER_BUTTON_ID                        16
#define SAVE_NOTE_BUTTON_ID                         17
#define TOGGLE_EXPLORER_BUTTON_ID                   18
#define VAULT_SETUP_VAULT_NAME_TEXT_INPUT_ID        19
#define VAULT_SETUP_VAULT_PASSWORD_TEXT_INPUT_ID    20
#define SWITCH_VAULT_OPTION_ID                      21
#define EXPLORER_MENU_RENAME_ID                     22
#define EXPLORER_MENU_DELETE_ID                     23
#define EXPLORER_MENU_NEW_NOTE_ID                   24
#define EXPLORER_MENU_NEW_FOLDER_ID                 25
#define RENAME_DIALOG_OK_BUTTON_ID                  26
#define RENAME_DIALOG_CANCEL_BUTTON_ID              27
#define RENAME_DIALOG_INPUT_ID                      28
#define DELETE_DIALOG_DELETE_BUTTON_ID              29
#define DELETE_DIALOG_CANCEL_BUTTON_ID              30
#define NEW_NOTE_DIALOG_CREATE_BUTTON_ID            31
#define NEW_NOTE_DIALOG_CANCEL_BUTTON_ID            32
#define NEW_NOTE_DIALOG_INPUT_ID                    33
#define NEW_FOLDER_DIALOG_CREATE_BUTTON_ID          34
#define NEW_FOLDER_DIALOG_CANCEL_BUTTON_ID          35
#define NEW_FOLDER_DIALOG_INPUT_ID                  36
#define ERROR_DIALOG_OK_BUTTON_ID                   37
#define OPEN_BUTTON_ID                              100

#define EXPLORER_PANE_ID                            200
#define DIALOG_OVERLAY_ID                           201
#define DIALOG_CONTAINER_ID                         202
#define HORIZONTAL_BUTTON_ROW_ID                    203

#define BUTTON_PADDING                              8
#define EXPLORER_CONTEXT_MENU_BUTTON_PADDING        4
#define TEXT_INPUT_PADDING                          4
#define LINE_GAP                                    3

#define SCROLL_BAR_THICKNESS        10.0f
#define SCROLL_DEBOUNCE_DELAY       0.00833f


#define THROTTLE_FAST               0.1
// #define THROTTLE_NORMAL             0.18
// #define THROTTLE_SLOW               0.25

// #define DEBUG_TEST_NOTE_TAB
#define DEBUG_TEXT_SELECTION
#define DEBUG_TEXT_INPUT
// #define DEBUG_KEYBOARD_EVENTS
// #define DEBUG_MOUSE_BUTTON
// #define DEBUG_TEXTURE
// #define DEBUG_SCROLL_AREA
// #define DEBUG_LAYOUT
// #define DEBUG_RENDER
#define DEBUG_CONTENT_BLOCKS
// #define DEBUG_SCROLL
// #define DEBUG_CURSOR_SHAPE
#define DEBUG_EXPLORER

#define MENU_BAR 0
#define SCROLL_AMOUNT 72

#define DEFAULT_ELEM_LENGTH     17
#define FLOATING_ARENA_CAPACITY 8192

#define ROOT_CONTAINER          elements[0]
#define WELCOME_SCREEN          elements[1]
#define TAB_BAR                 elements[2]
#define MAIN_CONTAINER          elements[3]
#define EXPLORER                elements[4]
#define CONTENT_CONTAINER       elements[5]
#define WELCOME_MAIN_CONTAINER  elements[6]
#define FLOATING_CONTAINER      elements[7]

#define EXPLORER_ELEMENT_HEIGHT 32.0f
#define EXPLORER_FLAG_EXPANDED  0b00000001
#define EXPLORER_FLAG_HOVER     0b00000010
#define EXPLORER_FLAG_ACTIVE    0b00000100

#define GET_FLAG(x, flag)   (x & flag)
#define SET_FLAG(x, flag)   x = x | flag
#define UNSET_FLAG(x, flag) x = x & ~(flag)

using namespace std;

struct ContentTab;
struct UIElement;

extern UIElement elements[DEFAULT_ELEM_LENGTH];

extern float iconButtonVertices[16];
extern bool darkTheme;

extern float contentLeft;
extern float contentRight;

struct HighlightedContentBlock;
struct ContentBlockMoveMarker;

extern HighlightedContentBlock highlightedContentBlock;
extern ContentBlockMoveMarker cbMove;

extern GLFWwindow* window;
extern bool minimized;
extern bool focused;
extern bool showFps;

extern uint8_t* moveCursorBitmap;

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void keyCallback(GLFWwindow* window, int key, int scanCode, int action, int mods);
void resizeCallback(GLFWwindow* window, int width, int height);
void cursorPosCallback(GLFWwindow * window, double xpos, double ypos);
void windowPosCallback(GLFWwindow* window, int x, int y);
void scrollCallback(GLFWwindow * window, double xoffset, double yoffset);
void minimizedCallback(GLFWwindow* window, int minimized);
void maximizedCallback(GLFWwindow* window, int maximized);
void windowFocusCallback(GLFWwindow *window, int focused);

void showWindow();
void toggleFullscreen();
void setupTabs(vector<float>& textVertices, vector<float>& xVertices);
void setupExplorer(vector<float>& vertices, vector<float>& iVertices);
void renameActiveExplorerNode();
void deleteActiveExplorerNode();
void createNoteInActiveExplorerNode();
void createFolderInActiveExplorerNode();
void setupUI(bool darkTheme);
void enterRenderLoop();
extern vector<ContentTab> tabs;
extern uint8_t activeTabIndex;
// extern UIElement* contentContainer;
extern UIElement* activeElement;
extern glm::vec2 resolution;
extern uint16_t windowHeight;
extern uint16_t windowWidth;

enum class CursorShape: uint8_t {
    Auto,
    Default,
    Text,
    Pointer,
    HorizontalResize,
    Move,
};

enum class UIElementType: uint8_t {
    Label,
    Button,
    ScrollArea,
    IconButton,
    Container,
    ContentContainer,
    TextInput,
    TextArea,
    TabBar,
    Tab,
    Explorer,
};

enum class ContentBlockType: uint8_t {
    Title,
    Paragraph,
    Heading1,
    Heading2,
    Heading3,
    Heading4,
    Heading5,
    Heading6,
};

enum class UIAlignment : uint8_t {
    None,
    Start,
    End,
    Center,
    SpaceBetween,
    SpaceAround,
};

enum class UISizePreference : uint8_t {
    Fixed,
    Percent,
    Stretch,
    Full,
    Grow,
};

enum class Icon : uint8_t {
    None,
    Plus,
    File,
};

enum class UIPositioning: uint8_t {
    Absolute,
    Relative,
    Relative_sibling,
};

enum class TextInputAlignment: uint8_t {
    Left,
    Center,
    Right,
};

struct UIPreferences {
    float maxHeight;
    float maxWidth;

    UIAlignment horizontalAlign;
    UIAlignment verticalAlign;

    UISizePreference widthPreference;
    UISizePreference heightPreference;

    /**
     * `true` - row
     * `false` - column
     */
    bool direction;
};

struct TextInputData {
    uint32_t cursorPosition;
    uint32_t cursorVAO;
    uint32_t cursorVBO;

    uint32_t textSelectStartRow;
    uint32_t textSelectStartCol;
    uint32_t textSelectEndRow;
    uint32_t textSelectEndCol;

    uint32_t textSelectVAO;
    uint32_t textSelectVBO;

    /**
     * Useful when selecting text, if it's same as cursorPosition, no text will
     * be marked as "selected".
     */
    uint32_t initialCursorPosition;
    uint32_t initialRow;
    uint32_t initialCol;
    uint32_t prevRow;

    bool textSelected;
    bool mouseDown;

    /**
     * Used for draw calls for rendering text selection.
     */
    bool textSelectUpdated;

    bool password;

    TextInputAlignment alignment;

    float mouseDownX;
    float mouseDownY;
    /** The glfwGetTime() when the cursor last moved. */
    float lastActiveTime;

    CommandList cmdList;

    ~TextInputData() {
        cmdList.~CommandList();
    }
};

enum class SelectionCBPosition : uint8_t {
    None,
    Current,
    Previous,
    Next,
};

struct SelectionData {
    uint32_t cursorPosition;
    uint32_t row;
    uint32_t col;
    uint32_t cursorCBIndex;
    float x;
    float y;
    uint16_t height;
    SelectionCBPosition cbPos;
};

struct ScrollData {
    float xOffset;
    float yOffset;
    float width;
    float height;
    float mouseDownX;
    float mouseDownY;

    float lastScrolled;

    bool showVerticalScrollbar;
    bool showHorizontalScrollbar;
    bool renderedUpdate;
    bool mouseDown;

    uint32_t verticalScrollBarVAO;
    uint32_t verticalScrollBarVBO;
    uint32_t horizontalScrollBarVAO;
    uint32_t horizontalScrollBarVBO;

    uint32_t verticalScrollThumbVAO;
    uint32_t verticalScrollThumbVBO;
    uint32_t horizontalScrollThumbVAO;
    uint32_t horizontalScrollThumbVBO;
};

struct ContentBlock;

struct UIElement {
    UIElementType type;

    uint8_t marginTop;

    uint16_t id;

    float top;
    float left;
    float width;
    float height;

    bool active;
    bool disabled;
    bool show;
    bool reDraw;

    string text;

    UIElement* parent;

    // Properties for children (if any)
    uint32_t totalChildren;
    uint32_t capacity;
    UIElement** children;

    uint32_t vao;
    uint32_t vbo;
    uint32_t texture;

    uint32_t textVAO;
    uint32_t textVBO;
    uint32_t textBufferSize;
    uint32_t textVerticesCount;

    Icon icon;

    uint8_t fontSize;
    uint8_t borderRadius;

    UIPositioning positioning;

    UIPreferences * preferences;
    TextInputData* textInputData;
    ScrollData* scrollData;
};

struct ContentBlock {
    ContentBlockType type;

    uint8_t flags;

    float top;
    float left;
    float height;

    uint32_t rows;

    uint32_t textVAO;
    uint32_t textVBO;
    uint32_t bufferSize;

    Rope* rope;
};

struct HighlightedContentBlock {
    uint32_t cbIndex;

    bool show;
    bool updated;
    bool mouseDown;

    float top;
    float left;
    float bottom;
    float right;

    uint32_t backgroundVAO;
    uint32_t backgroundVBO;
    uint32_t dragTabVAO;
    uint32_t dragTabVBO;
};

struct ContentBlockMoveMarker {
    uint32_t targetIndex;
    uint32_t vao;
    uint32_t vbo;

    bool show;
    bool updated;
};

struct ContentTab {
    bool showCursor;
    bool redrawCursor;
    bool textSelected;
    bool redrawSel;
    bool showScrollbar;
    bool scrollbarUpdated;
    bool updateCursorX;
    bool cbTextMouseDown;
    bool updated;

    float lastCursorTime;
    float lastScrolled;
    float scrollOffsetY;
    float maxHeight;

    uint32_t activeCBIndex;
    uint32_t cursorCBIndex;
    uint32_t cursorPosition;
    uint32_t initialCursorPosition;
    uint32_t initialCBIndex;

    uint32_t selStartCBIndex;
    uint32_t selStartCursorPosition;
    uint32_t selEndCBIndex;
    uint32_t selEndCursorPosition;

    uint32_t cursorRow;
    uint16_t cursorX;
    uint16_t cursorY;
    uint32_t currentCommand;

    uint32_t selVertexCount;
    uint32_t selVAO;
    uint32_t selVBO;
    uint32_t cursorVAO;
    uint32_t cursorVBO;
    uint32_t scrollbarVAO;
    uint32_t scrollbarVBO;
    uint32_t scrollThumbVAO;
    uint32_t scrollThumbVBO;

    uint32_t firstVisibleCBIndex;
    uint32_t visibleCBLength;

    uint16_t xOffset;
    uint16_t width;
    uint16_t closeX;
    uint16_t closeY;

    vector<ContentCommand> commands;
    vector<ContentCommandExt> extCommands;
    vector<ContentBlock> contentBlocks;
    filesystem::path filePath;
};

struct TabsDisplay {
    uint32_t xVao;
    uint32_t xVbo;
    uint32_t xBufferSize;
    uint32_t xVertices;
    uint32_t activeBgVao;
    uint32_t activeBgVbo;
};

extern TabsDisplay tabsDisplay;

enum class ExplorerNodeType: uint8_t {
    Folder          = 0b00000010,
    FolderUpdated   = 0b00000011,
    Note            = 0b00000100,
    NoteUpdated     = 0b00000101,
};

struct ExplorerNode {
    ExplorerNodeType type;
    uint8_t flags;

    uint16_t left;

    float top;

    string name;

    vector<ExplorerNode> children;

    bool operator== (ExplorerNode const &other) const {
        return (
            type == other.type
            && name == other.name
            && children == other.children
        );
    }
};

struct Explorer {
    uint32_t vao;
    uint32_t vbo;
    uint32_t lineVAO;
    uint32_t lineVBO;
    uint32_t hoverBgVao;
    uint32_t hoverBgVbo;
    uint32_t activeBgVao;
    uint32_t activeBgVbo;
    uint32_t verticalScrollVao;
    uint32_t verticalScrollVbo;
    uint32_t horizontalScrollVao;
    uint32_t horizontalScrollVbo;
    uint32_t iconsVAO;
    uint32_t iconsVBO;
    uint32_t textVertices;
    uint32_t textVerCap;
    uint32_t iconsVertices;
    uint32_t iconsVerCap;

    float lastClickedTime;

    bool elementHover;
    bool elementActive;
    bool lineHover;
    bool lineMouseDown;
    bool redrawLine;

    vector<ExplorerNode> nodes;
};

void defaultCallback(UIElement* elem);
void defaultKeyCallback(UIElement*, int, int, int);
void defaultOnMouseUpDownCallback(UIElement*, float, float, glm::vec2);

void cbKeyCallback(int, int, int);
void textInputKeyCallback(UIElement*, int, int, int);
void textInputMouseDownCallback(UIElement* elem, float mouseX, float mouseY, glm::vec2 res);
void tabBarMouseDownCallback(UIElement* elem, float mouseX, float mouseY, glm::vec2 res, int glfwButton);
void textInputMouseDragCallback(UIElement* elem, float mouseX, float mouseY, glm::vec2 res);
void textInputMouseUpCallback(UIElement* elem, float mouseX, float mouseY, glm::vec2 res);
void textInputUnselect(TextInputData * tid, string token = "");

void scrollAreaMouseDownCallback(UIElement*, float, float, glm::vec2);
void scrollAreaMouseDragCallback(UIElement*, float, float, glm::vec2);

/** Creates a command to replace the selected text in a text input. */
Command* replaceSelectedText(UIElement* elem, Command* oldCommand);
SelectionData getTextIndexForScreenCoords(UIElement* elem, float mouseX, float mouseY, glm::vec2 res);
void selectTextWithMouse(UIElement* elem, float mouseX, float mouseY, float mouseDownX, float mouseDownY, bool mouseUp, glm::vec2& res);
void initElement(UIElement& elem, UIElementType type, float top, float left, float width, float height);
void insertContentBlock(uint32_t tabIndex, ContentBlockType type, string text, uint32_t index);
void appendContentBlock(uint32_t tabIndex, ContentBlockType type, string text);
void moveContentBlock(uint32_t tabIndex, uint32_t newIndex, uint32_t oldIndex);
void newTab(filesystem::path tabPath);
void closeTab(uint32_t index);
void closeCurrentTab();
string getSelectedTextFromCurrentTab();
UIPreferences* initPreferences();
void initializeCapacity(UIElement* parent, int capacity, Arena* arena = nullptr);
void growCapacity(UIElement* parent, Arena* arena = nullptr);
void addChild(UIElement* parent, UIElement* child, Arena* arena = nullptr);

void toggleExplorer();
inline bool updateElementDimensionsAccordingToPreferences(UIElement* elem);
void recalculateLayout(UIElement * elem);
void recalculateContentWidth();
void recalculateCBCursor();
void updateCBCursor(ContentTab* tab, SelectionData sd);
void renderElement(UIElement& elem, glm::vec2 &resolution, glm::vec2 &cursor, uint32_t childIndex, float& yPrev);
glm::vec2 getActiveContentDimensions();
void setCBDimensions(vector<ContentBlock>::iterator cb, bool first, bool calculateHeight);
bool renderContentBlock(ContentTab* tab, vector<ContentBlock>::iterator cb, glm::vec2 &resolution, glm::vec2 &cursor);
void renderContentBlockHighlight();
void renderCBTextSelection(ContentTab* tab);
void contentBlockMouseDownCallback(UIElement* elem, float mouseX, float mouseY, glm::vec2 res);
void contentBlockMouseUpCallback(UIElement* elem, float mouseX, float mouseY, glm::vec2 res);
void renderCBMoveMarker();
void hideCBMoveMarker();
void reDrawAllContentBlocks();
void reDrawCBsFrom(uint32_t i, uint32_t limit);
void reDrawCBsFromIter(vector<ContentBlock>::iterator iter, uint32_t limit);

void insertCBs(
    ContentTab* tab,
    uint32_t index,
    vector<ContentBlock>::iterator from,
    vector<ContentBlock>::iterator to,
    uint32_t count
);

void deleteCBs(
    ContentTab* tab,
    vector<ContentBlock>::iterator from,
    vector<ContentBlock>::iterator to
);

void deleteCBsToDeletedBlocks(
    ContentTab* tab,
    vector<ContentBlock>::iterator from,
    vector<ContentBlock>::iterator to,
    ContentCommandExt* extCmd
);

UIElement* getElementById(UIElement * root, uint16_t id);
UIElement* getElementFromScreenCoords(UIElement * root, float x, float y);
void cbKeypressType(ContentTab* tab, string text);
ContentDeletionType deleteSelectedCBText(ContentTab* tab, ContentCommand* cmd, ContentCommandExt* cbExt);
vector<ContentBlock> textToCB(string& text);
void processCBCommand(ContentCommand cmd);
void processCBUndo(ContentTab* tab);
void processCBRedo(ContentTab* tab);
void processCommand(UIElement* elem, Command* cmd);
string getIconLocation(Icon icon);

bool isPointInside(float pointX, float pointY, float left, float right, float top, float bottom);

void getScrollbarDimensions(
    UIElement* elem,
    float& top, float& right, float& bottom, float& left,
    float& thumbStart, float& thumbEnd,
    bool vertical
);

uint8_t getCBFontSize(ContentBlockType cbType);
glm::vec2 getCBVerticalMargins(ContentBlockType cbType);

void getCBScrollbarDimensions(
    ContentTab* tab, float& top, float& right, float& bottom, float& left,
    float& thumbStart, float& thumbEnd
);

SelectionData getCBTextCursorFromScreenCoords(float mouseX, float mouseY,  glm::vec2 resolution);
SelectionData getCBTextCursorVertical(ContentTab* tab, bool page, bool up);

vector<ContentBlock>::iterator getFirstVisibleCB(ContentTab* tab);
vector<ContentBlock>::iterator getLastVisibleCB(ContentTab* tab);

void openFile(bool openDialog, filesystem::path p = "");
void saveFile();
void newNote(filesystem::path p = "");
void updateWindowTitle();

/** Returns string representation of the element type. */
string elementType(UIElement* elem);

void initFloatingUI();
void destroyFloatingUI();
void showExplorerContextMenu(float x, float y);
void showRenameDialog();
void showDeleteDialog();
void showNewFolderDialog();
void showNewNoteDialog();
void showErrorDialog(string title, string message);
void hideContextMenus();

#ifdef DEBUG_TEXT_SELECTION
void printTextSelectionVertices(vector<float> &fvec);
void printTextInputData(TextInputData* tid, string token = "");
#endif

#ifdef DEBUG_CONTENT_BLOCKS
string to_string(ContentBlock cb);
string to_string(ContentBlockType cbType);
void printContentBlocks(uint32_t tabIndex);
void printContentBlocksFromVector(vector<ContentBlock> cblock);
#endif

#endif // PATRAM_UI

