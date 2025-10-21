#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "include/shader.hpp"

using namespace std;

string vertexShaders[] = {
    // Default Shader
    "#version 330 core\n"
    "layout (location = 0) in vec2 aPos;\n"
    "uniform vec2 resolution;\n"
    "void main() {\n"
        "vec2 uv = aPos / resolution * 2;\n"
        "gl_Position = vec4(uv.x - 1, 1 - uv.y, 0.0f, 1.0f);\n"
    "}\0",

    // Text Shader
    "#version 330 core\n"
    "layout (location = 0) in vec4 aPos;\n"
    "out vec2 TexCoords;\n"
    "uniform vec2 resolution;\n"
    "void main() {\n"
        "vec2 uv = aPos.xy / resolution * 2;\n"
        "gl_Position = vec4(uv.x - 1, 1 - uv.y, 0.0f, 1.0f);\n"
        "TexCoords = aPos.zw;\n"
    "}\0",

    // Icon Button Shader
    "#version 330 core\n"
    "layout (location = 0) in vec2 aPos;\n"
    "layout (location = 1) in vec2 aTexCoords;\n"
    "uniform vec2 resolution;\n"
    "uniform vec2 topLeft;\n"
    "out vec2 TexCoords;\n"
    "void main() {\n"
        "vec2 uv = (aPos + topLeft) / resolution * 2;\n"
        "gl_Position = vec4(uv.x - 1, 1 - uv.y, 0.0f, 1.0f);\n"
        "TexCoords = aTexCoords;\n"
    "}\0",
};

string fragmentShaders[] = {
    // Default Shader
    "#version 330 core\n"
    "out vec4 FragColor;\n"
    "uniform vec4 solidColor;\n"
    "uniform float time;\n"
    "void main() {\n"
        "FragColor = solidColor;\n"
    "}\0",

    // Text Shader
    "#version 330 core\n"
    "in vec2 TexCoords;\n"
    "out vec4 color;\n"
    "uniform sampler2D text;\n"
    "uniform vec3 textColor;\n"
    "void main() {\n"
        "vec4 sampled = vec4(1, 1, 1, texture(text, TexCoords).r);\n"
        "color = vec4(textColor, 1) * sampled;\n"
    "}\0",

    // Icon Button Shader
    "#version 330 core\n"
    "in vec2 TexCoords;\n"
    "out vec4 FragColor;\n"
    "uniform vec4 solidColor;\n"
    "uniform vec4 backgroundColor;\n"
    "uniform vec4 hoverColor;\n"
    "uniform float time;\n"
    "uniform float width;\n"
    "uniform float height;\n"
    "uniform vec2 topLeft;\n"
    "uniform vec2 resolution;\n"
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
    "uniform vec2 resolution;\n"
    "uniform vec2 dimensions;\n" // width, height
    "uniform vec2 cursor;\n"

    "void main() {\n"
        "vec2 uv = gl_FragCoord.xy;"
        "uv.y = resolution.y - uv.y;\n"
        "vec2 br = topLeft + dimensions;\n"

        "if (uv.x < topLeft.x || uv.y < topLeft.y || uv.x > br.x || uv.y > br.y) {\n"
            "FragColor = vec4(0);"
            "return;\n"
        "}\n"

        "vec2 tl_plus_br = topLeft + borderRadius;\n"
        "vec2 br_minus_br = br - borderRadius;\n"

        // "if (tl_plus_br.x == topLeft.x + 10 && tl_plus_br.y == topLeft.y + 10) {"
        //     "FragColor = vec4(0, 0, 1, 1);\n"
        //     "return;\n"
        // "}\n"

        // "if (uv.y < tl_plus_br.y) {"
        //     "FragColor = vec4(1, 0, 1, 1);\n"
        //     "return;\n"
        // "}\n"
        // "if (uv.x < tl_plus_br.x && uv.y < tl_plus_br.y && (distance(uv.xy, tl_plus_br) <= borderRadius)) {"
        //     "FragColor = vec4(0, 0, 1, 1);\n"
        //     "return;\n"
        // "}\n"
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
    "}\0"
};

Shader shaders[] = {
    { 0, 0, 0 }, // SHADER_DEFAULT
    { 0, 1, 1 }, // SHADER_TEXT
    { 0, 2, 2 }, // SHADER_ICON_BUTTON
    { 0, 0, 3 }, // SHADER_BUTTON
};

uint32_t compile(uint8_t index, uint8_t vs, uint8_t fs) {
    cout << "\nCompiling shaders (" << (int)index + 1 << " out of " << TOTAL_SHADERS << ")";
    uint32_t vShaderID = glCreateShader(GL_VERTEX_SHADER);

    if (vShaderID == 0) {
        cerr << "\nCould not create vertex shader " << (int)index;
    }

    int success;

    const char * vsCode = vertexShaders[(int)vs].c_str();
    const char * fsCode = fragmentShaders[(int)fs].c_str();

    // cout << "\nVertex shader code ("<< (int) vs << "): \n" << vsCode << "\n";
    // cout << "\nFragment shader code (// " << (int) fs << "): \n" << fsCode << "\n";

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

    // cout << "\ncompiled vertex shader";

    uint32_t fShaderID = glCreateShader(GL_FRAGMENT_SHADER);

    if (fShaderID == 0) {
        cerr << "\nCould not create fragment shader " << (int)index;
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

    // cout << "\ncompiled fragment shader";

	uint32_t programID = glCreateProgram();

	glAttachShader(programID, vShaderID);
	glAttachShader(programID, fShaderID);

	glLinkProgram(programID);

	// cout << "\nLinking... Program ID: " << programID;

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
}

