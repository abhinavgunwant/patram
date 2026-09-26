#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "include/shader.hpp"

using namespace std;

uint32_t uniforms[TOTAL_UNIFORMS];

string vertexShaders[] = {
    // Default Shader
    "#version 330 core\n"
    "layout (location = 0) in vec2 aPos;\n"
    "layout (std140) uniform CommonUniforms {\n"
        "vec2 resolution;"
    "};\n"
    "void main() {\n"
        "vec2 uv = aPos / resolution * 2;\n"
        "gl_Position = vec4(uv.x - 1, 1 - uv.y, 0.0f, 1.0f);\n"
    "}\0",

    // Text Shader
    "#version 330 core\n"
    "layout (location = 0) in vec4 aPos;\n"
    "out vec2 TexCoords;\n"
    "layout (std140) uniform CommonUniforms {\n"
        "vec2 resolution;"
    "};\n"
    "void main() {\n"
        "vec2 uv = aPos.xy / resolution * 2;\n"
        "gl_Position = vec4(uv.x - 1, 1 - uv.y, 0.0f, 1.0f);\n"
        "TexCoords = aPos.zw;\n"
    "}\0",

    // Icon Button Shader
    "#version 330 core\n"
    "layout (location = 0) in vec2 aPos;\n"
    "layout (location = 1) in vec2 aTexCoords;\n"
    "layout (std140) uniform CommonUniforms {\n"
        "vec2 resolution;"
    "};\n"
    "uniform vec2 topLeft;\n"
    "out vec2 TexCoords;\n"
    "void main() {\n"
        "vec2 uv = (aPos + topLeft) / resolution * 2;\n"
        "gl_Position = vec4(uv.x - 1, 1 - uv.y, 0.0f, 1.0f);\n"
        "TexCoords = aTexCoords;\n"
    "}\0",

    // CB Text Highlight
    "#version 330 core\n"
    "layout (location = 0) in vec2 aPos;\n"
    "layout (std140) uniform CommonUniforms {\n"
        "vec2 resolution;"
    "};\n"
    "uniform float scrollOffsetY;\n"
    "void main() {\n"
        "vec2 uv = vec2(aPos.x, aPos.y - scrollOffsetY) / resolution * 2;\n"
        "gl_Position = vec4(uv.x - 1, 1 - uv.y, 0.0f, 1.0f);\n"
    "}\0",

    // Rectangle shader
    "#version 330 core\n"
    "layout (location = 0) in vec2 aPos;\n"
    "layout (location = 1) in uint colorIndex;\n"
    "layout (std140) uniform ColorsUniforms {\n"
        "vec4 colors[4];\n"
    "};\n"
    "layout (std140) uniform CommonUniforms {\n"
        "vec2 resolution;"
    "};\n"
    "out vec4 color;"
    "void main() {\n"
        "color = colors[colorIndex];\n"
        "vec2 uv = aPos / resolution * 2;\n"
        "gl_Position = vec4(uv.x - 1, 1 - uv.y, 0.0f, 1.0f);\n"
    "}\0",
};

