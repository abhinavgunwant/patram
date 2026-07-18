#ifndef PATRAM_UI_SHADER
#define PATRAM_UI_SHADER

#include <string>

// #define DEBUG_SHADER
#define TOTAL_SHADERS 8
#define TOTAL_VERTEX_SHADERS 5
#define TOTAL_FRAGMENT_SHADERS 7
#define TOTAL_UNIFORMS 34

#define U_DEFAULT_COLOR             0
#define U_DEFAULT_RESOLUTION        1
#define U_TEXT_COLOR                2
#define U_TEXT_RESOLUTION           3
#define U_ICON_BUTTON_COLOR         4
#define U_ICON_BUTTON_RESOLUTION    5
#define U_ICON_BUTTON_BG_COLOR      6
#define U_ICON_BUTTON_HOV_COLOR     7
#define U_ICON_BUTTON_WIDTH         8
#define U_ICON_BUTTON_HEIGHT        9
#define U_ICON_BUTTON_TOP_LEFT      10
#define U_ICON_BUTTON_CURSOR        11
#define U_ICON_BUTTON_TEXTURE       12
#define U_BUTTON_BG_COLOR           13
#define U_BUTTON_HOV_COLOR          14
#define U_BUTTON_BORDER_RADIUS      15
#define U_BUTTON_TOP_LEFT           16
#define U_BUTTON_RESOLUTION         17
#define U_BUTTON_DIMENSIONS         18
#define U_BUTTON_CURSOR             19
#define U_TEXT_INPUT_COLOR          20
#define U_TEXT_INPUT_BG_COLOR       21
#define U_TEXT_INPUT_HOV_COLOR      22
#define U_TEXT_INPUT_BORDER_RADIUS  23
#define U_TEXT_INPUT_TOP_LEFT       24
#define U_TEXT_INPUT_RESOLUTION     25
#define U_TEXT_INPUT_DIMENSIONS     26
#define U_TEXT_INPUT_CURSOR         27
#define U_TEXT_CURSOR_COLOR         28
#define U_TEXT_CURSOR_RESOLUTION    29
#define U_TEXT_CURSOR_TIME          30
#define U_TEXT_HL_COLOR             31
#define U_TEXT_HL_RESOLUTION        32
#define U_TEXT_HL_SCROLL_OFFSET_Y   33

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

