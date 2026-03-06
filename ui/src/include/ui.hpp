#ifndef PATRAM_UI
#define PATRAM_UI

#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "command.hpp"
#include "rope.hpp"

#define INITIAL_PARENT_CAPACITY     8
#define GROWTH_FACTOR               2
#define GROWTH_THRESHOLD            1000
#define INCR_POST_GROWTH_THRESHOLD  128

#define TOTAL_ICONS                 3

#define CONTENT_BLOCK_MARGIN        1.0f
#define CONTENT_BLOCK_TITLE_MARGIN  5.0f
#define CONTENT_BLOCK_DRAGTAB_WIDTH 20.0f
#define CONTENT_MAX_WIDTH           760.0f
#define CONTENT_BREAKPOINT          800
#define CONTENT_HORIZONTAL_MARGIN   20.0f
#define CONTENT_HIGHLIGHT_BG        glm::vec4(0.85f, 0.85f, 0.85f, 1.0)
#define CONTENT_HIGHLIGHT_BG_DARK   glm::vec4(0.15f, 0.15f, 0.15f, 1.0)
#define CONTENT_HIGHLIGHT_DRAG      glm::vec4(0.75f, 0.75f, 0.75f, 1.0)
#define CONTENT_HIGHLIGHT_DRAG_DARK glm::vec4(0.25f, 0.25f, 0.25f, 1.0)

#define CONTENT_BLOCK_VISIBLE       0b00000001
#define CONTENT_BLOCK_REDRAW        0b00000010
#define CONTENT_BLOCK_DRAGGING      0b00000100
#define CONTENT_BLOCK_EDITING       0b00001000

#define CURRENT_TAB                 (&tabs[activeTabIndex])

// Generic colors
#define WHITE               glm::vec4(1.0, 1.0, 1.0, 1.0)
#define BLACK               glm::vec4(0.0, 0.0, 0.0, 1.0)
#define TRANSPARENT         glm::vec4(0.0, 0.0, 0.0, 0.0)

// Theme colors
//// Light theme colors
#define ICON_BUTTON_BG              glm::vec4(0.93, 0.84, 0.85, 1.0)
#define ICON_BUTTON_HOVER_BG        glm::vec4(0.8, 0.7, 0.71, 1.0)
#define ICON_BUTTON_COLOR           glm::vec4(0.5, 0.45, 0.4, 1.0)

#define BUTTON_BG                   glm::vec4(0.93, 0.7, 0.7, 1.0)
#define BUTTON_HOVER_BG             glm::vec4(0.8, 0.7, 0.71, 1.0)
#define BUTTON_COLOR                glm::vec4(0.5, 0.45, 0.4, 1.0)

#define CURSOR_COLOR                glm::vec4(0.0, 0.38, 0.65, 1.0)

//// Dark theme colors
#define ICON_BUTTON_BG_DARK         glm::vec4(0.19, 0.17, 0.17, 1.0)
// #define ICON_BUTTON_BG_DARK         glm::vec4(0.22, 0.19, 0.20, 1.0)
#define ICON_BUTTON_HOVER_BG_DARK   glm::vec4(0.31, 0.27, 0.28, 1.0)
#define ICON_BUTTON_COLOR_DARK      glm::vec4(0.8, 0.75, 0.79, 1.0)

#define BUTTON_BG_DARK              glm::vec4(0.19, 0.17, 0.17, 1.0)
#define BUTTON_HOVER_BG_DARK        glm::vec4(0.31, 0.27, 0.28, 1.0)
#define BUTTON_COLOR_DARK           glm::vec4(0.8, 0.75, 0.79, 1.0)

#define CURSOR_COLOR_DARK           glm::vec4(0.0, 0.93, 1.0, 1.0)

#define BUTTON_PADDING              8
#define LINE_GAP                    3

#define SCROLL_BAR_THICKNESS        10.0f
#define SCROLL_DEBOUNCE_DELAY       0.00833f

#define SCROLL_BAR_TRACK_COLOR      glm::vec4(0.9f, 0.9f, 0.9f, 1.0f)
#define SCROLL_BAR_TRACK_COLOR_DARK glm::vec4(0.1f, 0.1f, 0.1f, 1.0f)

#define THROTTLE_FAST               0.1
// #define THROTTLE_NORMAL             0.18
// #define THROTTLE_SLOW               0.25

#define DEBUG_TEXT_SELECTION
#define DEBUG_TEXT_INPUT
#define DEBUG_MOUSE_BUTTON
// #define DEBUG_TEXTURE
// #define DEBUG_SCROLL_AREA
// #define DEBUG_LAYOUT
// #define DEBUG_RENDER
#define DEBUG_CONTENT_BLOCKS

using namespace std;

struct ContentTab;
struct UIElement;

extern float iconButtonVertices[16];
extern bool darkTheme;

extern float contentLeft;
extern float contentRight;

struct HighlightedContentBlock;
struct ContentBlockMoveMarker;

extern HighlightedContentBlock highlightedContentBlock;
extern ContentBlockMoveMarker cbMove;

extern vector<ContentTab> tabs;
extern uint8_t activeTabIndex;
extern UIElement* contentContainer;
extern UIElement* activeElement;

enum UIElementType {
    UI_LABEL,
    UI_BUTTON,
    UI_SCROLL_AREA,
    UI_ICON_BUTTON,
    UI_CONTAINER,
    UI_CONTENT_CONTAINER,
    UI_TEXT_INPUT,
    UI_TEXT_AREA,
    UI_TAB_BAR,
    UI_TAB,
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

enum UIAlignment {
    UI_ALIGN_START,
    UI_ALIGN_END,
    UI_ALIGN_CENTER,
    UI_ALIGN_SPACE_BETWEEN,
    UI_ALIGN_SPACE_AROUND,
};

enum UISizePreference {
    UI_FIXED,
    UI_PERCENT,
    UI_STRETCH,
    UI_FULL,
    UI_GROW,
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

