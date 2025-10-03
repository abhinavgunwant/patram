#ifndef PATRAM_UI_SHADER
#define PATRAM_UI_SHADER

#include <string>

#define TOTAL_SHADERS 2
#define TOTAL_VERTEX_SHADERS 2
#define TOTAL_FRAGMENT_SHADERS 2

using namespace std;

enum Shaders {
    SHADER_DEFAULT,
    SHADER_ICON_BUTTON,
};

struct Shader {
    uint32_t programId;
    uint8_t vsIndex;
    uint8_t fsIndex;
};

extern Shader shaders[TOTAL_SHADERS];

void initShaders();

#endif // PATRAM_UI_SHADER