string fragmentShaders[] = {
    // Default Shader
    "#version 330 core\n"
    "out vec4 FragColor;\n"
    "uniform vec4 solidColor;\n"
    "void main() {\n"
        "FragColor = solidColor;\n"
    "}\0",

    // Text Shader
    "#version 330 core\n"
    "in vec2 TexCoords;\n"
    "out vec4 color;\n"
    "uniform sampler2D text;\n"
    "uniform vec4 textColor;\n"
    "void main() {\n"
        "vec4 sampled = vec4(1, 1, 1, texture(text, TexCoords).r);\n"
        "color = vec4(textColor.rgb, 1) * sampled;\n"
    "}\0",

    // Icon Button Shader
    "#version 330 core\n"
    "in vec2 TexCoords;\n"
    "out vec4 FragColor;\n"
    "uniform vec4 solidColor;\n"
    "uniform vec4 backgroundColor;\n"
    "uniform vec4 hoverColor;\n"
    "uniform float width;\n"
    "uniform float height;\n"
    "uniform vec2 topLeft;\n"
    "layout (std140) uniform CommonUniforms {\n"
        "vec2 resolution;"
    "};\n"
    "uniform vec2 cursor;\n"
    "uniform sampler2D iconTexture;\n"

    // Final texture color
    "vec4 ftc(vec4 texCol, vec4 color, vec4 bgColor) {\n"
        "vec4 newColor = texCol;\n"
        "float alpha = newColor.a;\n"
        "newColor = 1 - newColor;\n"
        "newColor *= color;\n"

        "if (alpha != 0) {\n"
            "newColor = color;\n"
        "}\n"

        "newColor.a = alpha;\n"
        "return newColor;\n"
    "}\n"

    "void main() {\n"
        "vec4 texColor = texture(iconTexture, TexCoords);\n"

        "bool isForeg = texColor.a > 0;\n"

        "if (texColor.a != 0) {\n"
            "FragColor = ftc(texColor, solidColor, hoverColor);\n"
            "return;\n"
        "}\n"

        "vec2 uv = gl_FragCoord.xy;\n"
        "uv.y = resolution.y - uv.y;\n"
        "float width_2 = width/2;\n"
        "vec2 mid = topLeft;\n"
        "mid.x += width_2;\n"
        "mid.y += height/2;\n"

        "float d_uv_mid = distance(uv, mid);\n"

        "if (d_uv_mid > width_2) {\n"
            "FragColor = vec4(0);\n"
            "return;\n"
        "}\n"

        "float d_cursor_uv = distance(cursor, uv);\n"

        "if (d_cursor_uv > width * 1.5) {\n"
            "FragColor = ftc(texColor, solidColor, hoverColor);\n"
            "return;\n"
        "}\n"

        "float radians = clamp(d_cursor_uv / (width * 0.9), 0, 1.5708);\n"
        "FragColor = mix(backgroundColor, hoverColor, cos(radians));\n"
    "}\0",

    // Button rounded border shader
    "#version 330 core\n"
    "out vec4 FragColor;\n"

    "uniform vec4 backgroundColor;\n"
    "uniform vec4 hoverColor;\n"
    "uniform float borderRadius;\n"
    "uniform vec2 topLeft;\n"
    "layout (std140) uniform CommonUniforms {\n"
        "vec2 resolution;"
    "};\n"
    "uniform vec2 dimensions;\n" // width, height
    "uniform vec2 cursor;\n"

    "void main() {\n"
        "vec2 uv = gl_FragCoord.xy;"
        "uv.y = resolution.y - uv.y;\n"
        "vec2 br = topLeft + dimensions;\n"

        "vec2 tl_plus_br = topLeft + borderRadius;\n"
        "vec2 br_minus_br = br - borderRadius;\n"

        "bool hover = cursor.x > topLeft.x && cursor.x < br.x && cursor.y > topLeft.y && cursor.y < br.y;\n"

        "if ("
            "(uv.x >= tl_plus_br.x && uv.x <= br_minus_br.x)\n"
            "|| (uv.y >= tl_plus_br.y && uv.y <= br_minus_br.y)\n"
        ") {\n"
            "if (hover) {\n"
                "FragColor = hoverColor;\n"
                "return;\n"
            "}\n"

            "FragColor = backgroundColor;\n"
            "return;\n"
        "} else {\n"
            "if (uv.x < tl_plus_br.x && uv.y < tl_plus_br.y) {\n"
                "float d = distance(uv, tl_plus_br);"
                "float w = fwidth(d);"
                "float a = smoothstep(borderRadius + w, borderRadius - w - 0.00001, d);"
                "if (hover) {\n"
                    "FragColor = vec4(hoverColor.rgb, a);\n"
                "} else {\n"
                    "FragColor = vec4(backgroundColor.rgb, a);\n"
                "}\n"
            "} else if (uv.x >= br_minus_br.x && uv.y <= tl_plus_br.y) {\n"
                "float d = abs(distance(uv, vec2(br_minus_br.x, tl_plus_br.y)));\n"
                "float w = fwidth(d);"
                "float a = smoothstep(borderRadius + w, borderRadius - w - 0.00001, d);"
                "if (hover) {\n"
                    "FragColor = vec4(hoverColor.rgb, a);\n"
                "} else {\n"
                    "FragColor = vec4(backgroundColor.rgb, a);\n"
                "}\n"
            "} else if (uv.x > br_minus_br.x && uv.y > br_minus_br.y) {\n"
                "float d = abs(distance(br_minus_br, uv));\n"
                "float w = fwidth(d);"
                "float a = smoothstep(borderRadius + w, borderRadius - w - 0.00001, d);"
                "if (hover) {\n"
                    "FragColor = vec4(hoverColor.rgb, a);\n"
                "} else {\n"
                    "FragColor = vec4(backgroundColor.rgb, a);\n"
                "}\n"
            "} else {\n"
                "float d = abs(distance(uv, vec2(tl_plus_br.x, br_minus_br.y)));\n"
                "float w = fwidth(d);"
                "float a = smoothstep(borderRadius + w, borderRadius - w - 0.00001, d);"
                "if (hover) {\n"
                    "FragColor = vec4(hoverColor.rgb, a);\n"
                "} else {\n"
                    "FragColor = vec4(backgroundColor.rgb, a);\n"
                "}\n"
            "}\n"
            "return;\n"
        "}\n"

        "FragColor = vec4(0);\n"
    "}\0",

    // Text input shader
    "#version 330 core\n"
    "out vec4 FragColor;\n"

    "uniform vec4 color;\n"
    "uniform vec4 backgroundColor;\n"
    "uniform vec4 hoverColor;\n"
    "uniform float borderRadius;\n"
    "uniform vec2 topLeft;\n"
    "layout (std140) uniform CommonUniforms {\n"
        "vec2 resolution;"
    "};\n"
    "uniform vec2 dimensions;\n" // width, height
    "uniform vec2 cursor;\n"

    "void main() {\n"
        "vec2 uv = gl_FragCoord.xy;"
        "uv.y = resolution.y - uv.y;\n"
        "vec2 br = topLeft + dimensions;\n"

        "if (uv.x < topLeft.x || uv.y < topLeft.y || uv.x > br.x || uv.y > br.y) {\n"
            "FragColor = vec4(0);\n"
            "return;\n"
        "}\n"

        "vec2 tl_plus_br = topLeft + borderRadius;\n"
        "vec2 br_minus_br = br - borderRadius;\n"

        // border colors
        /*
        "if (\n"
            "uv.x >= topLeft.x && uv.x <= topLeft.x + 1\n"
            "|| uv.x <= br.x && uv.x >= br.x - 1\n"
            "|| uv.y >= topLeft.y && uv.y <= topLeft.y + 1\n"
            "|| uv.y <= br.y && uv.y >= br.y - 1\n"
        ") {\n"
            "FragColor = color;\n"
            "return;\n"
        "}\n"
        */

        "if ("
            "(uv.x >= tl_plus_br.x && uv.x <= br_minus_br.x)\n"
            "|| (uv.y >= tl_plus_br.y && uv.y <= br_minus_br.y)\n"
            "|| (uv.x < tl_plus_br.x && uv.y < tl_plus_br.y && (distance(uv.xy, tl_plus_br) <= borderRadius))\n"
            "|| (uv.x > br_minus_br.x && uv.y < tl_plus_br.y && (distance(uv.xy, vec2(br_minus_br.x, tl_plus_br.y)) <= borderRadius))\n"
            "|| (uv.x > br_minus_br.x && uv.y > br_minus_br.y && (distance(uv.xy, br_minus_br) <= borderRadius))\n"
            "|| (uv.x < tl_plus_br.x && uv.y > br_minus_br.y && (distance(uv.xy, vec2(tl_plus_br.x, br_minus_br.y)) <= borderRadius))\n"
        ") {\n"
            "if (cursor.x > topLeft.x && cursor.x < br.x && cursor.y > topLeft.y && cursor.y < br.y) {\n"
                "FragColor = hoverColor;\n"
                "return;\n"
            "}\n"

            "FragColor = backgroundColor;\n"
            "return;\n"
        "}\n"

        "FragColor = vec4(0);\n"
    "}\0",

    // Pulse Shader
    "#version 330 core\n"
    "out vec4 FragColor;\n"
    "uniform vec4 color;\n"
    "uniform float time;\n"
    "void main() {\n"
        // "FragColor = vec4(1.0f, 0.0f, 0.0f, 1.0f);"
        "FragColor = vec4(color.r, color.g, color.b, cos(time * 8) * color.a);\n"
        // "FragColor = color;\n"
    "}\0",

    // Rectangle Shader
    "#version 330 core\n"
    "in vec4 color;\n"
    "out vec4 FragColor;\n"
    "void main() {\n"
        "FragColor = vec4(color.rgb, color.a / 2);\n"
    "}\0"
};