    float mouseDownX;
    float mouseDownY;
    /** The glfwGetTime() when the cursor last moved. */
    float lastActiveTime;

    CommandList cmdList;

    ~TextInputData() {
        cmdList.~CommandList();
    }
};

struct SelectionData {
    uint32_t cursorPosition;
    uint32_t row;
    uint32_t col;
    float x;
    float y;
    uint16_t height;
};

struct ScrollData {
    float xOffset;
    float yOffset;
    float width;
    float height;

    float lastScrolled;

    bool showVerticalScrollbar;
    bool showHorizontalScrollbar;
    bool renderedUpdate;

    uint32_t verticalScrollBarVAO;
    uint32_t verticalScrollBarVBO;
    uint32_t horizontalScrollBarVAO;
    uint32_t horizontalScrollBarVBO;

    uint32_t verticalScrollThumbVAO;
    uint32_t verticalScrollThumbVBO;
    uint32_t horizontalScrollThumbVAO;
    uint32_t horizontalScrollThumbVBO;
};

union UIExtraData {
    TextInputData textInputData;
    ScrollData scrollData;

    ~UIExtraData() {
        textInputData.~TextInputData();
    }
};

struct ContentBlock;

struct UIElement {
    UIElementType type;

    float top;
    float left;
    float width;
    float height;

    bool active;
    bool disabled;
    bool autosizeChildren;
    bool reDraw;

    // void (*onClick)(UIElement*);
    // void (*onMouseDown)(UIElement*, float, float, glm::vec2);
    // void (*onMouseDrag)(UIElement*, float, float, glm::vec2);
    // void (*onMouseUp)(UIElement*, float, float, glm::vec2);

    string text;

    // Rope* rope;

    UIElement* parent;

    // Properties for children (if any)
    uint32_t totalChildren;
    uint32_t capacity;
    UIElement** children;

    // Colors
    glm::vec4 backgroundColor;
    glm::vec4 hoverColor;
    glm::vec4 color;

    uint32_t vao;
    uint32_t vbo;
    uint32_t texture;

    Icon icon;

    uint8_t fontSize;
    uint8_t borderRadius;

    UIPositioning positioning;

    UIPreferences * preferences;
    UIExtraData * extraData;
};

struct ContentBlock {
    ContentBlockType type;

    uint8_t flags;

    float top;
    float left;
    float height;

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
    uint32_t colorU;

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

    float lastCursorTime;
    float lastScrolled;
    float scrollOffsetY;
    float maxHeight;

    uint32_t activeCBIndex;
    uint32_t cursorCBIndex;
    uint32_t cursorPosition;
    uint32_t initialCursorPosition;

    uint32_t selStartRow;
    uint32_t selStartCol;
    uint32_t selEndRow;
    uint32_t selEndCol;

    uint16_t cursorX;
    uint32_t currentCommand;

    uint32_t selVAO;
    uint32_t selVBO;
    uint32_t cursorVAO;
    uint32_t cursorVBO;
    uint32_t scrollbarVAO;
    uint32_t scrollbarVBO;
    uint32_t scrollThumbVAO;
    uint32_t scrollThumbVBO;

    vector<ContentCommand> commands;
    vector<ContentBlock> contentBlocks;
};

void defaultCallback(UIElement* elem);
void defaultKeyCallback(UIElement*, int, int, int);
void defaultOnMouseUpDownCallback(UIElement*, float, float, glm::vec2);

void cbKeyCallback(int, int, int);
void textInputKeyCallback(UIElement*, int, int, int);
void textInputMouseDownCallback(UIElement* elem, float mouseX, float mouseY, glm::vec2 res);
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
void newTab(string title = "Untitled");
void closeTab(uint32_t index);
UIPreferences* initPreferences();
void initializeCapacity(UIElement* parent, int capacity);
void growCapacity(UIElement* parent);
void addChild(UIElement* parent, UIElement* child);
void recalculateLayout(UIElement * elem);
void renderElement(UIElement& elem, glm::vec2 &resolution, glm::vec2 &cursor);
void renderContentBlock(ContentTab* tab, vector<ContentBlock>::iterator cb, glm::vec2 &resolution, glm::vec2 &cursor);
void renderContentBlockHighlight();
void contentBlockMouseDownCallback(UIElement* elem, float mouseX, float mouseY, glm::vec2 res);
void renderCBMoveMarker();
void hideCBMoveMarker();
void reDrawAllContentBlocks();
UIElement* getElementFromScreenCoords(UIElement * root, float x, float y);
void processCBCommand(ContentCommand cmd);
void processCommand(UIElement* elem, Command* cmd);
string getIconLocation(Icon icon);

bool isPointInside(float pointX, float pointY, float left, float right, float top, float bottom);

void getScrollbarDimensions(
    UIElement* elem,
    float& top, float& right, float& bottom, float& left,
    float& thumbStart, float& thumbEnd,
    bool vertical
);

void getCBScrollbarDimensions(
    ContentTab* tab, float& top, float& right, float& bottom, float& left,
    float& thumbStart, float& thumbEnd
);

/** Returns string representation of the element type. */
string elementType(UIElement* elem);

#ifdef DEBUG_TEXT_SELECTION
void printTextSelectionVertices(vector<float> &fvec);
void printTextInputData(TextInputData* tid, string token = "");
#endif

#ifdef DEBUG_CONTENT_BLOCKS
void printContentBlocks(uint32_t tabIndex);
#endif

#endif // PATRAM_UI

