#ifndef PATRAM_UI_SHADER
#define PATRAM_UI_SHADER

#include <string>

#define TOTAL_SHADERS 5
#define TOTAL_VERTEX_SHADERS 3
#define TOTAL_FRAGMENT_SHADERS 5

using namespace std;

enum Shaders {
    SHADER_DEFAULT,
    SHADER_TEXT,
    SHADER_ICON_BUTTON,
    SHADER_BUTTON,
    SHADER_TEXT_INPUT,
};

struct Shader {
    uint32_t programId;
    uint8_t vsIndex;
    uint8_t fsIndex;
};

extern Shader shaders[TOTAL_SHADERS];

void initShaders();

#endif // PATRAM_UI_SHADER

