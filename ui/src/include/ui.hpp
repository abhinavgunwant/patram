#ifndef PATRAM_UI
#define PATRAM_UI

#include <string>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define INITIAL_PARENT_CAPACITY     8
#define GROWTH_FACTOR               2
#define GROWTH_THRESHOLD            1000
#define INCR_POST_GROWTH_THRESHOLD  128

#define TOTAL_ICONS                 3

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

//// Dark theme colors
#define ICON_BUTTON_BG_DARK         glm::vec4(0.19, 0.17, 0.17, 1.0)
// #define ICON_BUTTON_BG_DARK         glm::vec4(0.22, 0.19, 0.20, 1.0)
#define ICON_BUTTON_HOVER_BG_DARK   glm::vec4(0.31, 0.27, 0.28, 1.0)
#define ICON_BUTTON_COLOR_DARK      glm::vec4(0.8, 0.75, 0.79, 1.0)

#define BUTTON_BG_DARK              glm::vec4(0.19, 0.17, 0.17, 1.0)
#define BUTTON_HOVER_BG_DARK        glm::vec4(0.31, 0.27, 0.28, 1.0)
#define BUTTON_COLOR_DARK           glm::vec4(0.8, 0.75, 0.79, 1.0)


#define BUTTON_PADDING              8

using namespace std;

extern float iconButtonVertices[16];
extern bool darkTheme;

enum UIElementType {
    UI_LABEL,
    UI_BUTTON,
    UI_ICON_BUTTON,
    UI_CONTAINER,
    UI_TEXT_AREA,
    UI_INPUT,
    UI_TAB_BAR,
    UI_TAB,
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
};

enum Icon {
    ICON_NONE,
    ICON_PLUS,
    ICON_FILE,
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

    /**
     * Click behaviour for buttons.
     */
    void (*onClick)(UIElement*);
    void (*onHover)(UIElement*);

    string text;

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

    UIPreferences * preferences;
};

void defaultCallback(UIElement* elem);
void initElement(UIElement& elem, UIElementType type, float top, float left, float width, float height);
UIPreferences* initPreferences();
void addChild(UIElement& parent, UIElement* child);
void recalculateLayout(UIElement * elem);
void renderElement(UIElement& elem, glm::vec2 &resolution, glm::vec2 &cursor);
string getIconLocation(Icon icon);

#endif // PATRAM_UI

