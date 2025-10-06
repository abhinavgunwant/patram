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

// Generic colors
#define WHITE               glm::vec4(1.0, 1.0, 1.0, 1.0)
#define BLACK               glm::vec4(0.0, 0.0, 0.0, 0.0)

// Theme colors
//// Light theme colors
#define ICON_BUTTON_BG      glm::vec4(0.84, 0.80, 0.81, 1.0)
#define ICON_BUTTON_HOVER_BG      glm::vec4(0.9, 0.85, 0.86, 1.0)

//// Dark theme colors
#define ICON_BUTTON_BG_DARK glm::vec4(0.19, 0.17, 0.17, 1.0)
// #define ICON_BUTTON_BG_DARK glm::vec4(0.22, 0.19, 0.20, 1.0)
#define ICON_BUTTON_HOVER_BG_DARK      glm::vec4(0.31, 0.27, 0.28, 1.0)

using namespace std;

extern float iconButtonVertices[8];
extern bool darkTheme;

enum UIElementType {
    UI_LABEL,
    UI_BUTTON,
    UI_ICON_BUTTON,
    UI_CONTAINER,
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
     * `true` - row
     * `false` - column
     */
    bool direction;

    void (*onClick)(UIElement);
    void (*onHover)(UIElement);

    string text;

    UIElement* parent;

    // Properties for children (if any)
    UIAlignment horizontalAlign;
    UIAlignment verticalAlign;
    uint32_t totalChildren;
    uint32_t capacity;
    UIElement** children;

    // Colors
    glm::vec4 backgroundColor;
    glm::vec4 hoverColor;
    glm::vec4 color;

    uint32_t vao;
    uint32_t vbo;

    UISizePreference widthPreference;
    UISizePreference heightPreference;
};

void defaultCallback(UIElement);
void initElement(UIElement& elem, UIElementType type, float top, float left, float width, float height);
void addChild(UIElement& parent, UIElement* child);
void recalculateLayout(UIElement * elem);
void renderElement(UIElement& elem, glm::vec2 &resolution, glm::vec2 &cursor);

#endif // PATRAM_UI

