#ifndef PATRAM_UI_SHADER
#define PATRAM_UI_SHADER

#include <string>

#define TOTAL_SHADERS 1
#define TOTAL_VERTEX_SHADERS 1
#define TOTAL_FRAGMENT_SHADERS 1

using namespace std;

enum Shaders {
    SHADER_DEFAULT,
};

struct Shader {
    uint32_t programId;
    uint8_t vsIndex;
    uint8_t fsIndex;
};

extern Shader shaders[TOTAL_SHADERS];

void initShaders();

#endif // PATRAM_UI_SHADER

