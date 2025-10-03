#ifndef PATRAM_UI
#define PATRAM_UI

#include <string>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace std;

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
void renderElement(UIElement& elem, glm::vec2 &resolution);

#endif // PATRAM_UI