Shader shaders[] = {
    { 0, 0, 0 }, // SHADER_DEFAULT
    { 0, 1, 1 }, // SHADER_TEXT
    { 0, 2, 2 }, // SHADER_ICON_BUTTON
    { 0, 0, 3 }, // SHADER_BUTTON
    { 0, 0, 4 }, // SHADER_TEXT_INPUT
    { 0, 0, 5 }, // SHADER_TEXT_CURSOR
    { 0, 3, 0 }, // SHADER_CB_TEXT_HIGHLIGHT
    { 0, 4, 6 }, // SHADER_RECTANGLE
};

uint32_t compile(uint8_t index, uint8_t vs, uint8_t fs) {
    #ifdef DEBUG_SHADER
        cout << "\nCompiling shaders (" << (int)index + 1 << " out of " << TOTAL_SHADERS << ")";
    #endif
    uint32_t vShaderID = glCreateShader(GL_VERTEX_SHADER);

    if (vShaderID == 0) {
        cerr << "\nError: Could not create vertex shader " << (int)index;
    }

    int success;

    const char * vsCode = vertexShaders[(int)vs].c_str();
    const char * fsCode = fragmentShaders[(int)fs].c_str();

    #ifdef DEBUG_SHADER
        cout << "\nVertex shader code ("<< (int) vs << "): \n" << vsCode << "\n";
        cout << "\nFragment shader code (// " << (int) fs << "): \n" << fsCode << "\n";
    #endif

    glShaderSource(vShaderID, 1, (char const* const*)&vsCode, NULL);
    glCompileShader(vShaderID);
    glGetShaderiv(vShaderID, GL_COMPILE_STATUS, &success);

    if (!success) {
        char log[1024];
        glGetShaderInfoLog(vShaderID, 1024, NULL, log);
        cerr << "\nError: vertexShader(" << vShaderID << ") index " << (int)index
            << "'s compilation failed:\n" << log;

        return 0;
    }

    #ifdef DEBUG_SHADER
        cout << "\ncompiled vertex shader";
    #endif

    uint32_t fShaderID = glCreateShader(GL_FRAGMENT_SHADER);

    if (fShaderID == 0) {
        cerr << "\nError: Could not create fragment shader " << (int)index;
    }

    glShaderSource(fShaderID, 1, (char const* const*)&fsCode, NULL);
    glCompileShader(fShaderID);
    glGetShaderiv(fShaderID, GL_COMPILE_STATUS, &success);

    if (!success) {
        char log[1024];
        glGetShaderInfoLog(fShaderID, 1024, NULL, log);
        cerr << "\nError: fragmentShader(" << fShaderID << ", index: " << (int)index
            << ")'s compilation failed:\n" << log;

        return 0;
    }

    #ifdef DEBUG_SHADER
        cout << "\ncompiled fragment shader";
    #endif

    uint32_t programID = glCreateProgram();

    glAttachShader(programID, vShaderID);
    glAttachShader(programID, fShaderID);

    glLinkProgram(programID);

    #ifdef DEBUG_SHADER
        cout << "\nLinking... Program ID: " << programID;
    #endif

    glGetProgramiv(programID, GL_LINK_STATUS, &success);

    if (!success) {
        char log[1024];
        glGetProgramInfoLog(programID, 1024, NULL, log);
        cerr << "\nError while linking shader program:\n" << log;
    }

    glDeleteShader(vShaderID);
    glDeleteShader(fShaderID);

    return programID;
}

