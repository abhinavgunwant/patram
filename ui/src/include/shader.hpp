#ifndef PATRAM_UI_SHADER
#define PATRAM_UI_SHADER

#include <string>

// #define DEBUG_SHADER
#define TOTAL_SHADERS 6
#define TOTAL_VERTEX_SHADERS 3
#define TOTAL_FRAGMENT_SHADERS 6

using namespace std;

enum Shaders {
    SHADER_DEFAULT,
    SHADER_TEXT,
    SHADER_ICON_BUTTON,
    SHADER_BUTTON,
    SHADER_TEXT_INPUT,
    SHADER_TEXT_CURSOR,
};

struct Shader {
    uint32_t programId;
    uint8_t vsIndex;
    uint8_t fsIndex;
};

extern Shader shaders[TOTAL_SHADERS];

void initShaders();

#endif // PATRAM_UI_SHADER

