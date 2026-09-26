#ifndef PATRAM_UI_SHADER
#define PATRAM_UI_SHADER

#include <string>

// #define DEBUG_SHADER
#define TOTAL_SHADERS 9
#define TOTAL_VERTEX_SHADERS 5
#define TOTAL_FRAGMENT_SHADERS 8
#define TOTAL_UNIFORMS 27

#define U_DEFAULT_COLOR             0
#define U_TEXT_COLOR                1
#define U_ICON_BUTTON_COLOR         2
#define U_ICON_BUTTON_BG_COLOR      3
#define U_ICON_BUTTON_HOV_COLOR     4
#define U_ICON_BUTTON_WIDTH         5
#define U_ICON_BUTTON_HEIGHT        6
#define U_ICON_BUTTON_TOP_LEFT      7
#define U_ICON_BUTTON_CURSOR        8
#define U_ICON_BUTTON_TEXTURE       9
#define U_BUTTON_BG_COLOR           10
#define U_BUTTON_HOV_COLOR          11
#define U_BUTTON_BORDER_RADIUS      12
#define U_BUTTON_TOP_LEFT           13
#define U_BUTTON_DIMENSIONS         14
#define U_BUTTON_CURSOR             15
#define U_TEXT_INPUT_COLOR          16
#define U_TEXT_INPUT_BG_COLOR       17
#define U_TEXT_INPUT_HOV_COLOR      18
#define U_TEXT_INPUT_BORDER_RADIUS  19
#define U_TEXT_INPUT_TOP_LEFT       20
#define U_TEXT_INPUT_DIMENSIONS     21
#define U_TEXT_INPUT_CURSOR         22
#define U_TEXT_CURSOR_COLOR         23
#define U_TEXT_CURSOR_TIME          24
#define U_TEXT_HL_COLOR             25
#define U_TEXT_HL_SCROLL_OFFSET_Y   26

#define COMMON_UNIFORMS_UBO_INDEX           0
#define RECTANGLE_COLOR_INDEX_UBO_INDEX     1
#define RECTANGLE_COLOR_INDEX_COUNT         4

using namespace std;

enum Shaders {
    SHADER_DEFAULT,
    SHADER_TEXT,
    SHADER_ICON_BUTTON,
    SHADER_BUTTON,
    SHADER_TEXT_INPUT,
    SHADER_TEXT_CURSOR,
    SHADER_CB_TEXT_HIGHLIGHT,
    SHADER_RECTANGLE,
    SHADER_IMAGE,
};

struct Shader {
    uint32_t programId;
    uint8_t vsIndex;
    uint8_t fsIndex;
};

extern uint32_t uniforms[TOTAL_UNIFORMS];
extern Shader shaders[TOTAL_SHADERS];

void initShaders();

#endif // PATRAM_UI_SHADER