void initShaders() {
    for (uint8_t i=0; i<TOTAL_SHADERS; ++i) {
        shaders[i].programId = compile(i, shaders[i].vsIndex, shaders[i].fsIndex);
    }

    uint32_t id = shaders[SHADER_DEFAULT].programId;
    uniforms[U_DEFAULT_COLOR] = glGetUniformLocation(id, "solidColor");

    id = shaders[SHADER_TEXT].programId;
    uniforms[U_TEXT_COLOR] = glGetUniformLocation(id, "textColor");

    id = shaders[SHADER_ICON_BUTTON].programId;
    uniforms[U_ICON_BUTTON_COLOR] = glGetUniformLocation(id, "solidColor");
    uniforms[U_ICON_BUTTON_TOP_LEFT] = glGetUniformLocation(id, "topLeft");
    uniforms[U_ICON_BUTTON_BG_COLOR] = glGetUniformLocation(id, "backgroundColor");
    uniforms[U_ICON_BUTTON_HOV_COLOR] = glGetUniformLocation(id, "hoverColor");
    uniforms[U_ICON_BUTTON_WIDTH] = glGetUniformLocation(id, "width");
    uniforms[U_ICON_BUTTON_HEIGHT] = glGetUniformLocation(id, "height");
    uniforms[U_ICON_BUTTON_CURSOR] = glGetUniformLocation(id, "cursor");
    uniforms[U_ICON_BUTTON_TEXTURE] = glGetUniformLocation(id, "iconTexture");

    id = shaders[SHADER_BUTTON].programId;
    uniforms[U_BUTTON_BG_COLOR] = glGetUniformLocation(id, "backgroundColor");
    uniforms[U_BUTTON_HOV_COLOR] = glGetUniformLocation(id, "hoverColor");
    uniforms[U_BUTTON_BORDER_RADIUS] = glGetUniformLocation(id, "borderRadius");
    uniforms[U_BUTTON_TOP_LEFT] = glGetUniformLocation(id, "topLeft");
    uniforms[U_BUTTON_DIMENSIONS] = glGetUniformLocation(id, "dimensions");
    uniforms[U_BUTTON_CURSOR] = glGetUniformLocation(id, "cursor");

    id = shaders[SHADER_TEXT_INPUT].programId;
    uniforms[U_TEXT_INPUT_COLOR] = glGetUniformLocation(id, "color");
    uniforms[U_TEXT_INPUT_BG_COLOR] = glGetUniformLocation(id, "backgroundColor");
    uniforms[U_TEXT_INPUT_HOV_COLOR] = glGetUniformLocation(id, "hoverColor");
    uniforms[U_TEXT_INPUT_BORDER_RADIUS] = glGetUniformLocation(id, "borderRadius");
    uniforms[U_TEXT_INPUT_TOP_LEFT] = glGetUniformLocation(id, "topLeft");
    uniforms[U_TEXT_INPUT_DIMENSIONS] = glGetUniformLocation(id, "dimensions");
    uniforms[U_TEXT_INPUT_CURSOR] = glGetUniformLocation(id, "cursor");

    id = shaders[SHADER_TEXT_CURSOR].programId;
    uniforms[U_TEXT_CURSOR_COLOR] = glGetUniformLocation(id, "color");
    uniforms[U_TEXT_CURSOR_TIME] = glGetUniformLocation(id, "time");

    id = shaders[SHADER_CB_TEXT_HIGHLIGHT].programId;
    uniforms[U_TEXT_HL_COLOR] = glGetUniformLocation(id, "solidColor");
    uniforms[U_TEXT_HL_SCROLL_OFFSET_Y] = glGetUniformLocation(id, "scrollOffsetY");

    id = shaders[SHADER_RECTANGLE].programId;
    uint32_t commonUniformsIndex = glGetUniformBlockIndex(id, "CommonUniforms");
    uint32_t colorUniformsIndex = glGetUniformBlockIndex(id, "ColorsUniforms");
    glUniformBlockBinding(id, commonUniformsIndex, COMMON_UNIFORMS_UBO_INDEX);
    glUniformBlockBinding(id, colorUniformsIndex, RECTANGLE_COLOR_INDEX_UBO_INDEX);
}

